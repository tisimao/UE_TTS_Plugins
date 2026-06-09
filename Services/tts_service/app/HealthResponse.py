from __future__ import annotations

from pydantic import BaseModel, ConfigDict, Field


class HealthResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    ok: bool
    service: str = "tts_service"
    status: str
    model: str
    supported_modes: list[str] = Field(default_factory=lambda: ["auto", "clone", "design"])
