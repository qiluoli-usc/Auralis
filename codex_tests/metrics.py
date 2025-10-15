"""Offline audio metrics for the Codex smoke tests."""
from __future__ import annotations

import cmath
import math
from typing import Iterable, List


def rms(signal: Iterable[float]) -> float:
    total = 0.0
    count = 0
    for sample in signal:
        total += sample * sample
        count += 1
    if count == 0:
        return 0.0
    return math.sqrt(total / count)


def _next_power_of_two(n: int) -> int:
    if n <= 1:
        return 1
    return 1 << (n - 1).bit_length()


def _hann_window(length: int) -> List[float]:
    if length <= 1:
        return [1.0] * max(1, length)
    return [0.5 - 0.5 * math.cos(2.0 * math.pi * i / (length - 1)) for i in range(length)]


def _fft(values: List[complex]) -> List[complex]:
    n = len(values)
    if n == 1:
        return values
    even = _fft(values[0::2])
    odd = _fft(values[1::2])
    factor = [cmath.exp(-2j * math.pi * k / n) * odd[k] for k in range(n // 2)]
    result = [0j] * n
    for k in range(n // 2):
        result[k] = even[k] + factor[k]
        result[k + n // 2] = even[k] - factor[k]
    return result


def _rfft(signal: List[float]) -> List[complex]:
    n = len(signal)
    n_fft = _next_power_of_two(n)
    padded = list(signal) + [0.0] * (n_fft - n)
    complex_signal = [complex(val, 0.0) for val in padded]
    spectrum = _fft(complex_signal)
    return spectrum[: n_fft // 2 + 1]


def spectral_centroid(signal: Iterable[float], sr: int) -> float:
    samples = list(signal)
    if not samples:
        return 0.0
    window = _hann_window(len(samples))
    windowed = [sample * window[i] for i, sample in enumerate(samples)]
    spectrum = _rfft(windowed)
    n_fft = _next_power_of_two(len(samples))
    magnitudes = [abs(bin_) for bin_ in spectrum]
    total_mag = sum(magnitudes)
    if total_mag <= 1e-12:
        return 0.0
    centroid_num = 0.0
    for idx, mag in enumerate(magnitudes):
        freq = idx * sr / n_fft
        centroid_num += freq * mag
    return centroid_num / total_mag


__all__ = ["rms", "spectral_centroid"]
