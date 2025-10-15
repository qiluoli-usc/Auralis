#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <memory>

#include "Dsp/Synth/AuralisVoice.h"
#include "Mapping/JsonPatchApplier.h"
#include "Mapping/PatchMessage.h"
#include "Mapping/PromptRuleParser.h"
#include "Mapping/PromptServiceClient.h"

class AuralisAudioProcessor : public juce::AudioProcessor
{
public:
    AuralisAudioProcessor();
    ~AuralisAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    void queuePrompt(const juce::String& prompt, bool dryRun);
    bool fetchLatestPreview(juce::String& previewOut);
    void setPromptServiceBaseUrl(const juce::String& newUrl);

    void saveStateToFile(const juce::File& file);
    void loadStateFromFile(const juce::File& file);

private:
    juce::AudioProcessorValueTreeState parameters;
    auralis::dsp::ParameterState parameterState;

    auralis::mapping::PromptRuleParser promptParser;
    std::unique_ptr<auralis::mapping::JsonPatchApplier> patchApplier;

    std::unique_ptr<class PromptServiceWorker> promptWorker;
    auralis::mapping::PromptServiceClient promptClient;
    auralis::mapping::RingBuffer<auralis::mapping::PatchMessage, 16> pendingPatchMessages;

    void applyPatchMessage(const auralis::mapping::PatchMessage& message);
    void publishPreview(const juce::String& previewText);
    void enqueuePatchForAudio(const auralis::mapping::PatchMessage& message);
    void scheduleParameterUpdate(const juce::var& patchVar);

    juce::CriticalSection previewLock;
    juce::String latestPreview;
    std::atomic<bool> previewAvailable { false };

    juce::Synthesiser synth;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> reverbBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbMixSmoother;

    bool isPrepared = false;

    friend class PromptServiceWorker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralisAudioProcessor)
};
