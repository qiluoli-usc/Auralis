#include "AuralisProcessor.h"

#include "AuralisEditor.h"
#include "Param/ParamIDs.h"
#include "Param/ParamTree.h"

#include "Mapping/PatchMessage.h"
#include "Mapping/PromptServiceClient.h"
#include "Mapping/RingBuffer.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <deque>
#include <optional>
#include <utility>

#include <memory>

using namespace juce;

namespace
{
float variantToFloat(const juce::var& value, float fallback) noexcept
{
    if (value.isDouble() || value.isInt() || value.isInt64())
        return static_cast<float>(static_cast<double>(value));

    return fallback;
}

int waveformIndexFromName(const juce::String& name) noexcept
{
    const auto lowercase = name.trim().toLowerCase();
    if (lowercase == "saw")
        return 1;
    if (lowercase == "square")
        return 2;
    if (lowercase == "triangle")
        return 3;

    return 0;
}

juce::String waveformNameFromIndex(int index) noexcept
{
    switch (index)
    {
        case 1: return "saw";
        case 2: return "square";
        case 3: return "triangle";
        case 4: return "noise";
        default: break;
    }

    return "sine";
}

bool populatePatchMessage(const juce::var& patchVar,
                          juce::AudioProcessorValueTreeState& state,
                          auralis::mapping::PatchMessage& message)
{
    if (! patchVar.isObject())
        return false;

    bool anyChange = false;

    if (auto* root = patchVar.getDynamicObject())
    {
        std::optional<float> oscLevels[2];

        if (auto* oscArray = root->getProperty("oscillators").getArray())
        {
            for (int i = 0; i < juce::jmin(2, oscArray->size()); ++i)
            {
                if (auto* oscObj = (*oscArray)[i].getDynamicObject())
                {
                    const auto waveformName = oscObj->getProperty("waveform").toString();
                    if (waveformName.isNotEmpty())
                    {
                        message.hasOscWaveform[i] = true;
                        message.oscWaveform[i] = waveformIndexFromName(waveformName);
                        anyChange = true;
                    }

                    if (oscObj->hasProperty("detune_cents"))
                    {
                        message.hasDetune[i] = true;
                        message.detuneCents[i] = juce::jlimit(-200.0f,
                                                               200.0f,
                                                               variantToFloat(oscObj->getProperty("detune_cents"), 0.0f));
                        anyChange = true;
                    }

                    if (oscObj->hasProperty("level"))
                        oscLevels[i] = juce::jlimit(0.0f, 1.0f, variantToFloat(oscObj->getProperty("level"), 0.5f));
                }
            }
        }

        if (oscLevels[0].has_value() || oscLevels[1].has_value())
        {
            float currentMix = 0.5f;
            if (auto* raw = state.getRawParameterValue(auralis::params::oscMix))
                currentMix = raw->load();

            const auto osc0 = oscLevels[0].value_or(1.0f - currentMix);
            const auto osc1 = oscLevels[1].value_or(currentMix);
            const auto total = juce::jmax(0.0001f, osc0 + osc1);
            message.hasOscMix = true;
            message.oscMix = juce::jlimit(0.0f, 1.0f, osc1 / total);
            anyChange = true;
        }

        if (auto* filterObj = root->getProperty("filter").getDynamicObject())
        {
            if (filterObj->hasProperty("cutoff_hz"))
            {
                message.hasFilterCutoff = true;
                message.filterCutoff = juce::jlimit(20.0f,
                                                    20000.0f,
                                                    variantToFloat(filterObj->getProperty("cutoff_hz"), 1000.0f));
                anyChange = true;
            }

            if (filterObj->hasProperty("resonance"))
            {
                message.hasFilterResonance = true;
                const auto resonanceNorm = juce::jlimit(0.0f,
                                                        1.0f,
                                                        variantToFloat(filterObj->getProperty("resonance"), 0.2f));
                message.filterResonance = juce::jmap(resonanceNorm, 0.0f, 1.0f, 0.1f, 10.0f);
                anyChange = true;
            }
        }

        if (auto* envObj = root->getProperty("env").getDynamicObject())
        {
            if (auto* ampObj = envObj->getProperty("amp").getDynamicObject())
            {
                if (ampObj->hasProperty("attack_ms"))
                {
                    message.hasEnvAttack = true;
                    message.envAttack = juce::jlimit(0.1f,
                                                     5000.0f,
                                                     variantToFloat(ampObj->getProperty("attack_ms"), 10.0f));
                    anyChange = true;
                }

                if (ampObj->hasProperty("decay_ms"))
                {
                    message.hasEnvDecay = true;
                    message.envDecay = juce::jlimit(1.0f,
                                                    5000.0f,
                                                    variantToFloat(ampObj->getProperty("decay_ms"), 120.0f));
                    anyChange = true;
                }

                if (ampObj->hasProperty("sustain"))
                {
                    message.hasEnvSustain = true;
                    message.envSustain = juce::jlimit(0.0f,
                                                       1.0f,
                                                       variantToFloat(ampObj->getProperty("sustain"), 0.75f));
                    anyChange = true;
                }

                if (ampObj->hasProperty("release_ms"))
                {
                    message.hasEnvRelease = true;
                    message.envRelease = juce::jlimit(1.0f,
                                                       8000.0f,
                                                       variantToFloat(ampObj->getProperty("release_ms"), 250.0f));
                    anyChange = true;
                }
            }
        }

        if (auto* lfoArray = root->getProperty("lfo").getArray())
        {
            if (! lfoArray->isEmpty())
            {
                if (auto* lfoObj = (*lfoArray)[0].getDynamicObject())
                {
                    if (lfoObj->hasProperty("rate_hz"))
                    {
                        message.hasLfoRate = true;
                        message.lfoRate = juce::jlimit(0.1f,
                                                       30.0f,
                                                       variantToFloat(lfoObj->getProperty("rate_hz"), 2.0f));
                        anyChange = true;
                    }

                    float depthNormalised = 0.0f;
                    if (auto* targets = lfoObj->getProperty("targets").getArray())
                    {
                        for (const auto& targetVar : *targets)
                        {
                            if (auto* targetObj = targetVar.getDynamicObject())
                            {
                                if (targetObj->getProperty("param").toString() == "filter.cutoff_hz")
                                {
                                    depthNormalised = juce::jlimit(0.0f,
                                                                   1.0f,
                                                                   variantToFloat(targetObj->getProperty("depth"), 0.0f));
                                    break;
                                }
                            }
                        }
                    }

                    if (depthNormalised > 0.0f)
                    {
                        message.hasLfoDepth = true;
                        message.lfoDepth = juce::jmap(depthNormalised, 0.0f, 1.0f, 0.0f, 4000.0f);
                        anyChange = true;
                    }
                }
            }
        }

        if (auto* fxObj = root->getProperty("fx").getDynamicObject())
        {
            if (auto* reverbObj = fxObj->getProperty("reverb").getDynamicObject())
            {
                if (reverbObj->hasProperty("mix"))
                {
                    message.hasReverbMix = true;
                    message.reverbMix = juce::jlimit(0.0f,
                                                     1.0f,
                                                     variantToFloat(reverbObj->getProperty("mix"), 0.2f));
                    anyChange = true;
                }
            }
        }
    }

    return anyChange;
}

class PromptServiceWorker : private juce::Thread
{
public:
    struct Job
    {
        juce::String prompt;
        bool dryRun = true;
        juce::StringArray styleHints;
        std::optional<int> bpm;
        std::optional<int> seed;
        double startTimeMs = 0.0;
        bool measureLatency = false;
    };

