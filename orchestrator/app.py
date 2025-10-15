"""FastAPI application for the Auralis orchestration service."""

from fastapi import FastAPI

from .models import GeneratePatchRequest, GeneratePatchResponse
from .prompt_engine import build_initial_patch

app = FastAPI(
    title="Auralis Orchestrator",
    description=(
        "Prototype service that converts natural language prompts into "
        "structured synthesizer patches."
    ),
    version="0.1.0",
)


@app.get("/health", tags=["system"])
def health() -> dict[str, str]:
    """Simple health check endpoint."""
    return {"status": "ok"}


@app.post("/generate_patch", response_model=GeneratePatchResponse, tags=["patches"])
def generate_patch(payload: GeneratePatchRequest) -> GeneratePatchResponse:
    """Generate a draft patch based on the supplied language prompt."""
    patch = build_initial_patch(payload.prompt, payload.context)
    return GeneratePatchResponse(patch=patch)
