#pragma once

#include <juce_core/juce_core.h>

namespace auralis::mapping
{
class PromptRuleParser
{
public:
    struct Result
    {
        juce::var patch;
        juce::StringArray matchedRules;
    };

    [[nodiscard]] Result parse(const juce::String& prompt) const;

private:
    struct PatchState
    {
        struct Oscillator
        {
            juce::String waveform { "sine" };
            float detuneCents { 0.0f };
            int semiOffset { 0 };
            float level { 0.5f };
        };

        Oscillator oscillators[2];

        juce::String filterType { "ladder_lp" };
        float filterCutoffHz { 1600.0f };
        float filterResonance { 0.2f };
        float filterDrive { 0.0f };

        float attackMs { 60.0f };
        float decayMs { 300.0f };
        float sustain { 0.75f };
        float releaseMs { 900.0f };

        bool lfoEnabled { false };
        float lfoRateHz { 0.2f };
        juce::String lfoShape { "sine" };
        float lfoDepth { 0.0f };

        float reverbMix { 0.18f };

        bool hasBpm { false };
        int bpm { 120 };

        [[nodiscard]] juce::var toVar() const;
    };

    [[nodiscard]] static PatchState createDefaultPatch();
    [[nodiscard]] static bool containsAny(const juce::String& haystack, std::initializer_list<const char*> needles);
};
} // namespace auralis::mapping
