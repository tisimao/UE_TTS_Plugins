from __future__ import annotations

from pathlib import Path

from .ServiceError import ServiceError
from .TTSRequest import TTSRequest


class RequestValidator:
    def validate_tts_request(self, request: TTSRequest) -> None:
        if not request.text.strip():
            raise ServiceError("INVALID_TEXT", "text must not be empty.")

        self.validate_mode(request)

        if request.language_id is not None and not request.language_id.strip():
            raise ServiceError("INVALID_LANGUAGE", "language_id must not be blank.")

        if request.speed is not None and request.speed <= 0:
            raise ServiceError("INVALID_SPEED", "speed must be greater than 0.")

        if request.duration is not None and request.duration <= 0:
            raise ServiceError("INVALID_DURATION", "duration must be greater than 0.")

        if request.mode == "clone":
            self.validate_clone_fields(request)
        elif request.mode == "design":
            self.validate_design_fields(request)

    def validate_mode(self, request: TTSRequest) -> None:
        if request.mode not in {"auto", "clone", "design"}:
            raise ServiceError("INVALID_MODE", f"Unsupported mode: {request.mode}")

    def validate_clone_fields(self, request: TTSRequest) -> None:
        if not request.ref_audio:
            raise ServiceError(
                "INVALID_REF_AUDIO",
                "clone mode requires a non-empty ref_audio field.",
            )

        ref_audio_path = Path(request.ref_audio)
        if not ref_audio_path.exists():
            raise ServiceError(
                "INVALID_REF_AUDIO",
                f"Reference audio does not exist: {ref_audio_path}",
            )

        if request.ref_text is not None and not request.ref_text.strip():
            raise ServiceError(
                "INVALID_REF_TEXT",
                "ref_text must not be blank when provided.",
            )

    def validate_design_fields(self, request: TTSRequest) -> None:
        if not request.instruct or not request.instruct.strip():
            raise ServiceError(
                "INVALID_INSTRUCT",
                "design mode requires a non-empty instruct field.",
            )
