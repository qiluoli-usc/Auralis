#include "PromptRuleParser.h"

namespace auralis::mapping
{
namespace
{
    void addRuleIfNeeded(juce::StringArray& matchedRules, const juce::String& label)
    {
        if (! matchedRules.contains(label))
            matchedRules.add(label);
    }

    [[nodiscard]] float clamp01(float value) noexcept
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }
}

juce::var PromptRuleParser::PatchState::toVar() const
{
    auto* root = new juce::DynamicObject();

    juce::Array<juce::var> oscArray;
    for (const auto& osc : oscillators)
    {
        auto* oscObj = new juce::DynamicObject();
        oscObj->setProperty("waveform", osc.waveform.toLowerCase());
        oscObj->setProperty("detune_cents", juce::jlimit(-100.0f, 100.0f, osc.detuneCents));
        oscObj->setProperty("semi_offset", juce::jlimit(-24, 24, osc.semiOffset));
        oscObj->setProperty("level", clamp01(osc.level));
        oscArray.add(juce::var(oscObj));
    }
    root->setProperty("oscillators", oscArray);

    auto* filterObj = new juce::DynamicObject();
    filterObj->setProperty("type", filterType);
    filterObj->setProperty("cutoff_hz", juce::jlimit(20.0f, 20000.0f, filterCutoffHz));
    filterObj->setProperty("resonance", clamp01(filterResonance));
    if (filterDrive > 0.0f)
        filterObj->setProperty("drive", clamp01(filterDrive));
    root->setProperty("filter", juce::var(filterObj));

    auto* ampObj = new juce::DynamicObject();
    ampObj->setProperty("attack_ms", juce::jlimit(0.0f, 5000.0f, attackMs));
    ampObj->setProperty("decay_ms", juce::jlimit(0.0f, 5000.0f, decayMs));
    ampObj->setProperty("sustain", clamp01(sustain));
    ampObj->setProperty("release_ms", juce::jlimit(0.0f, 10000.0f, releaseMs));

    auto* envObj = new juce::DynamicObject();
    envObj->setProperty("amp", juce::var(ampObj));
    root->setProperty("env", juce::var(envObj));

    juce::Array<juce::var> lfoArray;
    if (lfoEnabled && lfoDepth > 0.0f)
    {
        auto* lfoObj = new juce::DynamicObject();
        lfoObj->setProperty("rate_hz", juce::jlimit(0.01f, 30.0f, lfoRateHz));
        lfoObj->setProperty("shape", lfoShape);

        juce::Array<juce::var> targets;
        auto* targetObj = new juce::DynamicObject();
        targetObj->setProperty("param", "filter.cutoff_hz");
        targetObj->setProperty("depth", clamp01(lfoDepth));
        targets.add(juce::var(targetObj));
        lfoObj->setProperty("targets", targets);
        lfoArray.add(juce::var(lfoObj));
    }
    root->setProperty("lfo", lfoArray);

    auto* fxObj = new juce::DynamicObject();
    auto* reverbObj = new juce::DynamicObject();
    reverbObj->setProperty("mix", clamp01(reverbMix));
    fxObj->setProperty("reverb", juce::var(reverbObj));
    root->setProperty("fx", juce::var(fxObj));

    if (hasBpm)
    {
        auto* metaObj = new juce::DynamicObject();
        metaObj->setProperty("bpm", juce::jlimit(20, 300, bpm));
        root->setProperty("meta", juce::var(metaObj));
    }

    return juce::var(root);
}

PromptRuleParser::PatchState PromptRuleParser::createDefaultPatch()
{
    PatchState state;
    state.oscillators[0].waveform = "sine";
    state.oscillators[0].level = 0.65f;
    state.oscillators[1].waveform = "sine";
    state.oscillators[1].level = 0.55f;

    state.filterType = "ladder_lp";
    state.filterCutoffHz = 1500.0f;
    state.filterResonance = 0.2f;
    state.filterDrive = 0.0f;

    state.attackMs = 80.0f;
    state.decayMs = 220.0f;
    state.sustain = 0.75f;
    state.releaseMs = 600.0f;

    state.lfoEnabled = false;
    state.lfoRateHz = 0.2f;
    state.lfoShape = "sine";
    state.lfoDepth = 0.0f;

    state.reverbMix = 0.18f;

    state.hasBpm = false;
    state.bpm = 120;

    return state;
}

bool PromptRuleParser::containsAny(const juce::String& haystack, std::initializer_list<const char*> needles)
{
    for (auto* needle : needles)
    {
        if (haystack.containsIgnoreCase(needle))
            return true;
    }
    return false;
}

bool PromptRuleParser::containsAnyWord(const juce::StringArray& tokens, std::initializer_list<const char*> needles)
{
    for (const auto& token : tokens)
    {
        for (auto* needle : needles)
        {
            if (token == needle)
                return true;
        }
    }

    return false;
}

