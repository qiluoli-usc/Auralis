#include "Dsp/Synth/AuralisVoice.h"

#include <cmath>

namespace auralis::dsp
{
namespace
{
constexpr float minimumCutoffHz = 20.0f;
constexpr float maximumCutoffHz = 20000.0f;
constexpr double smoothingTimeSeconds = 0.05;
}

AuralisVoice::AuralisVoice(ParameterState& state)
    : parameters(state)
{
}

void AuralisVoice::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec { spec.sampleRate, spec.maximumBlockSize, 1 }; // voice is mono

    osc1.prepare(sampleRate);
    osc2.prepare(sampleRate);
    filterLfo.prepare(sampleRate);

    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    filter.prepare(monoSpec);
    filter.reset();

    filterCutoffHz.reset(sampleRate, smoothingTimeSeconds);
    filterResonance.reset(sampleRate, smoothingTimeSeconds);
    oscMix.reset(sampleRate, smoothingTimeSeconds);
    lfoDepthHz.reset(sampleRate, smoothingTimeSeconds);
    lfoRateHz.reset(sampleRate, smoothingTimeSeconds);
    osc1DetuneCents.reset(sampleRate, smoothingTimeSeconds);
    osc2DetuneCents.reset(sampleRate, smoothingTimeSeconds);

    ampEnvelope.setSampleRate(sampleRate);
}

void AuralisVoice::startNote(int midiNoteNumber,
                             float velocity,
                             juce::SynthesiserSound*,
                             int /*currentPitchWheelPosition*/)
{
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    currentVelocity = juce::jlimit(0.0f, 1.0f, velocity);
    ampEnvelope.setParameters(parameters.getAmpEnvelopeParameters());
    ampEnvelope.noteOn();

    const auto osc1Wave = waveformFromIndex(parameters.getOscWaveformIndex(0));
    const auto osc2Wave = waveformFromIndex(parameters.getOscWaveformIndex(1));
    currentOsc1Waveform = osc1Wave;
    currentOsc2Waveform = osc2Wave;
    osc1.setWaveform(osc1Wave);
    osc2.setWaveform(osc2Wave);

    osc1.resetPhase();
    osc2.resetPhase();
    filter.reset();

    filterCutoffHz.setCurrentAndTargetValue(parameters.getFilterCutoffHz());
    filterResonance.setCurrentAndTargetValue(parameters.getFilterResonance());
    oscMix.setCurrentAndTargetValue(parameters.getOscMix());
    lfoDepthHz.setCurrentAndTargetValue(parameters.getLfoDepthHz());
    lfoRateHz.setCurrentAndTargetValue(parameters.getLfoRateHz());
    osc1DetuneCents.setCurrentAndTargetValue(parameters.getOscDetuneCents(0));
    osc2DetuneCents.setCurrentAndTargetValue(parameters.getOscDetuneCents(1));
}

void AuralisVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampEnvelope.reset();
    }
}

void AuralisVoice::pitchWheelMoved(int)
{
}

void AuralisVoice::controllerMoved(int, int)
{
}

bool AuralisVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<AuralisSound*>(sound) != nullptr;
}

void AuralisVoice::updateSmoothingTargets()
{
    const auto osc1WaveIndex = parameters.getOscWaveformIndex(0);
    const auto osc2WaveIndex = parameters.getOscWaveformIndex(1);
    const auto newOsc1Waveform = waveformFromIndex(osc1WaveIndex);
    const auto newOsc2Waveform = waveformFromIndex(osc2WaveIndex);

    if (newOsc1Waveform != currentOsc1Waveform)
    {
        currentOsc1Waveform = newOsc1Waveform;
        osc1.setWaveform(currentOsc1Waveform);
    }

    if (newOsc2Waveform != currentOsc2Waveform)
    {
        currentOsc2Waveform = newOsc2Waveform;
        osc2.setWaveform(currentOsc2Waveform);
    }

    ampEnvelope.setParameters(parameters.getAmpEnvelopeParameters());

    filterCutoffHz.setTargetValue(parameters.getFilterCutoffHz());
    filterResonance.setTargetValue(parameters.getFilterResonance());
    oscMix.setTargetValue(parameters.getOscMix());
    lfoDepthHz.setTargetValue(parameters.getLfoDepthHz());
    lfoRateHz.setTargetValue(parameters.getLfoRateHz());
    osc1DetuneCents.setTargetValue(parameters.getOscDetuneCents(0));
    osc2DetuneCents.setTargetValue(parameters.getOscDetuneCents(1));
}

float AuralisVoice::centsToRatio(float cents) const noexcept
{
    return std::pow(2.0f, cents / 1200.0f);
}

void AuralisVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    updateSmoothingTargets();

    auto* leftChannel = outputBuffer.getWritePointer(0, startSample);
    auto* rightChannel = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto envValue = ampEnvelope.getNextSample();

        const auto osc1Ratio = centsToRatio(osc1DetuneCents.getNextValue());
        const auto osc2Ratio = centsToRatio(osc2DetuneCents.getNextValue());

        osc1.setFrequency(currentFrequency * osc1Ratio);
        osc2.setFrequency(currentFrequency * osc2Ratio);

        const auto oscSample1 = osc1.getNextSample();
        const auto oscSample2 = osc2.getNextSample();
        const auto mix = juce::jlimit(0.0f, 1.0f, oscMix.getNextValue());
        const auto combined = oscSample1 * (1.0f - mix) + oscSample2 * mix;

        const auto lfoRateValue = juce::jmax(0.0f, lfoRateHz.getNextValue());
        filterLfo.setFrequency(lfoRateValue);
        const auto lfoValue = filterLfo.getNextSample();
        const auto lfoDepthValue = lfoDepthHz.getNextValue();

        const auto baseCutoff = filterCutoffHz.getNextValue();
        const auto modulatedCutoff = juce::jlimit(minimumCutoffHz,
                                                  maximumCutoffHz,
                                                  baseCutoff + (lfoValue * lfoDepthValue));

        filter.setCutoffFrequencyHz(modulatedCutoff);
        filter.setResonance(filterResonance.getNextValue());

        auto filteredSample = filter.processSample(0, combined);
        filteredSample *= envValue * currentVelocity;

        leftChannel[sample] += filteredSample;
        if (rightChannel != nullptr)
            rightChannel[sample] += filteredSample;

        if (! ampEnvelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }

    if (! ampEnvelope.isActive())
        clearCurrentNote();
}
} // namespace auralis::dsp
