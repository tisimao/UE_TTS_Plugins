from __future__ import annotations

from .HealthResponse import HealthResponse
from .ServiceError import ServiceError
from .TTSRequest import TTSRequest
from .TTSResponse import TTSResponse


class TTSController:
    def __init__(self, service) -> None:
        self._service = service

    def get_health(self) -> HealthResponse:
        return self._service.health()

    def post_tts(self, request: TTSRequest) -> TTSResponse:
        try:
            return self._service.synthesize(request)
        except ServiceError as ex:
            return TTSResponse(
                ok=False,
                request_id=ex.request_id,
                error_code=ex.error_code,
                error_message=ex.error_message,
            )
