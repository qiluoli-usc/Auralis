#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Param/ParamIDs.h"

namespace auralis::dsp
{
struct ParameterState
{
    void initialise(juce::AudioProcessorValueTreeState& state)
    {
        osc1Waveform = state.getRawParameterValue(auralis::params::osc1Waveform);
        osc2Waveform = state.getRawParameterValue(auralis::params::osc2Waveform);
        osc1DetuneCents = state.getRawParameterValue(auralis::params::osc1DetuneCents);
        osc2DetuneCents = state.getRawParameterValue(auralis::params::osc2DetuneCents);
        oscMix = state.getRawParameterValue(auralis::params::oscMix);
        filterCutoffHz = state.getRawParameterValue(auralis::params::filterCutoffHz);
        filterResonance = state.getRawParameterValue(auralis::params::filterResonance);
        envAttackMs = state.getRawParameterValue(auralis::params::envAttackMs);
        envDecayMs = state.getRawParameterValue(auralis::params::envDecayMs);
        envSustain = state.getRawParameterValue(auralis::params::envSustain);
        envReleaseMs = state.getRawParameterValue(auralis::params::envReleaseMs);
        lfoRateHz = state.getRawParameterValue(auralis::params::lfoRateHz);
        lfoDepth = state.getRawParameterValue(auralis::params::lfoDepthHz);
        reverbMix = state.getRawParameterValue(auralis::params::reverbMix);
    }

    [[nodiscard]] int getOscWaveformIndex(int index) const noexcept
    {
        const auto* ptr = index == 0 ? osc1Waveform : osc2Waveform;
        return ptr != nullptr ? juce::roundToInt(ptr->load()) : 0;
    }

    [[nodiscard]] float getOscDetuneCents(int index) const noexcept
    {
        const auto* ptr = index == 0 ? osc1DetuneCents : osc2DetuneCents;
        return ptr != nullptr ? ptr->load() : 0.0f;
    }

    [[nodiscard]] float getOscMix() const noexcept
    {
        return oscMix != nullptr ? oscMix->load() : 0.5f;
    }

    [[nodiscard]] float getFilterCutoffHz() const noexcept
    {
        return filterCutoffHz != nullptr ? filterCutoffHz->load() : 1000.0f;
    }

    [[nodiscard]] float getFilterResonance() const noexcept
    {
        return filterResonance != nullptr ? filterResonance->load() : 0.7f;
    }

    [[nodiscard]] juce::ADSR::Parameters getAmpEnvelopeParameters() const noexcept
    {
        juce::ADSR::Parameters params;
        params.attack = (envAttackMs != nullptr ? envAttackMs->load() : 10.0f) * 0.001f;
        params.decay = (envDecayMs != nullptr ? envDecayMs->load() : 120.0f) * 0.001f;
        params.sustain = envSustain != nullptr ? envSustain->load() : 0.75f;
        params.release = (envReleaseMs != nullptr ? envReleaseMs->load() : 250.0f) * 0.001f;
        return params;
    }

    [[nodiscard]] float getLfoRateHz() const noexcept
    {
        return lfoRateHz != nullptr ? lfoRateHz->load() : 2.0f;
    }

    [[nodiscard]] float getLfoDepthHz() const noexcept
    {
        return lfoDepth != nullptr ? lfoDepth->load() : 0.0f;
    }

    [[nodiscard]] float getReverbMix() const noexcept
    {
        return reverbMix != nullptr ? reverbMix->load() : 0.2f;
    }

    std::atomic<float>* osc1Waveform = nullptr;
    std::atomic<float>* osc2Waveform = nullptr;
    std::atomic<float>* osc1DetuneCents = nullptr;
    std::atomic<float>* osc2DetuneCents = nullptr;
    std::atomic<float>* oscMix = nullptr;
    std::atomic<float>* filterCutoffHz = nullptr;
    std::atomic<float>* filterResonance = nullptr;
    std::atomic<float>* envAttackMs = nullptr;
    std::atomic<float>* envDecayMs = nullptr;
    std::atomic<float>* envSustain = nullptr;
    std::atomic<float>* envReleaseMs = nullptr;
    std::atomic<float>* lfoRateHz = nullptr;
    std::atomic<float>* lfoDepth = nullptr;
    std::atomic<float>* reverbMix = nullptr;
};
} // namespace auralis::dsp
