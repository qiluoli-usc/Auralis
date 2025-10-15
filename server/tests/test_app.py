from __future__ import annotations

import json
from pathlib import Path

from fastapi.testclient import TestClient
from jsonschema import validate

from server.app import app, build_few_shot_prompt

client = TestClient(app)
SCHEMA_PATH = Path(__file__).resolve().parents[2] / "schema" / "auralis.schema.json"


def load_schema() -> dict:
    with SCHEMA_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def test_map_endpoint_returns_valid_patch():
    response = client.post(
        "/map",
        json={"prompt": "warm pad", "styleHints": ["airy"], "bpm": 120},
    )
    assert response.status_code == 200
    patch = response.json()
    schema = load_schema()
    validate(instance=patch, schema=schema)
    assert patch["meta"]["model"] == "auralis-fastapi-rule"
    assert patch["meta"]["bpm"] == 120


def test_map_endpoint_uses_cache():
    request_payload = {"prompt": "bright pluck", "styleHints": ["motion"], "seed": 42}
    first = client.post("/map", json=request_payload)
    second = client.post("/map", json=request_payload)
    assert first.status_code == 200
    assert second.status_code == 200
    assert first.json() == second.json()


def test_build_few_shot_prompt_includes_template():
    prompt = build_few_shot_prompt("Deep bass", ["growl", "dark"])
    assert "SYSTEM:" in prompt
    assert "USER: Deep bass" in prompt
    assert "Style hints: growl, dark." in prompt
    assert prompt.strip().endswith("ASSISTANT:")
