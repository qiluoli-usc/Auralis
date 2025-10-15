#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace auralis::dsp
{
class Lfo
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        setFrequency(frequency);
        phase = 0.0f;
    }

    void setFrequency(float newFrequency)
    {
        frequency = juce::jmax(0.0f, newFrequency);
        if (sampleRate > 0.0)
            phaseIncrement = static_cast<float>(frequency / sampleRate);
    }

    [[nodiscard]] float getNextSample() noexcept
    {
        const auto currentPhase = phase;
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return std::sin(currentPhase * juce::MathConstants<float>::twoPi);
    }

private:
    double sampleRate = 0.0;
    float frequency = 0.5f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
};
} // namespace auralis::dsp
