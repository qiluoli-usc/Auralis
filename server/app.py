"""FastAPI application bridging prompts to deterministic Auralis patches."""

from __future__ import annotations

import json
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse

from jsonschema import Draft7Validator

from server.rules.mapper import map_prompt_to_patch

REPO_ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = REPO_ROOT / "schema" / "auralis.schema.json"
TEMPLATE_PATH = Path(__file__).resolve().parent / "prompt_templates" / "llm.txt"

app = FastAPI(title="Auralis Prompt Mapper", version="0.1.0")


@dataclass
class MapRequest:
    prompt: str
    styleHints: Optional[List[str]] = None
    bpm: Optional[int] = None
    seed: Optional[int] = None

    @staticmethod
    def _normalise_hints(hints: Optional[List[str]]) -> Optional[List[str]]:
        if hints is None:
            return None
        if not isinstance(hints, list):
            raise HTTPException(status_code=400, detail="styleHints must be a list of strings")
        cleaned: List[str] = []
        for hint in hints:
            if not isinstance(hint, str):
                raise HTTPException(status_code=400, detail="styleHints entries must be strings")
            stripped = hint.strip()
            if stripped:
                cleaned.append(stripped)
        return cleaned or None

    @classmethod
    def from_dict(cls, payload: dict) -> "MapRequest":
        if not isinstance(payload, dict):
            raise HTTPException(status_code=400, detail="Request body must be an object")

        prompt = payload.get("prompt")
        if not isinstance(prompt, str) or not prompt.strip():
            raise HTTPException(status_code=400, detail="prompt must be a non-empty string")

        style_hints = cls._normalise_hints(payload.get("styleHints"))

        bpm_value = payload.get("bpm")
        bpm_int: Optional[int] = None
        if bpm_value is not None:
            if not isinstance(bpm_value, (int, float)):
                raise HTTPException(status_code=400, detail="bpm must be a number")
            bpm_int = int(bpm_value)
            if bpm_int < 20 or bpm_int > 300:
                raise HTTPException(status_code=400, detail="bpm out of range [20, 300]")

        seed_value = payload.get("seed")
        seed_int: Optional[int] = None
        if seed_value is not None:
            if not isinstance(seed_value, int):
                raise HTTPException(status_code=400, detail="seed must be an integer")
            seed_int = seed_value

        return cls(prompt=prompt.strip(), styleHints=style_hints, bpm=bpm_int, seed=seed_int)


@lru_cache(maxsize=1)
def _load_schema() -> Draft7Validator:
    with SCHEMA_PATH.open("r", encoding="utf-8") as handle:
        schema = json.load(handle)
    return Draft7Validator(schema)


@lru_cache(maxsize=1)
def _few_shot_template() -> str:
    return TEMPLATE_PATH.read_text(encoding="utf-8").strip()


def build_few_shot_prompt(prompt: str, style_hints: Optional[Sequence[str]] = None) -> str:
    """Return the system prompt containing few-shot examples followed by the user request."""

    template = _few_shot_template()
    hint_suffix = ""
    if style_hints:
        filtered = [hint.strip() for hint in style_hints if hint and hint.strip()]
        if filtered:
            hint_suffix = f" Style hints: {', '.join(filtered)}."

    user_line = f"USER: {prompt.strip()}{hint_suffix}"
    return f"{template}\n{user_line}\nASSISTANT:"


def _cache_key(request: MapRequest) -> Tuple:
    hints = tuple(h.strip().lower() for h in request.styleHints or [])
    return (request.prompt.strip().lower(), hints, request.bpm, request.seed)


_cache: dict[Tuple, dict] = {}


@app.post("/map")
def map_prompt(request: MapRequest) -> JSONResponse:
    """Map the incoming request to an AuralisPatch JSON object."""

    key = _cache_key(request)
    if key in _cache:
        return JSONResponse(_cache[key])

    patch = map_prompt_to_patch(request.prompt, request.styleHints, request.bpm, request.seed)

    validator = _load_schema()
    errors = sorted(validator.iter_errors(patch), key=lambda e: e.path)
    if errors:
        first = errors[0]
        raise HTTPException(status_code=400, detail=f"Schema validation failed: {'/'.join(str(p) for p in first.path)} {first.message}")

    serialisable = json.loads(json.dumps(patch))
    _cache[key] = serialisable
    return JSONResponse(serialisable)


__all__ = ["app", "build_few_shot_prompt"]
