# Auralis Architecture Notes

## High-Level Modules

| Module | Technology | Responsibilities |
| ------ | ---------- | ---------------- |
| Prompt Gateway | Electron/React + JUCE bridge | Collect prompts, provide chat-like UX, stream audio previews. |
| Orchestrator Service | Python (FastAPI) | Manage conversation state, call LLM, validate and post-process patch specs. |
| DSP Engine | Rust (VST3/AU/CLAP) | Interpret patch spec, produce audio in the DAW, expose parameter automation. |
| Knowledge Base | SQLite + embeddings | Store approved patches, user edits, and terminology mapping. |
| Analytics | Python notebooks | Evaluate quality, latency, and vocabulary coverage. |

## Data Flow

1. User submits a prompt via the UI.
2. Gateway sends structured request `{prompt, context, references}` to Orchestrator.
3. Orchestrator builds LLM messages with few-shot examples and schema constraints.
4. LLM responds with a patch JSON + rationale.
5. Schema validator enforces ranges, resolves wavetable IDs, and ensures modulation sanity.
6. DSP engine loads the patch and renders audio back to the gateway.
7. User edits or re-prompts; diffs and telemetry feed into the knowledge base for continual improvement.

## Patch Schema Draft

```json
{
  "meta": {
    "name": "Shimmer Pad",
    "tempo_sync": true,
    "llm_model": "gpt-5-codex"
  },
  "oscillators": [
    {"type": "wavetable", "table": "AiryPad01", "unison": 6, "detune": 0.12},
    {"type": "noise", "color": "pink", "level": 0.15}
  ],
  "filters": [
    {"type": "ladder_lp", "cutoff": 4800, "resonance": 0.35, "drive": 0.1}
  ],
  "envelopes": [
    {"target": "amp", "attack": 0.85, "decay": 1.8, "sustain": 0.7, "release": 2.5}
  ],
  "lfos": [
    {"shape": "sine", "rate_hz": 0.3, "depth": 0.2, "targets": ["filter.cutoff"]}
  ],
  "effects": [
    {"type": "chorus", "mix": 0.35},
    {"type": "reverb", "mix": 0.42}
  ],
  "macros": {
    "Motion": {
      "targets": ["lfos[0].depth", "effects[0].mix"],
      "range": [0.1, 0.6]
    }
  }
}
```

## LLM Prompt Strategy

* Few-shot prompts mapping descriptive language to patch snippets.
* Function-calling or JSON schema enforcement to guarantee valid output.
* Post-processing heuristics (clipping values, default fallbacks).
* Use reflection prompts to have the LLM critique and improve its own patch.

## Latency Targets

* UI to patch render: < 2 s for cloud LLM, < 500 ms with local distilled models.
* Audio engine block processing must stay within DAW buffer (64–256 samples).

## Risks & Mitigations

* **Ambiguous prompts** – Ask clarifying questions or generate multiple variants ranked by "confidence".
* **Model hallucinations** – Validate IDs, clamp parameter ranges, maintain curated wavetable library.
* **LLM cost/latency** – Cache prompts, enable on-prem models, support offline patch editing.
* **User trust** – Provide rationale and highlight key parameters so users can learn from generated patches.

## Next Steps

1. Finalize JSON schema and generate TypeScript/Rust bindings.
2. Build CLI prototype to test text → patch translation without UI overhead.
3. Integrate spectral analysis for reference audio alignment.
4. Draft UX mockups focusing on prompt panel + macro controls.
5. Define telemetry events (prompt, patch hash, playback duration, edits).
