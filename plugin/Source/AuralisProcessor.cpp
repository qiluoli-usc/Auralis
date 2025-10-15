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
}

void AuralisAudioProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // No DSP yet.
}

void AuralisAudioProcessor::releaseResources()
{
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

    midiMessages.clear();

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
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