    PromptServiceWorker(AuralisAudioProcessor& owner, auralis::mapping::PromptServiceClient& clientIn)
        : juce::Thread("PromptServiceWorker"), processor(owner), client(clientIn)
    {
        startThread();
    }

    ~PromptServiceWorker() override
    {
        signalThreadShouldExit();
        jobAvailable.signal();
        stopThread(4000);
    }

    void enqueue(Job job)
    {
        {
            const juce::ScopedLock lock(queueLock);
            jobs.push_back(std::move(job));
        }
        jobAvailable.signal();
    }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            if (! jobAvailable.wait(500))
                continue;

            for (;;)
            {
                if (threadShouldExit())
                    return;

                Job job;

                {
                    const juce::ScopedLock lock(queueLock);
                    if (jobs.empty())
                        break;

                    job = std::move(jobs.front());
                    jobs.pop_front();
                }

                handleJob(job);
            }
        }
    }

    void handleJob(const Job& job)
    {
        processor.publishPreview("Contacting mapper service…");

        auto response = client.mapPrompt(job.prompt, job.styleHints, job.bpm, job.seed);

        juce::var patchVar;

        if (! response.ok)
        {
            auto localResult = processor.promptParser.parse(job.prompt);
            patchVar = localResult.patch;
            processor.publishPreview("Mapper offline. Using local rules:\n" + juce::JSON::toString(patchVar, true));

            if (! job.dryRun)
            {
                processor.scheduleParameterUpdate(patchVar);
                auralis::mapping::PatchMessage message;
                if (populatePatchMessage(patchVar, processor.parameters, message))
                {
                    message.measureLatency = job.measureLatency;
                    message.requestStartMs = job.startTimeMs;
                    processor.enqueuePatchForAudio(message);
                }
            }
            return;
        }

        juce::String parseError;
        patchVar = juce::JSON::parse(response.body, parseError);
        if (parseError.isNotEmpty())
        {
            processor.publishPreview("Invalid JSON from mapper: " + parseError);
            return;
        }

        const auto formatted = juce::JSON::toString(patchVar, true);
        processor.publishPreview(formatted);

        if (! job.dryRun)
        {
            processor.scheduleParameterUpdate(patchVar);
            auralis::mapping::PatchMessage message;
            if (populatePatchMessage(patchVar, processor.parameters, message))
            {
                message.measureLatency = job.measureLatency;
                message.requestStartMs = job.startTimeMs;
                processor.enqueuePatchForAudio(message);
            }
        }
    }

    AuralisAudioProcessor& processor;
    auralis::mapping::PromptServiceClient& client;

    juce::WaitableEvent jobAvailable;
    juce::CriticalSection queueLock;
    std::deque<Job> jobs;
};
} // namespace

