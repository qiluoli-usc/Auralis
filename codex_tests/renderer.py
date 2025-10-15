"""Offline renderer that turns Auralis patches into audio buffers."""
from __future__ import annotations

import math
import random
import wave
from array import array
from typing import Dict, List

SR = 48_000
_TARGET_PEAK = 10 ** (-1.0 / 20.0)  # ~ -1 dBFS


def _clamp(value: float, minimum: float, maximum: float) -> float:
    return max(minimum, min(maximum, value))


def _generate_waveform_sample(waveform: str, phase: float, rng: random.Random) -> float:
    waveform = waveform.lower()
    if waveform == "sine":
        return math.sin(phase)
    if waveform == "saw":
        return 2.0 * (phase / (2 * math.pi) - math.floor(phase / (2 * math.pi) + 0.5))
    if waveform == "square":
        return 1.0 if math.sin(phase) >= 0.0 else -1.0
    if waveform == "triangle":
        # triangle from saw
        saw = 2.0 * (phase / (2 * math.pi) - math.floor(phase / (2 * math.pi) + 0.5))
        return 2.0 * abs(saw) - 1.0
    if waveform == "noise":
        return rng.uniform(-1.0, 1.0)
    return math.sin(phase)


def _generate_lfo(shape: str, rate_hz: float, num_samples: int, sr: int, rng: random.Random) -> List[float]:
    values: List[float] = [0.0] * num_samples
    phase = 0.0
    phase_inc = 2.0 * math.pi * rate_hz / sr
    if shape == "random":
        hold_samples = max(1, int(sr / max(rate_hz, 1e-6)))
        current = rng.uniform(-1.0, 1.0)
        for i in range(num_samples):
            if i % hold_samples == 0:
                current = rng.uniform(-1.0, 1.0)
            values[i] = current
        return values

    for i in range(num_samples):
        if shape == "triangle":
            pos = (phase / (2 * math.pi)) % 1.0
            values[i] = 2.0 * abs(2.0 * pos - 1.0) - 1.0
        elif shape == "saw":
            pos = (phase / (2 * math.pi)) % 1.0
            values[i] = 2.0 * pos - 1.0
        elif shape == "square":
            values[i] = 1.0 if math.sin(phase) >= 0.0 else -1.0
        else:  # sine fallback
            values[i] = math.sin(phase)
        phase += phase_inc
    return values


class ADSR:
    """Simple ADSR envelope with linear segments."""

    def __init__(self, attack_ms: float, decay_ms: float, sustain: float, release_ms: float, sr: int) -> None:
        self.attack_samples = int(max(0.0, attack_ms) * sr / 1000.0)
        self.decay_samples = int(max(0.0, decay_ms) * sr / 1000.0)
        self.release_samples = int(max(0.0, release_ms) * sr / 1000.0)
        self.sustain = _clamp(sustain, 0.0, 1.0)
        self.sr = sr

    def render(self, total_samples: int) -> List[float]:
        env = [0.0] * total_samples
        release_start = max(total_samples - self.release_samples, 0)

        attack_end = min(self.attack_samples, total_samples)
        for i in range(attack_end):
            env[i] = i / max(1, self.attack_samples)

        if self.attack_samples == 0 and total_samples > 0:
            env[0] = 1.0

        decay_end = min(attack_end + self.decay_samples, total_samples)
        start_level = env[attack_end - 1] if attack_end > 0 else 1.0
        decay_length = max(1, decay_end - attack_end)
        for idx, i in enumerate(range(attack_end, decay_end)):
            env[i] = start_level + (self.sustain - start_level) * (idx / decay_length)

        sustain_start = decay_end
        for i in range(sustain_start, release_start):
            env[i] = self.sustain

        if self.release_samples > 0 and release_start < total_samples:
            start_level = env[release_start - 1] if release_start > 0 else self.sustain
            for idx, i in enumerate(range(release_start, total_samples)):
                env[i] = start_level * (1.0 - (idx / max(1, self.release_samples)))
        elif release_start < total_samples:
            for i in range(release_start, total_samples):
                env[i] = env[release_start - 1] if release_start > 0 else self.sustain

        return env


class BiquadLPF:
    """Direct-form I biquad low-pass filter."""

    def __init__(self, sr: int) -> None:
        self.sr = sr
        self.b0 = 1.0
        self.b1 = 0.0
        self.b2 = 0.0
        self.a1 = 0.0
        self.a2 = 0.0
        self.x1 = 0.0
        self.x2 = 0.0
        self.y1 = 0.0
        self.y2 = 0.0

    def update_coefficients(self, cutoff_hz: float, resonance: float) -> None:
        cutoff = _clamp(cutoff_hz, 20.0, self.sr / 2.0 - 100.0)
        q = 0.5 + resonance * 4.5
        w0 = 2.0 * math.pi * cutoff / self.sr
        cos_w0 = math.cos(w0)
        sin_w0 = math.sin(w0)
        alpha = sin_w0 / (2.0 * max(q, 1e-6))

        b0 = (1 - cos_w0) / 2.0
        b1 = 1 - cos_w0
        b2 = (1 - cos_w0) / 2.0
        a0 = 1 + alpha
        a1 = -2 * cos_w0
        a2 = 1 - alpha

        self.b0 = b0 / a0
        self.b1 = b1 / a0
        self.b2 = b2 / a0
        self.a1 = a1 / a0
        self.a2 = a2 / a0

    def process(self, data: List[float], start: int, end: int) -> None:
        for i in range(start, end):
            x0 = data[i]
            y0 = (
                self.b0 * x0
                + self.b1 * self.x1
                + self.b2 * self.x2
                - self.a1 * self.y1
                - self.a2 * self.y2
            )
            data[i] = y0
            self.x2 = self.x1
            self.x1 = x0
            self.y2 = self.y1
            self.y1 = y0


