"""Pydantic models for the orchestration service."""

from __future__ import annotations

from typing import Any, Dict, Optional

from pydantic import BaseModel, Field


class GeneratePatchRequest(BaseModel):
    """Input payload for requesting a patch generation."""

    prompt: str = Field(..., description="Natural language description of the desired sound")
    context: Optional[Dict[str, Any]] = Field(
        default=None,
        description="Optional session context such as tempo, key, or reference tags.",
    )


class PatchEnvelope(BaseModel):
    """Simplified representation of an amplitude envelope."""

    attack: float = Field(..., ge=0.0)
    decay: float = Field(..., ge=0.0)
    sustain: float = Field(..., ge=0.0, le=1.0)
    release: float = Field(..., ge=0.0)


class OscillatorSettings(BaseModel):
    """Oscillator configuration subset."""

    wavetable: str = Field(..., description="Name of the wavetable to load")
    octave: int = Field(0)
    detune: float = Field(0.0)
    blend: float = Field(0.5, ge=0.0, le=1.0)


class Patch(BaseModel):
    """Minimal synthesizer patch description."""

    name: str
    oscillators: list[OscillatorSettings]
    envelope: PatchEnvelope
    notes: Optional[str] = None


class GeneratePatchResponse(BaseModel):
    """Response payload returned to the client."""

    patch: Patch
