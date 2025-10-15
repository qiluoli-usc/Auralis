"""Lightweight JSON Schema validator subset for offline testing."""
from __future__ import annotations

from typing import Any, Dict


class ValidationError(Exception):
    """Raised when validation fails."""

    def __init__(self, message: str, path: str):
        super().__init__(message)
        self.message = message
        self.path = path


class SchemaError(Exception):
    """Raised when the schema contains unsupported constructs."""

    def __init__(self, message: str, path: str):
        super().__init__(message)
        self.message = message
        self.path = path


_SUPPORTED_TYPES = {"object", "array", "number", "integer", "string"}


def validate(instance: Any, schema: Dict[str, Any]) -> None:
    """Validate *instance* against *schema*.

    This minimal implementation supports the subset of JSON Schema features
    required by the offline tests and is intentionally small.
    """

    _validate(instance, schema, path="$")


def _validate(instance: Any, schema: Dict[str, Any], path: str) -> None:
    schema_type = schema.get("type")
    if isinstance(schema_type, list):
        allowed = set(schema_type)
    elif schema_type is None:
        allowed = None
    else:
        allowed = {schema_type}

    if allowed is not None:
        unknown = allowed - _SUPPORTED_TYPES
        if unknown:
            raise SchemaError(f"Unsupported schema type(s) {unknown}", path)

        if "object" in allowed and isinstance(instance, dict):
            pass
        elif "array" in allowed and isinstance(instance, list):
            pass
        elif "number" in allowed and isinstance(instance, (int, float)):
            pass
        elif "integer" in allowed and isinstance(instance, int) and not isinstance(instance, bool):
            pass
        elif "string" in allowed and isinstance(instance, str):
            pass
        else:
            raise ValidationError(f"Type mismatch: expected {allowed}, got {type(instance).__name__}", path)

    if "enum" in schema and instance not in schema["enum"]:
        raise ValidationError(f"Value {instance!r} not in enum", path)

    if isinstance(instance, (int, float)) and not isinstance(instance, bool):
        if "minimum" in schema and instance < schema["minimum"]:
            raise ValidationError(f"Value {instance} < minimum {schema['minimum']}", path)
        if "maximum" in schema and instance > schema["maximum"]:
            raise ValidationError(f"Value {instance} > maximum {schema['maximum']}", path)

    is_object = isinstance(instance, dict)
    if (schema.get("type") == "object") or (allowed and "object" in allowed and is_object):
        properties = schema.get("properties", {})
        required = schema.get("required", [])
        additional_properties = schema.get("additionalProperties", True)

        for key in required:
            if key not in instance:
                raise ValidationError(f"Missing required property '{key}'", path)

        for key, value in instance.items():
            if key in properties:
                _validate(value, properties[key], f"{path}.{key}")
            elif additional_properties is False:
                raise ValidationError(f"Unexpected property '{key}'", path)

    is_array = isinstance(instance, list)
    if (schema.get("type") == "array") or (allowed and "array" in allowed and is_array):
        min_items = schema.get("minItems")
        if min_items is not None and len(instance) < min_items:
            raise ValidationError(f"Array shorter than minItems={min_items}", path)
        items_schema = schema.get("items")
        if isinstance(items_schema, dict):
            for idx, item in enumerate(instance):
                _validate(item, items_schema, f"{path}[{idx}]")


class Draft7Validator:
    def __init__(self, schema: Dict[str, Any]):
        self.schema = schema

    def iter_errors(self, instance: Any):
        try:
            validate(instance, self.schema)
        except ValidationError as exc:
            yield exc