PromptRuleParser::Result PromptRuleParser::parse(const juce::String& prompt) const
{
    auto state = createDefaultPatch();
    Result result;

    const auto lowerPrompt = prompt.toLowerCase();

    juce::StringArray lowerTokens;
    lowerTokens.addTokens(prompt, " ,.;:\n\t-_/()", "");
    lowerTokens.removeEmptyStrings();
    for (auto& token : lowerTokens)
        token = token.toLowerCase();

    if (containsAnyWord(lowerTokens, { "warm", "warmer", "warmth" })
        || containsAny(lowerPrompt, { "warm", "lush", "buttery", "velvety", "cozy", "cosy" }))
    {
        state.oscillators[0].waveform = "saw";
        state.oscillators[1].waveform = "saw";
        state.oscillators[0].level = 0.7f;
        state.oscillators[1].level = 0.6f;
        state.filterCutoffHz = 1800.0f;
        state.filterResonance = juce::jmax(state.filterResonance, 0.24f);
        state.reverbMix = juce::jmax(state.reverbMix, 0.22f);
        addRuleIfNeeded(result.matchedRules, "warm tonality");
    }

    if (containsAny(lowerPrompt, { "analog", "analogue", "vintage" }))
    {
        state.oscillators[0].detuneCents = juce::jlimit(-100.0f, 100.0f, state.oscillators[0].detuneCents + 7.0f);
        state.oscillators[1].detuneCents = juce::jlimit(-100.0f, 100.0f, state.oscillators[1].detuneCents - 6.0f);
        state.filterDrive = juce::jmax(state.filterDrive, 0.18f);
        state.lfoEnabled = true;
        state.lfoDepth = juce::jmax(state.lfoDepth, 0.12f);
        state.lfoRateHz = 0.28f;
        addRuleIfNeeded(result.matchedRules, "analog character");
    }

    if (containsAnyWord(lowerTokens, { "pad", "pads" })
        || containsAny(lowerPrompt, { "pad", "swell", "wash", "bed", "drape", "blanket" }))
    {
        state.attackMs = juce::jmax(state.attackMs, 900.0f);
        state.decayMs = juce::jmax(state.decayMs, 600.0f);
        state.sustain = juce::jmax(state.sustain, 0.8f);
        state.releaseMs = juce::jmax(state.releaseMs, 1600.0f);
        addRuleIfNeeded(result.matchedRules, "pad envelope");
    }

    if (lowerPrompt.contains("gentle movement") || lowerPrompt.contains("gentle motion")
        || lowerPrompt.contains("slow movement") || lowerPrompt.contains("subtle movement"))
    {
        state.lfoEnabled = true;
        state.lfoDepth = juce::jmax(state.lfoDepth, 0.18f);
        state.lfoRateHz = 0.3f;
        addRuleIfNeeded(result.matchedRules, "gentle movement");
    }

    if (containsAny(lowerPrompt, { "lowpass", "low-pass" }))
    {
        state.filterType = "ladder_lp";
        state.filterCutoffHz = juce::jlimit(20.0f, 20000.0f, lowerPrompt.contains("open") ? 2000.0f : 1400.0f);
        addRuleIfNeeded(result.matchedRules, "lowpass emphasis");
    }

    if (containsAny(lowerPrompt, { "long release", "slow release", "extended release" }))
    {
        state.releaseMs = juce::jlimit(0.0f, 10000.0f, 2200.0f);
        addRuleIfNeeded(result.matchedRules, "long release");
    }

    if (containsAnyWord(lowerTokens, { "bright", "brighter", "brightness" })
        || containsAny(lowerPrompt, { "bright", "shimmer", "sparkle", "sparkly", "glassy", "brilliant" }))
    {
        state.filterCutoffHz = juce::jmax(state.filterCutoffHz, 3600.0f);
        state.filterResonance = juce::jmin(0.5f, juce::jmax(state.filterResonance, 0.28f));
        addRuleIfNeeded(result.matchedRules, "brightness");
    }

    if (containsAny(lowerPrompt, { "dark", "mellow", "muted" }))
    {
        state.filterCutoffHz = juce::jmin(state.filterCutoffHz, 900.0f);
        state.filterResonance = juce::jmax(state.filterResonance, 0.2f);
        addRuleIfNeeded(result.matchedRules, "dark tone");
    }

    if (containsAnyWord(lowerTokens, { "pluck", "plucks", "plucked", "plucky" })
        || containsAny(lowerPrompt, { "plucky", "snappy", "percussive", "spiky", "stab" }))
    {
        state.attackMs = juce::jmin(state.attackMs, 40.0f);
        state.decayMs = juce::jmin(state.decayMs, 200.0f);
        state.releaseMs = juce::jmin(state.releaseMs, 300.0f);
        state.sustain = juce::jmin(state.sustain, 0.3f);
        addRuleIfNeeded(result.matchedRules, "plucked articulation");
    }

    for (int i = 0; i < lowerTokens.size(); ++i)
    {
        const auto& tokenLower = lowerTokens.getReference(i);
        if (tokenLower == "bpm" && i > 0)
        {
            const int bpmValue = lowerTokens[i - 1].getIntValue();
            if (bpmValue >= 20 && bpmValue <= 300)
            {
                state.hasBpm = true;
                state.bpm = bpmValue;
                addRuleIfNeeded(result.matchedRules, "tempo");
            }
        }
        else if (tokenLower.endsWith("bpm"))
        {
            const auto number = tokenLower.upToFirstOccurrenceOf("bpm", false, false);
            const int bpmValue = number.getIntValue();
            if (bpmValue >= 20 && bpmValue <= 300)
            {
                state.hasBpm = true;
                state.bpm = bpmValue;
                addRuleIfNeeded(result.matchedRules, "tempo");
            }
        }
    }

    if (state.lfoDepth > 0.0f)
        state.lfoEnabled = true;

    result.patch = state.toVar();
    return result;
}

} // namespace auralis::mapping
