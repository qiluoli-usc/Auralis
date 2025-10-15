# Auralis

TODO: Detailed project overview will be provided.

## Offline prompt-to-patch smoke test

To exercise the offline prompt parser, renderer, and metrics without JUCE:

```bash
pip install -r requirements-codex.txt
pytest -q
python codex_tests/smoke.py
ls artifacts
```

This generates validation metrics and a `warm_pad.wav` artifact rendered purely in Python.