AuralisAudioProcessor::AuralisAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                         .withOutput("Output", AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, auralis::params::parameterGroup, createParameterLayout()),
      promptClient()
{
    parameterState.initialise(parameters);
    patchApplier = std::make_unique<auralis::mapping::JsonPatchApplier>(parameters);
    parameterHistory = std::make_unique<auralis::utils::ParameterHistory>(parameters);
    macroController = std::make_unique<auralis::utils::MacroController>(parameters);

    constexpr int numVoices = 8;
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new auralis::dsp::AuralisVoice(parameterState));

    synth.addSound(new auralis::dsp::AuralisSound());

    promptWorker = std::make_unique<PromptServiceWorker>(*this, promptClient);
}

AuralisAudioProcessor::~AuralisAudioProcessor()
{
    macroController.reset();
    parameterHistory.reset();
    promptWorker.reset();
}

void AuralisAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  static_cast<juce::uint32>(samplesPerBlock),
                                  static_cast<juce::uint32>(getTotalNumOutputChannels()) };

    synth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<auralis::dsp::AuralisVoice*>(synth.getVoice(i)))
            voice->prepare(spec);

    reverb.prepare(spec);
    reverb.reset();

    reverbBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, false, true);

    reverbMixSmoother.reset(sampleRate, 0.05);
    reverbMixSmoother.setCurrentAndTargetValue(parameterState.getReverbMix());

    isPrepared = true;
}

void AuralisAudioProcessor::releaseResources()
{
    isPrepared = false;
    reverb.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AuralisAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void AuralisAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    auralis::mapping::PatchMessage message;
    while (pendingPatchMessages.pop(message))
        applyPatchMessage(message);

    if (! isPrepared)
    {
        buffer.clear();
        midiMessages.clear();
        return;
    }

    buffer.clear();

    reverbMixSmoother.setTargetValue(parameterState.getReverbMix());

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    midiMessages.clear();

    jassert(reverbBuffer.getNumSamples() >= buffer.getNumSamples());
    jassert(reverbBuffer.getNumChannels() == buffer.getNumChannels());

    reverbBuffer.makeCopyOf(buffer, false);

    juce::dsp::AudioBlock<float> wetBlock(reverbBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    reverb.process(wetContext);

    float* const* dryChannels = buffer.getArrayOfWritePointers();
    const float* const* wetChannels = reverbBuffer.getArrayOfReadPointers();
    const auto numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto mix = juce::jlimit(0.0f, 1.0f, reverbMixSmoother.getNextValue());

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* dry = dryChannels[channel];
            auto* wet = wetChannels[channel];
            dry[sample] = (dry[sample] * (1.0f - mix)) + (wet[sample] * mix);
        }
    }
}

AudioProcessorEditor* AuralisAudioProcessor::createEditor()
{
    return new AuralisAudioProcessorEditor(*this);
}

