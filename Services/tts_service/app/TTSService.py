from __future__ import annotations

import time

from .HealthResponse import HealthResponse
from .OmniVoiceEngine import OmniVoiceEngine
from .RequestIdGenerator import RequestIdGenerator
from .RequestValidator import RequestValidator
from .ServiceError import ServiceError
from .TTSRequest import TTSRequest
from .TTSResponse import TTSResponse
from .WavCacheManager import WavCacheManager


class TTSService:
    def __init__(
        self,
        validator: RequestValidator,
        request_id_generator: RequestIdGenerator,
        cache_manager: WavCacheManager,
        engine: OmniVoiceEngine,
        service_logger,
    ) -> None:
        self._validator = validator
        self._request_id_generator = request_id_generator
        self._cache_manager = cache_manager
        self._engine = engine
        self._service_logger = service_logger

    def warmup(self) -> None:
        self._cache_manager.ensure_dirs()
        self._engine.load_model()

    def health(self) -> HealthResponse:
        return HealthResponse(
            ok=self._engine.is_ready(),
            status="ready" if self._engine.is_ready() else "loading",
            model="OmniVoice",
        )

    def synthesize(self, request: TTSRequest) -> TTSResponse:
        self._validator.validate_tts_request(request)
        request_id = self._request_id_generator.next_id()
        self._service_logger.log_request_start(request_id, request.mode)

        started_at = time.perf_counter()
        try:
            if request.mode == "clone" and request.ref_audio and not request.ref_text:
                if self._engine.can_transcribe_ref_audio():
                    request = request.model_copy(
                        update={"ref_text": self._engine.transcribe_ref_audio(request.ref_audio)}
                    )
                else:
                    raise ServiceError(
                        "INVALID_REF_TEXT",
                        "clone mode requires ref_text when ASR is unavailable.",
                        400,
                        request_id=request_id,
                    )

            audio, sample_rate = self._engine.generate(request)
            wav_path = self._cache_manager.build_wav_path(request_id)
            saved_path = self._cache_manager.save_audio(audio, sample_rate, wav_path)
            duration_ms = int((time.perf_counter() - started_at) * 1000)
            self._service_logger.log_request_finish(request_id, saved_path, duration_ms)

            return TTSResponse(
                ok=True,
                request_id=request_id,
                mode=request.mode,
                wav_path=saved_path,
                sample_rate=sample_rate,
                duration_ms=duration_ms,
            )
        except ServiceError as ex:
            ex.request_id = request_id
            self._service_logger.log_exception(request_id, ex)
            raise
        except Exception as ex:  # pragma: no cover - wrapped for API stability
            self._service_logger.log_exception(request_id, ex)
            raise ServiceError(
                "INTERNAL_ERROR",
                f"Unexpected internal error: {ex}",
                500,
                request_id=request_id,
            ) from ex
