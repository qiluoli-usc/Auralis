#include "AuralisProcessor.h"

#include "AuralisEditor.h"
#include "Param/ParamIDs.h"
#include "Param/ParamTree.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

using namespace juce;

AuralisAudioProcessor::AuralisAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                         .withOutput("Output", AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, auralis::params::parameterGroup, createParameterLayout())
{
    parameterState.initialise(parameters);

    constexpr int numVoices = 8;
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new auralis::dsp::AuralisVoice(parameterState));

    synth.addSound(new auralis::dsp::AuralisSound());
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

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuralisAudioProcessor();
}
