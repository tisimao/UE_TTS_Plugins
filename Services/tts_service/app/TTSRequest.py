from __future__ import annotations

from typing import Literal

from pydantic import BaseModel, ConfigDict, field_validator


ModeLiteral = Literal["auto", "clone", "design"]


class TTSRequest(BaseModel):
    model_config = ConfigDict(extra="forbid", str_strip_whitespace=True)

    text: str
    mode: ModeLiteral = "auto"
    language_id: str | None = None
    ref_audio: str | None = None
    ref_text: str | None = None
    instruct: str | None = None
    duration: float | None = None
    speed: float | None = 1.0

    @field_validator("text")
    @classmethod
    def validate_text(cls, value: str) -> str:
        if not value or not value.strip():
            raise ValueError("text must not be empty")
        return value
