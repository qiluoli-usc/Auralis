#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Dsp/Oscillators/BasicOscillator.h"
#include "Dsp/Oscillators/Lfo.h"
#include "Dsp/Synth/ParameterState.h"

namespace auralis::dsp
{
class AuralisSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class AuralisVoice : public juce::SynthesiserVoice
{
public:
    explicit AuralisVoice(ParameterState& state);

    void prepare(const juce::dsp::ProcessSpec& spec);
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    bool canPlaySound(juce::SynthesiserSound* sound) override;

private:
    void updateSmoothingTargets();
    [[nodiscard]] float centsToRatio(float cents) const noexcept;

    ParameterState& parameters;

    juce::ADSR ampEnvelope;

    BasicOscillator osc1;
    BasicOscillator osc2;

    auralis::dsp::Lfo filterLfo;
    juce::dsp::LadderFilter<float> filter;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> filterCutoffHz;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> filterResonance;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> oscMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> lfoDepthHz;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> lfoRateHz;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> osc1DetuneCents;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> osc2DetuneCents;

    BasicOscillator::Waveform currentOsc1Waveform = BasicOscillator::Waveform::sine;
    BasicOscillator::Waveform currentOsc2Waveform = BasicOscillator::Waveform::sine;

    double sampleRate = 44100.0;
    float currentFrequency = 440.0f;
    float currentVelocity = 1.0f;
};
} // namespace auralis::dsp
