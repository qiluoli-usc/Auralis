"""Offline prompt-to-patch mapper for deterministic testing."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict


def clamp(value: float, minimum: float, maximum: float) -> float:
    """Clamp *value* to the inclusive [minimum, maximum] range."""
    return max(minimum, min(maximum, value))


@dataclass
class EnvelopeSettings:
    attack_ms: float
    decay_ms: float
    sustain: float
    release_ms: float


def _base_patch() -> Dict:
    return {
        "oscillators": [
            {"waveform": "saw", "detune_cents": 0.0, "semi_offset": 0, "level": 0.8},
            {"waveform": "sine", "detune_cents": -2.0, "semi_offset": 0, "level": 0.5},
        ],
        "filter": {"type": "lp", "cutoff_hz": 1200.0, "resonance": 0.25, "drive": 0.05},
        "env": {
            "amp": {
                "attack_ms": 15.0,
                "decay_ms": 180.0,
                "sustain": 0.65,
                "release_ms": 320.0,
            }
        },
        "lfo": [
            {
                "rate_hz": 0.5,
                "shape": "sine",
                "targets": [
                    {"param": "filter.cutoff_hz", "depth": 0.0},
                ],
            }
        ],
        "fx": {"reverb": {"mix": 0.12}},
        "meta": {"bpm": 100, "seed": 0, "model": "offline-rule", "temperature": 0.5},
    }


def _set_envelope(patch: Dict, settings: EnvelopeSettings) -> None:
    env = patch["env"]["amp"]
    env["attack_ms"] = clamp(settings.attack_ms, 0.0, 5000.0)
    env["decay_ms"] = clamp(settings.decay_ms, 0.0, 5000.0)
    env["sustain"] = clamp(settings.sustain, 0.0, 1.0)
    env["release_ms"] = clamp(settings.release_ms, 0.0, 10000.0)


def _apply_pad(patch: Dict) -> None:
    _set_envelope(
        patch,
        EnvelopeSettings(attack_ms=900.0, decay_ms=650.0, sustain=0.8, release_ms=1800.0),
    )
    patch["lfo"][0]["targets"][0]["depth"] = 0.22
    patch["lfo"][0]["rate_hz"] = 0.35
    patch["fx"]["reverb"]["mix"] = clamp(patch["fx"]["reverb"]["mix"] + 0.15, 0.0, 1.0)


def _apply_pluck(patch: Dict) -> None:
    _set_envelope(
        patch,
        EnvelopeSettings(attack_ms=5.0, decay_ms=120.0, sustain=0.2, release_ms=180.0),
    )
    patch["lfo"][0]["targets"][0]["depth"] = 0.05
    patch["lfo"][0]["rate_hz"] = 2.0
    patch["fx"]["reverb"]["mix"] = clamp(patch["fx"]["reverb"]["mix"] + 0.05, 0.0, 1.0)


def _apply_bass(patch: Dict) -> None:
    for osc in patch["oscillators"]:
        osc["semi_offset"] = -12
        osc["detune_cents"] = clamp(osc.get("detune_cents", 0.0), -30.0, 30.0)
    patch["filter"]["cutoff_hz"] = clamp(400.0, 20.0, 20000.0)
    patch["filter"]["resonance"] = clamp(patch["filter"]["resonance"] + 0.1, 0.0, 1.0)
    patch["meta"]["bpm"] = 128


def prompt_to_patch_offline(prompt: str) -> Dict:
    """Map a natural-language *prompt* to a deterministic patch dictionary."""
    prompt_lc = prompt.lower()
    tokens = prompt_lc.split()

    patch = _base_patch()
    patch["meta"]["seed"] = int(abs(hash(prompt_lc)) % (2**32))

    if "bright" in tokens or "sparkling" in prompt_lc:
        patch["filter"]["cutoff_hz"] = clamp(patch["filter"]["cutoff_hz"] + 2500.0, 20.0, 20000.0)
        patch["filter"]["resonance"] = clamp(patch["filter"]["resonance"] - 0.05, 0.0, 1.0)

    if "warm" in tokens or "buttery" in prompt_lc:
        patch["filter"]["cutoff_hz"] = clamp(patch["filter"]["cutoff_hz"] - 500.0, 20.0, 20000.0)
        patch["filter"]["drive"] = clamp(patch["filter"].get("drive", 0.0) + 0.2, 0.0, 1.0)

    if "pad" in tokens:
        _apply_pad(patch)

    if "pluck" in tokens or "plucky" in prompt_lc:
        _apply_pluck(patch)

    if "bass" in tokens:
        _apply_bass(patch)

    # keep oscillator levels in range
    for osc in patch["oscillators"]:
        osc["level"] = clamp(osc.get("level", 1.0), 0.0, 1.0)
        osc["detune_cents"] = clamp(osc.get("detune_cents", 0.0), -100.0, 100.0)
        osc["semi_offset"] = int(clamp(float(osc.get("semi_offset", 0)), -24.0, 24.0))

    patch["filter"]["drive"] = clamp(patch["filter"].get("drive", 0.0), 0.0, 1.0)
    patch["filter"]["cutoff_hz"] = clamp(patch["filter"].get("cutoff_hz", 1000.0), 20.0, 20000.0)
    patch["filter"]["resonance"] = clamp(patch["filter"].get("resonance", 0.2), 0.0, 1.0)

    reverb = patch["fx"].get("reverb", {"mix": 0.0})
    reverb["mix"] = clamp(reverb.get("mix", 0.0), 0.0, 1.0)
    patch["fx"]["reverb"] = reverb

    patch["meta"]["bpm"] = int(clamp(float(patch["meta"].get("bpm", 120)), 20.0, 300.0))
    patch["meta"]["temperature"] = clamp(patch["meta"].get("temperature", 0.5), 0.0, 2.0)

    return patch


__all__ = ["prompt_to_patch_offline", "clamp"]