bool AuralisAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AuralisAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AuralisAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

void AuralisAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AuralisAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(ValueTree::fromXml(*xmlState));
        }
    }
}

void AuralisAudioProcessor::saveStateToFile(const File& file)
{
    MemoryBlock block;
    getStateInformation(block);
    file.replaceWithData(block.getData(), block.getSize());
}

void AuralisAudioProcessor::loadStateFromFile(const File& file)
{
    if (! file.existsAsFile())
        return;

    MemoryBlock block;
    if (file.loadFileAsData(block))
        setStateInformation(block.getData(), static_cast<int>(block.getSize()));
}

void AuralisAudioProcessor::exportPatchToFile(const juce::File& file)
{
    auto patchVar = buildPatchVarFromState();
    const auto json = juce::JSON::toString(patchVar, true);
    auto parent = file.getParentDirectory();
    if (! parent.exists())
        parent.createDirectory();
    file.replaceWithText(json);
    publishPreview("Exported patch to " + file.getFileName());
}

void AuralisAudioProcessor::importPatchFromFile(const juce::File& file)
{
    if (! file.existsAsFile())
    {
        publishPreview("Patch file not found.");
        return;
    }

    juce::FileInputStream stream(file);
    if (! stream.openedOk())
    {
        publishPreview("Unable to open patch file.");
        return;
    }

    const auto jsonText = stream.readEntireStreamAsString();

    juce::String parseError;
    auto patchVar = juce::JSON::parse(jsonText, parseError);

    if (parseError.isNotEmpty())
    {
        publishPreview("Patch parse error: " + parseError);
        return;
    }

    if (! patchVar.isObject())
    {
        publishPreview("Patch must be a JSON object.");
        return;
    }

    publishPreview("Loaded patch:\n" + juce::JSON::toString(patchVar, true));

    scheduleParameterUpdate(patchVar);

    auralis::mapping::PatchMessage message;
    if (populatePatchMessage(patchVar, parameters, message))
    {
        message.measureLatency = false;
        message.requestStartMs = juce::Time::getMillisecondCounterHiRes();
        enqueuePatchForAudio(message);
    }
}

bool AuralisAudioProcessor::undoLastChange()
{
    if (parameterHistory == nullptr)
        return false;

    const auto performed = parameterHistory->undo();
    if (performed)
        publishPreview("Undo applied.");
    return performed;
}

bool AuralisAudioProcessor::redoLastChange()
{
    if (parameterHistory == nullptr)
        return false;

    const auto performed = parameterHistory->redo();
    if (performed)
        publishPreview("Redo applied.");
    return performed;
}

bool AuralisAudioProcessor::canUndo() const
{
    return parameterHistory != nullptr && parameterHistory->canUndo();
}

bool AuralisAudioProcessor::canRedo() const
{
    return parameterHistory != nullptr && parameterHistory->canRedo();
}

double AuralisAudioProcessor::getLastPromptLatencyMs() const
{
    return lastPromptLatencyMs.load(std::memory_order_acquire);
}

