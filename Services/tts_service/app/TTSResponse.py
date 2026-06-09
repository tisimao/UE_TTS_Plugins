from __future__ import annotations

from pydantic import BaseModel, ConfigDict


class TTSResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    ok: bool
    request_id: str = ""
    mode: str = ""
    wav_path: str = ""
    sample_rate: int = 0
    duration_ms: int = 0
    error_code: str = ""
    error_message: str = ""
