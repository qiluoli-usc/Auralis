#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace auralis::dsp
{
class BasicOscillator
{
public:
    enum class Waveform
    {
        sine = 0,
        saw,
        square,
        triangle
    };

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = 0.0f;
        updatePhaseIncrement();
    }

    void setFrequency(float newFrequency)
    {
        frequency = newFrequency;
        updatePhaseIncrement();
    }

    void setWaveform(Waveform newWaveform) { waveform = newWaveform; }

    void resetPhase(float newPhase = 0.0f) { phase = juce::jlimit(0.0f, 1.0f, newPhase); }

    [[nodiscard]] float getNextSample() noexcept
    {
        const float currentPhase = phase;
        advancePhase();

        switch (waveform)
        {
            case Waveform::sine:
                return std::sin(currentPhase * juce::MathConstants<float>::twoPi);
            case Waveform::saw:
                return (2.0f * currentPhase) - 1.0f;
            case Waveform::square:
                return currentPhase < 0.5f ? 1.0f : -1.0f;
            case Waveform::triangle:
            default:
                return 1.0f - 4.0f * std::abs(currentPhase - 0.5f);
        }
    }

private:
    void advancePhase() noexcept
    {
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    void updatePhaseIncrement() noexcept
    {
        if (sampleRate <= 0.0)
            return;

        phaseIncrement = juce::jlimit(0.0f, 0.5f, frequency / static_cast<float>(sampleRate));
    }

    double sampleRate = 0.0;
    float frequency = 0.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    Waveform waveform = Waveform::sine;
};

inline BasicOscillator::Waveform waveformFromIndex(int index)
{
    switch (index)
    {
        case 0: return BasicOscillator::Waveform::sine;
        case 1: return BasicOscillator::Waveform::saw;
        case 2: return BasicOscillator::Waveform::square;
        case 3: return BasicOscillator::Waveform::triangle;
        default: break;
    }

    return BasicOscillator::Waveform::sine;
}

} // namespace auralis::dsp
