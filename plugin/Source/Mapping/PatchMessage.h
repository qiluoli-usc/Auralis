#pragma once

namespace auralis::mapping
{
struct PatchMessage
{
    bool hasOscWaveform[2] { false, false };
    int oscWaveform[2] { 0, 0 };

    bool hasDetune[2] { false, false };
    float detuneCents[2] { 0.0f, 0.0f };

    bool hasOscMix { false };
    float oscMix { 0.5f };

    bool hasFilterCutoff { false };
    float filterCutoff { 1000.0f };

    bool hasFilterResonance { false };
    float filterResonance { 0.7f };

    bool hasEnvAttack { false };
    float envAttack { 10.0f };

    bool hasEnvDecay { false };
    float envDecay { 120.0f };

    bool hasEnvSustain { false };
    float envSustain { 0.75f };

    bool hasEnvRelease { false };
    float envRelease { 250.0f };

    bool hasLfoRate { false };
    float lfoRate { 2.0f };

    bool hasLfoDepth { false };
    float lfoDepth { 0.0f };

    bool hasReverbMix { false };
    float reverbMix { 0.2f };
};
}
