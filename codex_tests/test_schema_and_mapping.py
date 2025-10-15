from __future__ import annotations

import json
from pathlib import Path

import jsonschema


SCHEMA_PATH = Path(__file__).resolve().parents[1] / "schema" / "auralis.schema.json"
EXAMPLE_PATCH = Path(__file__).resolve().parents[1] / "examples" / "patches" / "warm_pad.json"


def test_example_patch_matches_schema() -> None:
    schema = json.loads(SCHEMA_PATH.read_text())
    patch = json.loads(EXAMPLE_PATCH.read_text())
    jsonschema.validate(patch, schema)

    assert patch["oscillators"], "expected oscillators to be present"
    assert patch["filter"]["type"] in {"lp", "ladder_lp"}
    assert patch["env"]["amp"]["attack_ms"] >= 0
