from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from codex_tests.metrics import rms, spectral_centroid
from codex_tests.offline_mapper import prompt_to_patch_offline
from codex_tests.renderer import SR, render_patch, save_wav

ARTIFACT_PATH = Path(__file__).resolve().parents[1] / "artifacts" / "warm_pad.wav"


def main() -> None:
    prompt = "Warm pad with gentle movement"
    patch = prompt_to_patch_offline(prompt)

    print("Prompt:", prompt)
    print("Patch:")
    print(json.dumps(patch, indent=2))

    audio = render_patch(patch, duration=2.0, frequency=220.0, sr=SR)
    save_wav(str(ARTIFACT_PATH), audio, sr=SR)

    print(f"Saved WAV to {ARTIFACT_PATH}")
    print(f"RMS: {rms(audio):.4f}")
    print(f"Spectral centroid: {spectral_centroid(audio, SR):.2f} Hz")


if __name__ == "__main__":
    main()
