#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace auralis::params
{
// Oscillator
inline constexpr auto osc1Waveform = "osc1.waveform";
inline constexpr auto osc1DetuneCents = "osc1.detune_cents";

// Filter
inline constexpr auto filterCutoffHz = "filter.cutoff_hz";
inline constexpr auto filterResonance = "filter.resonance";

// Amp envelope
inline constexpr auto envAttackMs = "env.amp.attack_ms";
inline constexpr auto envDecayMs = "env.amp.decay_ms";
inline constexpr auto envSustain = "env.amp.sustain";
inline constexpr auto envReleaseMs = "env.amp.release_ms";

// LFO
inline constexpr auto lfoRateHz = "lfo1.rate_hz";

// FX
inline constexpr auto reverbMix = "fx.reverb.mix";

inline constexpr auto parameterGroup = "AuralisParameters";
} // namespace auralis::params

inline juce::String getParameterName(const juce::String& paramID)
{
    if (paramID == auralis::params::osc1Waveform) return "Osc 1 Waveform";
    if (paramID == auralis::params::osc1DetuneCents) return "Osc 1 Detune (cents)";
    if (paramID == auralis::params::filterCutoffHz) return "Filter Cutoff (Hz)";
    if (paramID == auralis::params::filterResonance) return "Filter Resonance";
    if (paramID == auralis::params::envAttackMs) return "Amp Attack (ms)";
    if (paramID == auralis::params::envDecayMs) return "Amp Decay (ms)";
    if (paramID == auralis::params::envSustain) return "Amp Sustain";
    if (paramID == auralis::params::envReleaseMs) return "Amp Release (ms)";
    if (paramID == auralis::params::lfoRateHz) return "LFO Rate (Hz)";
    if (paramID == auralis::params::reverbMix) return "Reverb Mix";

    return paramID;
}
