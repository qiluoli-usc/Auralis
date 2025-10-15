# Auralis: LLM-Driven Sound Design Plugin

Auralis is an experimental audio plug-in concept that reimagines sound design by letting producers describe the timbre they want in natural language. A large language model (LLM) interprets the description, translates it into synthesizer parameters, and a modern wavetable/virtual-analog engine (inspired by tools like Serum) renders the audio in real time.

## Vision

* **Language-first sound creation** – Type "airy pad with shimmering harmonics and soft attack" and instantly audition the sound.
* **Explorable presets** – The LLM suggests variations, morph targets, and modulation ideas derived from the text prompt.
* **Deep manual control** – Generated patches remain editable with a familiar synth UI, providing a hybrid workflow between automation and craftsmanship.

## System Overview

```
Natural Language Prompt
        ↓
Prompt Understanding LLM
        ↓
Sound Design Graph Builder
        ↓
Synthesis Engine (Wavetable + Filters + FX)
        ↓
Audio Output / DAW Host
```

### Core Components

1. **Prompt Gateway**
   * Collects user requests (text, style tags, reference audio snippets).
   * Adds contextual metadata (project tempo, key, mood).
2. **LLM Orchestrator**
   * Uses prompt engineering and few-shot examples to map language into a structured patch schema.
   * Calls optional analytic tools (e.g., spectral analyzers) for reference matching.
3. **Patch Schema**
   * Defines oscillator settings, filter routing, modulation sources, effects, and macro assignments.
   * Versioned so patches are reproducible and shareable.
4. **Synthesis Engine**
   * Wavetable oscillators with morphing, unison, and FM options.
   * Multi-mode filters, flexible envelopes/LFOs, and studio-grade FX chain.
5. **Realtime Feedback Loop**
   * Render preview, capture user edits, and feed deltas back into the LLM to improve future suggestions.

## Key Workflows

### 1. Text → Patch
1. User enters a descriptive prompt.
2. LLM outputs a JSON patch spec following the schema.
3. Engine loads the patch, renders, and the user audition the sound.
4. User can refine via additional prompts ("more plucky", "add motion").

### 2. Reference Matching
1. User drops a short audio clip.
2. Feature extractor (e.g., MFCCs, spectral centroid) summarizes the clip.
3. LLM receives both language description and extracted features to approximate the sound.

### 3. Patch Evolution
1. User tweaks controls manually.
2. Engine diffs the changes against the generated patch.
3. LLM suggests names, macro mappings, or variations based on edits.

## Roadmap Highlights

* [ ] Define the patch schema and JSON format.
* [ ] Build a minimal wavetable synth core (Rust + VST3/AU).
* [ ] Implement the LLM prompt orchestration service (Python, FastAPI, OpenAI API or local model).
* [ ] Create UI mockups: prompt console, parameter panel, modulation matrix, macro pads.
* [ ] Prototype "describe the change" loop that translates follow-up prompts into parameter deltas.

## Repository Layout

```
Auralis/
├── synth/             # Rust crate with patch validation primitives
├── orchestrator/      # FastAPI service that maps prompts to patches
├── schemas/           # JSON schema definitions and sample patches
├── docs/              # Design notes, research, experiments
└── tests/             # (reserved) Audio regression + schema validation
```

### Getting Started

**Run the orchestration service**

```bash
cd orchestrator
pip install -r requirements.txt
uvicorn orchestrator.app:app --reload
```

Open <http://localhost:8000/docs> to try the `/generate_patch` endpoint.

**Validate a patch in Rust**

```bash
cd synth
cargo test
```

The Rust crate currently focuses on data modeling and validation. DSP routines
will be layered on once the patch contract stabilizes.

### Working on feature branches

Need to experiment on a separate branch or confirm tests before opening a PR?
Follow the [Testing & Branch Workflow](docs/testing.md) guide. It walks through
creating a branch, installing dependencies, and running the current checks so
that your work stays isolated until it is ready to merge.

## Research Directions

* Evaluate how well LLMs capture timbral terminology ("warm", "glassy", "swirly").
* Investigate integrating differentiable DSP or neural audio models for more organic responses.
* Explore few-shot learning with user-made patches to personalize the LLM output.
* Consider on-device inference optimizations or caching to reduce latency in DAWs.

---

Auralis aims to reduce the gap between inspiration and execution, empowering producers to focus on storytelling while retaining the depth of professional sound design tools.
