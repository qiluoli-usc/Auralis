#include "JsonPatchApplier.h"

#include "Param/ParamIDs.h"

namespace auralis::mapping
{
namespace
{
    [[nodiscard]] float variantToFloat(const juce::var& value, float fallback) noexcept
    {
        if (value.isDouble() || value.isInt() || value.isInt64())
            return static_cast<float>(static_cast<double>(value));

        return fallback;
    }
}

JsonPatchApplier::JsonPatchApplier(juce::AudioProcessorValueTreeState& stateIn)
    : state(stateIn)
{
}

void JsonPatchApplier::apply(const juce::var& patchVar)
{
    if (! patchVar.isObject())
        return;

    if (auto* root = patchVar.getDynamicObject())
    {
        if (auto* oscArray = root->getProperty("oscillators").getArray())
            applyOscillators(*oscArray);

        applyFilter(root->getProperty("filter"));
        applyEnvelope(root->getProperty("env"));

        if (auto* lfoArray = root->getProperty("lfo").getArray())
            applyLfo(*lfoArray);

        applyFx(root->getProperty("fx"));
    }
}

void JsonPatchApplier::applyOscillators(const juce::Array<juce::var>& oscillators)
{
    float oscLevels[2] { 0.5f, 0.5f };
    bool levelDefined[2] { false, false };

    for (int i = 0; i < juce::jmin(2, oscillators.size()); ++i)
    {
        if (auto* oscObj = oscillators[i].getDynamicObject())
        {
            const auto waveformName = oscObj->getProperty("waveform").toString();
            if (waveformName.isNotEmpty())
                setChoiceParameter(i == 0 ? auralis::params::osc1Waveform : auralis::params::osc2Waveform,
                                   waveformIndexFromName(waveformName));

            if (oscObj->hasProperty("detune_cents"))
            {
                const auto detune = juce::jlimit(-200.0f, 200.0f, variantToFloat(oscObj->getProperty("detune_cents"), 0.0f));
                setFloatParameter(i == 0 ? auralis::params::osc1DetuneCents : auralis::params::osc2DetuneCents, detune);
            }

            if (oscObj->hasProperty("level"))
            {
                oscLevels[i] = juce::jlimit(0.0f, 1.0f, variantToFloat(oscObj->getProperty("level"), 0.5f));
                levelDefined[i] = true;
            }
        }
    }

    if (levelDefined[0] || levelDefined[1])
    {
        if (! levelDefined[0])
            oscLevels[0] = 1.0f - getCurrentFloatParameter(auralis::params::oscMix);

        if (! levelDefined[1])
            oscLevels[1] = getCurrentFloatParameter(auralis::params::oscMix);

        const auto total = juce::jmax(0.0001f, oscLevels[0] + oscLevels[1]);
        const auto mix = juce::jlimit(0.0f, 1.0f, oscLevels[1] / total);
        setFloatParameter(auralis::params::oscMix, mix);
    }
}

void JsonPatchApplier::applyFilter(const juce::var& filterVar)
{
    if (! filterVar.isObject())
        return;

    if (auto* filterObj = filterVar.getDynamicObject())
    {
        if (filterObj->hasProperty("cutoff_hz"))
        {
            const auto cutoff = juce::jlimit(20.0f, 20000.0f, variantToFloat(filterObj->getProperty("cutoff_hz"), 1000.0f));
            setFloatParameter(auralis::params::filterCutoffHz, cutoff);
        }

        if (filterObj->hasProperty("resonance"))
        {
            const auto resonanceNorm = juce::jlimit(0.0f, 1.0f, variantToFloat(filterObj->getProperty("resonance"), 0.2f));
            const auto mappedResonance = juce::jmap(resonanceNorm, 0.0f, 1.0f, 0.1f, 10.0f);
            setFloatParameter(auralis::params::filterResonance, mappedResonance);
        }
    }
}

void JsonPatchApplier::applyEnvelope(const juce::var& envVar)
{
    if (! envVar.isObject())
        return;

    if (auto* envObj = envVar.getDynamicObject())
    {
        const auto ampVar = envObj->getProperty("amp");
        if (! ampVar.isObject())
            return;

        if (auto* ampObj = ampVar.getDynamicObject())
        {
            if (ampObj->hasProperty("attack_ms"))
            {
                const auto attack = juce::jlimit(0.1f, 5000.0f, variantToFloat(ampObj->getProperty("attack_ms"), 10.0f));
                setFloatParameter(auralis::params::envAttackMs, attack);
            }

            if (ampObj->hasProperty("decay_ms"))
            {
                const auto decay = juce::jlimit(1.0f, 5000.0f, variantToFloat(ampObj->getProperty("decay_ms"), 120.0f));
                setFloatParameter(auralis::params::envDecayMs, decay);
            }

            if (ampObj->hasProperty("sustain"))
            {
                const auto sustain = juce::jlimit(0.0f, 1.0f, variantToFloat(ampObj->getProperty("sustain"), 0.8f));
                setFloatParameter(auralis::params::envSustain, sustain);
            }

            if (ampObj->hasProperty("release_ms"))
            {
                const auto release = juce::jlimit(1.0f, 8000.0f, variantToFloat(ampObj->getProperty("release_ms"), 250.0f));
                setFloatParameter(auralis::params::envReleaseMs, release);
            }
        }
    }
}

void JsonPatchApplier::applyLfo(const juce::Array<juce::var>& lfoArray)
{
    if (lfoArray.isEmpty())
        return;

    if (auto* lfoObj = lfoArray[0].getDynamicObject())
    {
        if (lfoObj->hasProperty("rate_hz"))
        {
            const auto rate = juce::jlimit(0.1f, 30.0f, variantToFloat(lfoObj->getProperty("rate_hz"), 2.0f));
            setFloatParameter(auralis::params::lfoRateHz, rate);
        }

        float depthNormalised = 0.0f;

        if (auto* targets = lfoObj->getProperty("targets").getArray())
        {
            for (const auto& targetVar : *targets)
            {
                if (auto* targetObj = targetVar.getDynamicObject())
                {
                    if (targetObj->getProperty("param").toString() == "filter.cutoff_hz")
                    {
                        depthNormalised = juce::jlimit(0.0f, 1.0f, variantToFloat(targetObj->getProperty("depth"), 0.0f));
                        break;
                    }
                }
            }
        }

        if (depthNormalised > 0.0f)
        {
            const auto mappedDepth = juce::jmap(depthNormalised, 0.0f, 1.0f, 0.0f, 4000.0f);
            setFloatParameter(auralis::params::lfoDepthHz, mappedDepth);
        }
    }
}

void JsonPatchApplier::applyFx(const juce::var& fxVar)
{
    if (! fxVar.isObject())
        return;

    if (auto* fxObj = fxVar.getDynamicObject())
    {
        const auto reverbVar = fxObj->getProperty("reverb");
        if (auto* reverbObj = reverbVar.getDynamicObject())
        {
            if (reverbObj->hasProperty("mix"))
            {
                const auto mix = juce::jlimit(0.0f, 1.0f, variantToFloat(reverbObj->getProperty("mix"), 0.2f));
                setFloatParameter(auralis::params::reverbMix, mix);
            }
        }
    }
}

int JsonPatchApplier::waveformIndexFromName(const juce::String& name)
{
    const auto lowercase = name.trim().toLowerCase();
    if (lowercase == "saw")
        return 1;
    if (lowercase == "square")
        return 2;
    if (lowercase == "triangle")
        return 3;

    return 0; // sine fallback
}

void JsonPatchApplier::setChoiceParameter(const juce::String& paramID, int choiceValue)
{
    juce::Value value = state.getParameterAsValue(paramID);
    value.setValue(choiceValue);
}

void JsonPatchApplier::setFloatParameter(const juce::String& paramID, float value)
{
    auto range = state.getParameterRange(paramID);
    const auto clamped = range.snapToLegalValue(juce::jlimit(range.start, range.end, value));
    juce::Value parameterValue = state.getParameterAsValue(paramID);
    parameterValue.setValue(clamped);
}

float JsonPatchApplier::getCurrentFloatParameter(const juce::String& paramID) const
{
    if (auto* raw = state.getRawParameterValue(paramID))
        return raw->load();

    return 0.0f;
}

} // namespace auralis::mapping
