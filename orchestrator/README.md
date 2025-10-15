# Auralis Orchestrator Prototype

This package contains a FastAPI application that exposes a `/generate_patch` endpoint. The
endpoint accepts a natural language prompt and returns a structured synthesizer patch following
the draft schema defined in `schemas/patch.schema.json`.

## Development

```bash
pip install -r requirements.txt
uvicorn orchestrator.app:app --reload
```

Visit http://localhost:8000/docs to interact with the auto-generated Swagger UI.

The current implementation uses lightweight keyword heuristics. Replacing
`prompt_engine.build_initial_patch` with LLM-backed logic is the next milestone.