def _apply_drive(signal: List[float], drive: float) -> List[float]:
    if drive <= 0.0:
        return list(signal)
    amount = _clamp(drive, 0.0, 1.0)
    factor = 1.0 + 3.0 * amount
    return [math.tanh(sample * factor) for sample in signal]


def _apply_reverb(signal: List[float], mix: float, sr: int) -> List[float]:
    mix = _clamp(mix, 0.0, 1.0)
    if mix <= 1e-6:
        return signal
    delay_samples = max(1, int(0.1 * sr))
    buffer = [0.0] * delay_samples
    out = [0.0] * len(signal)
    feedback = 0.35 + 0.4 * mix
    damp = 0.5
    idx = 0
    for i, sample in enumerate(signal):
        delayed = buffer[idx]
        y = sample + delayed * feedback
        buffer[idx] = sample + delayed * damp
        out[i] = y
        idx = (idx + 1) % delay_samples
    return [(1 - mix) * dry + mix * wet for dry, wet in zip(signal, out)]


def render_patch(patch: Dict, duration: float, frequency: float = 220.0, sr: int = SR) -> List[float]:
    num_samples = int(duration * sr)
    if num_samples <= 0:
        raise ValueError("Duration must yield at least one sample")

    rng = random.Random(patch.get("meta", {}).get("seed", 0))
    signal = [0.0] * num_samples
    base_freq = float(frequency)

    for osc in patch.get("oscillators", []):
        waveform = osc.get("waveform", "sine")
        level = float(_clamp(osc.get("level", 1.0), 0.0, 1.0))
        detune_cents = float(_clamp(osc.get("detune_cents", 0.0), -100.0, 100.0))
        semi_offset = int(_clamp(float(osc.get("semi_offset", 0)), -24.0, 24.0))
        freq = base_freq * (2.0 ** (semi_offset / 12.0)) * (2.0 ** (detune_cents / 1200.0))
        phase_inc = 2.0 * math.pi * freq / sr
        phase = 0.0
        for i in range(num_samples):
            sample = _generate_waveform_sample(waveform, phase, rng)
            signal[i] += sample * level
            phase += phase_inc

    filter_data = patch.get("filter", {})
    cutoff = float(_clamp(filter_data.get("cutoff_hz", 1000.0), 20.0, 20_000.0))
    resonance = float(_clamp(filter_data.get("resonance", 0.2), 0.0, 1.0))
    drive = float(_clamp(filter_data.get("drive", 0.0), 0.0, 1.0))

    lfo_mod = [0.0] * num_samples
    for lfo in patch.get("lfo", []) or []:
        shape = lfo.get("shape", "sine")
        rate_hz = float(_clamp(lfo.get("rate_hz", 0.5), 0.01, 30.0))
        lfo_values = _generate_lfo(shape, rate_hz, num_samples, sr, rng)
        for target in lfo.get("targets", []) or []:
            if target.get("param") == "filter.cutoff_hz":
                depth = float(_clamp(target.get("depth", 0.0), 0.0, 1.0))
                for i in range(num_samples):
                    lfo_mod[i] += lfo_values[i] * depth

    drive_signal = _apply_drive(signal, drive)

    filter_instance = BiquadLPF(sr)
    filtered = list(drive_signal)
    chunk_size = 256
    for start in range(0, num_samples, chunk_size):
        end = min(start + chunk_size, num_samples)
        mod_slice = lfo_mod[start:end]
        if mod_slice:
            avg_mod = sum(mod_slice) / len(mod_slice)
        else:
            avg_mod = 0.0
        cutoff_chunk = cutoff * (1.0 + avg_mod)
        filter_instance.update_coefficients(_clamp(cutoff_chunk, 20.0, 20_000.0), resonance)
        filter_instance.process(filtered, start, end)

    env_settings = patch.get("env", {}).get("amp", {})
    envelope = ADSR(
        attack_ms=float(env_settings.get("attack_ms", 10.0)),
        decay_ms=float(env_settings.get("decay_ms", 200.0)),
        sustain=float(env_settings.get("sustain", 0.7)),
        release_ms=float(env_settings.get("release_ms", 300.0)),
        sr=sr,
    ).render(num_samples)

    wet = [sample * env for sample, env in zip(filtered, envelope)]

    reverb_mix = float(_clamp(patch.get("fx", {}).get("reverb", {}).get("mix", 0.0), 0.0, 1.0))
    wet = _apply_reverb(wet, reverb_mix, sr)

    peak = max((abs(sample) for sample in wet), default=0.0)
    if peak > 1e-6:
        scale = _TARGET_PEAK / peak
        wet = [sample * scale for sample in wet]

    return wet


def save_wav(path: str, audio: List[float], sr: int = SR) -> None:
    pcm = array("h", (0 for _ in range(len(audio))))
    for i, sample in enumerate(audio):
        clamped = max(-1.0, min(1.0, sample))
        pcm[i] = int(clamped * 32767.0)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(pcm.tobytes())


__all__ = ["render_patch", "save_wav", "SR"]
