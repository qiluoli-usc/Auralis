//! Prototype DSP core for the Auralis sound design plugin.
//!
//! The goal of this crate is to expose minimal data structures and helpers for
//! loading synthesizer patches produced by the orchestration service. DSP
//! routines will evolve over time; for now we provide light-weight parsing and
//! validation logic so integration tests can begin immediately.

use serde::{Deserialize, Serialize};
use thiserror::Error;

/// Representation of an oscillator in the synthesis engine.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Oscillator {
    pub wavetable: String,
    #[serde(default)]
    pub octave: i8,
    #[serde(default)]
    pub detune: f32,
    #[serde(default = "default_blend")]
    pub blend: f32,
}

fn default_blend() -> f32 {
    0.5
}

/// Basic ADSR envelope structure.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Envelope {
    pub attack: f32,
    pub decay: f32,
    pub sustain: f32,
    pub release: f32,
}

/// High-level patch data shared between the LLM orchestrator and DSP core.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Patch {
    pub name: String,
    pub oscillators: Vec<Oscillator>,
    pub envelope: Envelope,
    #[serde(default)]
    pub notes: Option<String>,
}

/// Errors that can occur when validating a patch.
#[derive(Debug, Error)]
pub enum PatchValidationError {
    #[error("patch JSON could not be parsed: {0}")]
    InvalidFormat(String),
    #[error("patch must define at least one oscillator")]
    MissingOscillators,
    #[error("oscillator blend {0} is out of range (0.0..=1.0)")]
    InvalidBlend(f32),
    #[error("envelope sustain {0} is out of range (0.0..=1.0)")]
    InvalidSustain(f32),
}

impl Patch {
    /// Validate fundamental constraints that the DSP engine relies upon.
    pub fn validate(&self) -> Result<(), PatchValidationError> {
        if self.oscillators.is_empty() {
            return Err(PatchValidationError::MissingOscillators);
        }

        for osc in &self.oscillators {
            if !(0.0..=1.0).contains(&osc.blend) {
                return Err(PatchValidationError::InvalidBlend(osc.blend));
            }
        }

        if !(0.0..=1.0).contains(&self.envelope.sustain) {
            return Err(PatchValidationError::InvalidSustain(self.envelope.sustain));
        }

        Ok(())
    }
}

/// Load a patch from JSON and immediately validate it.
pub fn load_patch_from_json(json: &str) -> Result<Patch, PatchValidationError> {
    let patch: Patch = serde_json::from_str(json)
        .map_err(|err| PatchValidationError::InvalidFormat(err.to_string()))?;
    patch.validate()?;
    Ok(patch)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validates_patch() {
        let json = r#"{
            "name": "Test",
            "oscillators": [
                {"wavetable": "InitSaw", "blend": 0.5}
            ],
            "envelope": {"attack": 0.1, "decay": 0.2, "sustain": 0.8, "release": 0.3}
        }"#;

        let patch = load_patch_from_json(json).expect("valid patch");
        assert_eq!(patch.oscillators.len(), 1);
    }

    #[test]
    fn rejects_invalid_blend() {
        let json = r#"{
            "name": "Bad Blend",
            "oscillators": [
                {"wavetable": "InitSaw", "blend": 1.5}
            ],
            "envelope": {"attack": 0.1, "decay": 0.2, "sustain": 0.5, "release": 0.3}
        }"#;

        let err = load_patch_from_json(json).unwrap_err();
        assert!(matches!(err, PatchValidationError::InvalidBlend(_)));
    }

    #[test]
    fn rejects_malformed_json() {
        let err = load_patch_from_json("not json").unwrap_err();
        assert!(matches!(err, PatchValidationError::InvalidFormat(_)));
    }
}