juce::var AuralisAudioProcessor::buildPatchVarFromState() const
{
    auto getValue = [this](const juce::String& paramID, float fallback) -> float
    {
        if (auto* ptr = parameters.getRawParameterValue(paramID))
            return ptr->load();
        return fallback;
    };

    juce::DynamicObject::Ptr root(new juce::DynamicObject());

    auto* oscArray = new juce::Array<juce::var>();
    const auto mix = juce::jlimit(0.0f, 1.0f, getValue(auralis::params::oscMix, 0.5f));
    const float oscLevelRaw[2] { juce::jmax(0.0f, 1.0f - mix), juce::jmax(0.0f, mix) };
    const auto levelTotal = juce::jmax(0.0001f, oscLevelRaw[0] + oscLevelRaw[1]);

    for (int i = 0; i < 2; ++i)
    {
        juce::DynamicObject::Ptr osc(new juce::DynamicObject());
        const auto waveformIndex = juce::roundToInt(getValue(i == 0 ? auralis::params::osc1Waveform
                                                                    : auralis::params::osc2Waveform,
                                                              0.0f));
        osc->setProperty("waveform", waveformNameFromIndex(waveformIndex));
        const auto detune = juce::jlimit(-100.0f, 100.0f,
                                         getValue(i == 0 ? auralis::params::osc1DetuneCents
                                                         : auralis::params::osc2DetuneCents,
                                                   0.0f));
        osc->setProperty("detune_cents", detune);
        osc->setProperty("semi_offset", 0);
        osc->setProperty("level", juce::jlimit(0.0f, 1.0f, oscLevelRaw[i] / levelTotal));
        oscArray->add(juce::var(osc));
    }

    root->setProperty("oscillators", juce::var(oscArray));

    juce::DynamicObject::Ptr filter(new juce::DynamicObject());
    filter->setProperty("type", "ladder_lp");
    const auto cutoff = juce::jlimit(20.0f, 20000.0f, getValue(auralis::params::filterCutoffHz, 1000.0f));
    filter->setProperty("cutoff_hz", cutoff);
    const auto resonanceValue = juce::jlimit(0.1f, 10.0f, getValue(auralis::params::filterResonance, 0.7f));
    const auto resonanceNormalised = juce::jlimit(0.0f, 1.0f, juce::jmap(resonanceValue, 0.1f, 10.0f, 0.0f, 1.0f));
    filter->setProperty("resonance", resonanceNormalised);
    filter->setProperty("drive", 0.0f);
    root->setProperty("filter", juce::var(filter));

    juce::DynamicObject::Ptr amp(new juce::DynamicObject());
    amp->setProperty("attack_ms", juce::jlimit(0.0f, 5000.0f, getValue(auralis::params::envAttackMs, 10.0f)));
    amp->setProperty("decay_ms", juce::jlimit(0.0f, 5000.0f, getValue(auralis::params::envDecayMs, 120.0f)));
    amp->setProperty("sustain", juce::jlimit(0.0f, 1.0f, getValue(auralis::params::envSustain, 0.75f)));
    amp->setProperty("release_ms", juce::jlimit(0.0f, 10000.0f, getValue(auralis::params::envReleaseMs, 250.0f)));

    juce::DynamicObject::Ptr env(new juce::DynamicObject());
    env->setProperty("amp", juce::var(amp));
    root->setProperty("env", juce::var(env));

    auto* lfoTargets = new juce::Array<juce::var>();
    const auto lfoDepthHz = juce::jlimit(0.0f, 4000.0f, getValue(auralis::params::lfoDepthHz, 0.0f));
    const auto lfoDepthNormalised = juce::jlimit(0.0f, 1.0f, lfoDepthHz / 4000.0f);
    if (lfoDepthNormalised > 0.0f)
    {
        juce::DynamicObject::Ptr target(new juce::DynamicObject());
        target->setProperty("param", "filter.cutoff_hz");
        target->setProperty("depth", lfoDepthNormalised);
        lfoTargets->add(juce::var(target));
    }

    juce::DynamicObject::Ptr lfo(new juce::DynamicObject());
    lfo->setProperty("rate_hz", juce::jlimit(0.01f, 30.0f, getValue(auralis::params::lfoRateHz, 2.0f)));
    lfo->setProperty("shape", "sine");
    lfo->setProperty("targets", juce::var(lfoTargets));

    auto* lfoArray = new juce::Array<juce::var>();
    lfoArray->add(juce::var(lfo));
    root->setProperty("lfo", juce::var(lfoArray));

    juce::DynamicObject::Ptr reverb(new juce::DynamicObject());
    reverb->setProperty("mix", juce::jlimit(0.0f, 1.0f, getValue(auralis::params::reverbMix, 0.2f)));

    juce::DynamicObject::Ptr fx(new juce::DynamicObject());
    fx->setProperty("reverb", juce::var(reverb));
    root->setProperty("fx", juce::var(fx));

    return juce::var(root);
}

void AuralisAudioProcessor::queuePrompt(const juce::String& prompt, bool dryRun)
{
    const auto trimmed = prompt.trim();
    if (trimmed.isEmpty())
    {
        publishPreview("Prompt is empty.");
        return;
    }

    const auto requestStart = juce::Time::getMillisecondCounterHiRes();

    if (promptWorker != nullptr)
    {
        PromptServiceWorker::Job job;
        job.prompt = trimmed;
        job.dryRun = dryRun;
        job.startTimeMs = requestStart;
        job.measureLatency = ! dryRun;

        publishPreview("Queued prompt for mapping…");
        promptWorker->enqueue(std::move(job));
        return;
    }

    auto fallback = promptParser.parse(trimmed);
    auto jsonText = juce::JSON::toString(fallback.patch, true);
    publishPreview(jsonText);

    if (! dryRun)
    {
        scheduleParameterUpdate(fallback.patch);

        auralis::mapping::PatchMessage message;
        if (populatePatchMessage(fallback.patch, parameters, message))
        {
            message.measureLatency = true;
            message.requestStartMs = requestStart;
            enqueuePatchForAudio(message);
        }
    }
}

