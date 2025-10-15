#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace auralis::params
{
// Oscillator
inline constexpr auto osc1Waveform = "osc1.waveform";
inline constexpr auto osc1DetuneCents = "osc1.detune_cents";
inline constexpr auto osc2Waveform = "osc2.waveform";
inline constexpr auto osc2DetuneCents = "osc2.detune_cents";
inline constexpr auto oscMix = "osc.mix";

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
inline constexpr auto lfoDepthHz = "lfo1.depth_hz";

// FX
inline constexpr auto reverbMix = "fx.reverb.mix";

// Macros
inline constexpr auto macroBrightness = "macro.brightness";
inline constexpr auto macroMovement = "macro.movement";
inline constexpr auto macroAtmosphere = "macro.atmosphere";

inline constexpr auto parameterGroup = "AuralisParameters";
} // namespace auralis::params

inline juce::String getParameterName(const juce::String& paramID)
{
    if (paramID == auralis::params::osc1Waveform) return "Osc 1 Waveform";
    if (paramID == auralis::params::osc1DetuneCents) return "Osc 1 Detune (cents)";
    if (paramID == auralis::params::osc2Waveform) return "Osc 2 Waveform";
    if (paramID == auralis::params::osc2DetuneCents) return "Osc 2 Detune (cents)";
    if (paramID == auralis::params::oscMix) return "Osc Mix";
    if (paramID == auralis::params::filterCutoffHz) return "Filter Cutoff (Hz)";
    if (paramID == auralis::params::filterResonance) return "Filter Resonance";
    if (paramID == auralis::params::envAttackMs) return "Amp Attack (ms)";
    if (paramID == auralis::params::envDecayMs) return "Amp Decay (ms)";
    if (paramID == auralis::params::envSustain) return "Amp Sustain";
    if (paramID == auralis::params::envReleaseMs) return "Amp Release (ms)";
    if (paramID == auralis::params::lfoRateHz) return "LFO Rate (Hz)";
    if (paramID == auralis::params::lfoDepthHz) return "LFO Depth (Hz)";
    if (paramID == auralis::params::reverbMix) return "Reverb Mix";
    if (paramID == auralis::params::macroBrightness) return "Macro A Brightness";
    if (paramID == auralis::params::macroMovement) return "Macro B Movement";
    if (paramID == auralis::params::macroAtmosphere) return "Macro C Atmosphere";

    return paramID;
}
