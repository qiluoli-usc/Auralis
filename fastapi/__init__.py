"""Minimal FastAPI-compatible surface tailored for offline testing."""

from __future__ import annotations

import inspect
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional, Tuple, get_type_hints


class HTTPException(Exception):
    def __init__(self, status_code: int, detail: str):
        super().__init__(detail)
        self.status_code = status_code
        self.detail = detail


class JSONResponse:
    def __init__(self, content: Any, status_code: int = 200):
        self.content = content
        self.status_code = status_code

    def json(self) -> Any:
        return self.content


class FastAPI:
    def __init__(self, title: str = "", version: str = ""):
        self.title = title
        self.version = version
        self._routes: Dict[Tuple[str, str], Tuple[Callable[..., Any], Optional[type]]] = {}

    def post(self, path: str) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        def decorator(func: Callable[..., Any]) -> Callable[..., Any]:
            request_model: Optional[type] = None
            signature = inspect.signature(func)
            parameters = list(signature.parameters.values())
            type_hints = get_type_hints(func)
            if parameters:
                param_name = parameters[0].name
                annotation = type_hints.get(param_name, parameters[0].annotation)
                if annotation is not inspect._empty:
                    request_model = annotation
            self._routes[("POST", path)] = (func, request_model)
            return func

        return decorator


@dataclass
class _Response:
    status_code: int
    _content: Any

    def json(self) -> Any:
        return self._content


class TestClient:
    def __init__(self, app: FastAPI):
        self.app = app

    def post(self, path: str, json: Optional[Dict[str, Any]] = None) -> _Response:
        handler, model = self.app._routes.get(("POST", path), (None, None))
        if handler is None:
            raise AssertionError(f"No handler registered for POST {path}")

        payload = json or {}

        if model is not None and hasattr(model, "from_dict"):
            try:
                request_obj = model.from_dict(payload)
            except HTTPException as exc:
                return _Response(exc.status_code, {"detail": exc.detail})
        else:
            request_obj = payload

        try:
            result = handler(request_obj)
        except HTTPException as exc:
            return _Response(exc.status_code, {"detail": exc.detail})

        if isinstance(result, JSONResponse):
            return _Response(getattr(result, "status_code", 200), result.content)

        return _Response(200, result)


__all__ = ["FastAPI", "HTTPException", "JSONResponse", "TestClient"]
