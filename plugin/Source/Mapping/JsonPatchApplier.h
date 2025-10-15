#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace auralis::mapping
{
class JsonPatchApplier
{
public:
    explicit JsonPatchApplier(juce::AudioProcessorValueTreeState& state);

    void apply(const juce::var& patchVar);

private:
    void applyOscillators(const juce::Array<juce::var>& oscillators);
    void applyFilter(const juce::var& filterVar);
    void applyEnvelope(const juce::var& envVar);
    void applyLfo(const juce::Array<juce::var>& lfoArray);
    void applyFx(const juce::var& fxVar);

    static int waveformIndexFromName(const juce::String& name);
    void setChoiceParameter(const juce::String& paramID, int choiceValue);
    void setFloatParameter(const juce::String& paramID, float value);

    [[nodiscard]] float getCurrentFloatParameter(const juce::String& paramID) const;

    juce::AudioProcessorValueTreeState& state;
};
} // namespace auralis::mapping
