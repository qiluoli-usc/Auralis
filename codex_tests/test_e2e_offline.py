from __future__ import annotations

import json
from pathlib import Path

import jsonschema

from codex_tests.metrics import rms, spectral_centroid
from codex_tests.offline_mapper import prompt_to_patch_offline
from codex_tests.renderer import SR, render_patch, save_wav

SCHEMA_PATH = Path(__file__).resolve().parents[1] / "schema" / "auralis.schema.json"
ARTIFACT_PATH = Path(__file__).resolve().parents[1] / "artifacts" / "warm_pad.wav"


def test_prompt_to_patch_renders_audio() -> None:
    prompt = "Warm pad with gentle movement"
    patch = prompt_to_patch_offline(prompt)

    schema = json.loads(SCHEMA_PATH.read_text())
    jsonschema.validate(patch, schema)

    audio = render_patch(patch, duration=2.0, frequency=220.0, sr=SR)
    assert len(audio) == int(2.0 * SR)

    level = rms(audio)
    centroid = spectral_centroid(audio, SR)

    assert 0.05 <= level <= 0.95
    assert 200.0 <= centroid <= 4000.0

    save_wav(str(ARTIFACT_PATH), audio, sr=SR)
    assert ARTIFACT_PATH.exists()
