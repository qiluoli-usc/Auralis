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

## FastAPI prompt mapper

The optional prompt-mapping service runs entirely in Python and provides the `/map` endpoint consumed by the plugin's worker thread:

```bash
pip install -r server/requirements.txt
uvicorn server.app:app --reload
```

By default the plugin talks to `http://127.0.0.1:8000/map` and will fall back to its built-in rule parser if the service is unavailable.
