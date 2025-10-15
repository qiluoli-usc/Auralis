"""Rule-based prompt mapper used by the FastAPI service."""

from __future__ import annotations

from typing import Dict, Iterable, Optional, Sequence

from codex_tests.offline_mapper import clamp, prompt_to_patch_offline


def _normalise_hint(hint: str) -> str:
    return hint.strip().lower()


def _apply_hint_overrides(patch: Dict, hints: Iterable[str]) -> None:
    """Apply additional deterministic tweaks for specific style hints."""

    normalised = {_normalise_hint(h) for h in hints if h}

    if "airy" in normalised:
        patch["filter"]["cutoff_hz"] = clamp(patch["filter"]["cutoff_hz"] + 800.0, 20.0, 20000.0)
        patch["fx"]["reverb"]["mix"] = clamp(patch["fx"]["reverb"]["mix"] + 0.1, 0.0, 1.0)

    if "noisy" in normalised:
        patch["oscillators"][1]["waveform"] = "noise"
        patch["oscillators"][1]["level"] = clamp(patch["oscillators"][1].get("level", 0.4) + 0.2, 0.0, 1.0)

    if "motion" in normalised or "movement" in normalised:
        lfo = patch.setdefault("lfo", [{"rate_hz": 0.4, "shape": "sine", "targets": [{"param": "filter.cutoff_hz", "depth": 0.15}]}])
        if lfo:
            lfo[0]["targets"][0]["depth"] = clamp(lfo[0]["targets"][0].get("depth", 0.15) + 0.1, 0.0, 1.0)
            lfo[0]["rate_hz"] = clamp(lfo[0].get("rate_hz", 0.4) + 0.1, 0.01, 30.0)


def map_prompt_to_patch(
    prompt: str,
    style_hints: Optional[Sequence[str]] = None,
    bpm: Optional[int] = None,
    seed: Optional[int] = None,
) -> Dict:
    """Map a prompt and optional modifiers to an AuralisPatch dictionary."""

    combined_prompt = prompt.strip()
    if style_hints:
        hints_text = " ".join(h.strip() for h in style_hints if h)
        if hints_text:
            combined_prompt = f"{combined_prompt} {hints_text}".strip()

    patch = prompt_to_patch_offline(combined_prompt)

    if style_hints:
        _apply_hint_overrides(patch, style_hints)

    meta = patch.setdefault("meta", {})
    meta["model"] = "auralis-fastapi-rule"

    if bpm is not None:
        meta["bpm"] = int(clamp(float(bpm), 20.0, 300.0))

    if seed is not None:
        meta["seed"] = int(seed)

    return patch


__all__ = ["map_prompt_to_patch"]
