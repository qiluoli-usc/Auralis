#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

#include "Param/ParamIDs.h"

namespace auralis::utils
{
class MacroController : private juce::AudioProcessorValueTreeState::Listener,
                        private juce::AsyncUpdater
{
public:
    explicit MacroController(juce::AudioProcessorValueTreeState& state)
        : parameters(state)
    {
        initialise();
    }

    ~MacroController() override
    {
        for (auto* parameterID : macroParameterIDs)
            parameters.removeParameterListener(parameterID, this);
    }

private:
    void initialise()
    {
        for (size_t i = 0; i < macroValues.size(); ++i)
        {
            if (auto* parameter = parameters.getParameter(macroParameterIDs[i]))
                macroValues[i].store(parameter->getValue(), std::memory_order_relaxed);
            else
                macroValues[i].store(0.0f, std::memory_order_relaxed);

            parameters.addParameterListener(macroParameterIDs[i], this);
        }

    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        for (size_t i = 0; i < macroParameterIDs.size(); ++i)
        {
            if (parameterID == macroParameterIDs[i])
            {
                macroValues[i].store(newValue, std::memory_order_relaxed);
                triggerAsyncUpdate();
                break;
            }
        }
    }

    void handleAsyncUpdate() override
    {
        applyMacros();
    }

    void applyMacros()
    {
        const auto brightness = juce::jlimit(0.0f, 1.0f, macroValues[0].load(std::memory_order_relaxed));
        const auto movement = juce::jlimit(0.0f, 1.0f, macroValues[1].load(std::memory_order_relaxed));
        const auto atmosphere = juce::jlimit(0.0f, 1.0f, macroValues[2].load(std::memory_order_relaxed));

        const auto brightnessOffset = brightness - 0.5f;
        const auto movementOffset = movement - 0.5f;
        const auto atmosphereOffset = atmosphere - 0.5f;

        if (auto* cutoff = parameters.getParameter(auralis::params::filterCutoffHz))
        {
            const auto cutoffHz = juce::jlimit(200.0f, 16000.0f, 1000.0f + (brightnessOffset * 4800.0f));
            const auto normalised = cutoff->convertTo0to1(cutoffHz);
            cutoff->beginChangeGesture();
            cutoff->setValueNotifyingHost(normalised);
            cutoff->endChangeGesture();
        }

        if (auto* resonance = parameters.getParameter(auralis::params::filterResonance))
        {
            const auto resonanceValue = juce::jlimit(0.2f, 1.4f, 0.7f - (brightnessOffset * 0.5f));
            const auto normalised = resonance->convertTo0to1(resonanceValue);
            resonance->beginChangeGesture();
            resonance->setValueNotifyingHost(normalised);
            resonance->endChangeGesture();
        }

        if (auto* lfoDepth = parameters.getParameter(auralis::params::lfoDepthHz))
        {
            const auto depthHz = juce::jlimit(0.0f, 4000.0f, juce::jmap(movement, 0.0f, 1.0f, 0.0f, 2400.0f));
            const auto normalised = lfoDepth->convertTo0to1(depthHz);
            lfoDepth->beginChangeGesture();
            lfoDepth->setValueNotifyingHost(normalised);
            lfoDepth->endChangeGesture();
        }

        if (auto* lfoRate = parameters.getParameter(auralis::params::lfoRateHz))
        {
            const auto rateHz = juce::jlimit(0.1f, 8.0f, 2.0f + (movementOffset * 3.0f));
            const auto normalised = lfoRate->convertTo0to1(rateHz);
            lfoRate->beginChangeGesture();
            lfoRate->setValueNotifyingHost(normalised);
            lfoRate->endChangeGesture();
        }

        if (auto* reverbMix = parameters.getParameter(auralis::params::reverbMix))
        {
            const auto mix = juce::jlimit(0.05f, 0.65f, 0.2f + (atmosphereOffset * 0.3f));
            const auto normalised = reverbMix->convertTo0to1(mix);
            reverbMix->beginChangeGesture();
            reverbMix->setValueNotifyingHost(normalised);
            reverbMix->endChangeGesture();
        }

        if (auto* releaseMs = parameters.getParameter(auralis::params::envReleaseMs))
        {
            const auto release = juce::jlimit(80.0f, 6000.0f, 250.0f + (atmosphereOffset * 2500.0f));
            const auto normalised = releaseMs->convertTo0to1(release);
            releaseMs->beginChangeGesture();
            releaseMs->setValueNotifyingHost(normalised);
            releaseMs->endChangeGesture();
        }
    }

    juce::AudioProcessorValueTreeState& parameters;
    std::array<std::atomic<float>, 3> macroValues {};
    std::array<const juce::String, 3> macroParameterIDs {
        auralis::params::macroBrightness,
        auralis::params::macroMovement,
        auralis::params::macroAtmosphere
    };
};
} // namespace auralis::utils