bool AuralisAudioProcessor::fetchLatestPreview(juce::String& previewOut)
{
    if (! previewAvailable.load(std::memory_order_acquire))
        return false;

    {
        const juce::ScopedLock lock(previewLock);
        previewOut = latestPreview;
    }

    previewAvailable.store(false, std::memory_order_release);
    return true;
}

void AuralisAudioProcessor::setPromptServiceBaseUrl(const juce::String& newUrl)
{
    promptClient.setBaseUrl(newUrl);
}

void AuralisAudioProcessor::publishPreview(const juce::String& previewText)
{
    {
        const juce::ScopedLock lock(previewLock);
        latestPreview = previewText;
    }
    previewAvailable.store(true, std::memory_order_release);
}

void AuralisAudioProcessor::enqueuePatchForAudio(const auralis::mapping::PatchMessage& message)
{
    auralis::mapping::PatchMessage queued = message;
    while (! pendingPatchMessages.push(queued))
    {
        auralis::mapping::PatchMessage discarded;
        if (! pendingPatchMessages.pop(discarded))
            break;
    }
}

void AuralisAudioProcessor::scheduleParameterUpdate(const juce::var& patchVar)
{
    if (patchApplier == nullptr)
        return;

    juce::var patchCopy(patchVar);
    juce::MessageManager::callAsync([this, patchCopy]() mutable {
        if (patchApplier != nullptr)
            patchApplier->apply(patchCopy);
    });
}

void AuralisAudioProcessor::applyPatchMessage(const auralis::mapping::PatchMessage& message)
{
    using std::memory_order_release;

    if (message.hasOscWaveform[0] && parameterState.osc1Waveform != nullptr)
        parameterState.osc1Waveform->store(static_cast<float>(message.oscWaveform[0]), memory_order_release);
    if (message.hasOscWaveform[1] && parameterState.osc2Waveform != nullptr)
        parameterState.osc2Waveform->store(static_cast<float>(message.oscWaveform[1]), memory_order_release);

    if (message.hasDetune[0] && parameterState.osc1DetuneCents != nullptr)
        parameterState.osc1DetuneCents->store(message.detuneCents[0], memory_order_release);
    if (message.hasDetune[1] && parameterState.osc2DetuneCents != nullptr)
        parameterState.osc2DetuneCents->store(message.detuneCents[1], memory_order_release);

    if (message.hasOscMix && parameterState.oscMix != nullptr)
        parameterState.oscMix->store(message.oscMix, memory_order_release);

    if (message.hasFilterCutoff && parameterState.filterCutoffHz != nullptr)
        parameterState.filterCutoffHz->store(message.filterCutoff, memory_order_release);

    if (message.hasFilterResonance && parameterState.filterResonance != nullptr)
        parameterState.filterResonance->store(message.filterResonance, memory_order_release);

    if (message.hasEnvAttack && parameterState.envAttackMs != nullptr)
        parameterState.envAttackMs->store(message.envAttack, memory_order_release);
    if (message.hasEnvDecay && parameterState.envDecayMs != nullptr)
        parameterState.envDecayMs->store(message.envDecay, memory_order_release);
    if (message.hasEnvSustain && parameterState.envSustain != nullptr)
        parameterState.envSustain->store(message.envSustain, memory_order_release);
    if (message.hasEnvRelease && parameterState.envReleaseMs != nullptr)
        parameterState.envReleaseMs->store(message.envRelease, memory_order_release);

    if (message.hasLfoRate && parameterState.lfoRateHz != nullptr)
        parameterState.lfoRateHz->store(message.lfoRate, memory_order_release);
    if (message.hasLfoDepth && parameterState.lfoDepth != nullptr)
        parameterState.lfoDepth->store(message.lfoDepth, memory_order_release);

    if (message.hasReverbMix && parameterState.reverbMix != nullptr)
        parameterState.reverbMix->store(message.reverbMix, memory_order_release);

    if (message.measureLatency)
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto latencyMs = juce::jmax(0.0, now - message.requestStartMs);
        lastPromptLatencyMs.store(latencyMs, std::memory_order_release);
    }
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuralisAudioProcessor();
}
