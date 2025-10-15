# Auralis Roadmap

## Phase 0 – Research & Foundations
- Survey language used by sound designers, create prompt lexicon.
- Compile dataset of wavetable patches with labeled descriptors.
- Define evaluation metrics: timbre similarity, user satisfaction, latency.

## Phase 1 – Prototype Services
- Build REST API that converts prompt → patch JSON using hosted LLM.
- Implement schema validator and fallback patch generator when LLM output fails.
- Create CLI that can audition patches via command line (using existing synth engine like Surge XT for testing).

## Phase 2 – Custom Synth Engine
- Implement minimal wavetable synth core in Rust with CLAP/VST3 wrapper.
- Expose engine parameters via gRPC or OSC for rapid iteration.
- Integrate modulation matrix and effects rack.

## Phase 3 – Integrated Plugin
- Develop JUCE-based UI with prompt console, chat history, macro controls, and spectral visualizer.
- Embed orchestrator runtime or connect to local daemon for low latency.
- Support saving/loading patches and exporting prompt history.

## Phase 4 – Intelligent Enhancements
- Introduce reinforcement learning from user feedback to improve LLM mapping.
- Add "explain this sound" feature that narrates how parameters shape the timbre.
- Enable collaborative modes: share prompt chains and patch variations.

## Phase 5 – Production Readiness
- Implement offline mode with distilled local language model.
- Harden security, telemetry opt-in, and privacy controls for cloud mode.
- Perform DAW compatibility testing (Ableton, Logic, FL Studio, Cubase, Reaper).
- Prepare documentation, tutorials, and marketing materials.
