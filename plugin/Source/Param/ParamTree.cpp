#include "ParamTree.h"

#include "ParamIDs.h"

#include <memory>
#include <vector>

using namespace juce;

namespace
{
    std::unique_ptr<RangedAudioParameter> makeWaveformParameter()
    {
        StringArray choices { "Sine", "Saw", "Square", "Triangle" };
        return std::make_unique<AudioParameterChoice>(auralis::params::osc1Waveform,
                                                      "Osc 1 Waveform",
                                                      choices,
                                                      0);
    }

    std::unique_ptr<RangedAudioParameter> makeDetuneParameter()
    {
        NormalisableRange<float> range { -200.0f, 200.0f, 0.01f };
        range.setSkewForCentre(0.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::osc1DetuneCents,
                                                     "Osc 1 Detune (cents)",
                                                     range,
                                                     0.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeCutoffParameter()
    {
        NormalisableRange<float> range { 20.0f, 20000.0f, 0.01f };
        range.setSkewForCentre(1000.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::filterCutoffHz,
                                                     "Filter Cutoff (Hz)",
                                                     range,
                                                     1000.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeResonanceParameter()
    {
        NormalisableRange<float> range { 0.1f, 10.0f, 0.0f };
        range.setSkewForCentre(1.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::filterResonance,
                                                     "Filter Resonance",
                                                     range,
                                                     0.7f);
    }

    std::unique_ptr<RangedAudioParameter> makeAttackParameter()
    {
        NormalisableRange<float> range { 0.1f, 5000.0f, 0.01f };
        range.setSkewForCentre(50.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::envAttackMs,
                                                     "Amp Attack (ms)",
                                                     range,
                                                     10.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeDecayParameter()
    {
        NormalisableRange<float> range { 1.0f, 5000.0f, 0.01f };
        range.setSkewForCentre(100.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::envDecayMs,
                                                     "Amp Decay (ms)",
                                                     range,
                                                     120.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeSustainParameter()
    {
        NormalisableRange<float> range { 0.0f, 1.0f, 0.0f };
        return std::make_unique<AudioParameterFloat>(auralis::params::envSustain,
                                                     "Amp Sustain",
                                                     range,
                                                     0.75f);
    }

    std::unique_ptr<RangedAudioParameter> makeReleaseParameter()
    {
        NormalisableRange<float> range { 1.0f, 8000.0f, 0.01f };
        range.setSkewForCentre(200.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::envReleaseMs,
                                                     "Amp Release (ms)",
                                                     range,
                                                     250.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeLfoRateParameter()
    {
        NormalisableRange<float> range { 0.1f, 30.0f, 0.0f };
        range.setSkewForCentre(2.0f);
        return std::make_unique<AudioParameterFloat>(auralis::params::lfoRateHz,
                                                     "LFO Rate (Hz)",
                                                     range,
                                                     2.0f);
    }

    std::unique_ptr<RangedAudioParameter> makeReverbMixParameter()
    {
        NormalisableRange<float> range { 0.0f, 1.0f, 0.0f };
        return std::make_unique<AudioParameterFloat>(auralis::params::reverbMix,
                                                     "Reverb Mix",
                                                     range,
                                                     0.2f);
    }
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    params.reserve(10);

    params.push_back(makeWaveformParameter());
    params.push_back(makeDetuneParameter());
    params.push_back(makeCutoffParameter());
    params.push_back(makeResonanceParameter());
    params.push_back(makeAttackParameter());
    params.push_back(makeDecayParameter());
    params.push_back(makeSustainParameter());
    params.push_back(makeReleaseParameter());
    params.push_back(makeLfoRateParameter());
    params.push_back(makeReverbMixParameter());

    return { params.begin(), params.end() };
}
