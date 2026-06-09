from __future__ import annotations

import os
from typing import Any

import torch
from omnivoice import OmniVoice

from .AppConfig import AppConfig
from .ServiceError import ServiceError
from .TTSRequest import TTSRequest


class OmniVoiceEngine:
    def __init__(self, config: AppConfig, logger) -> None:
        self._config = config
        self._logger = logger
        self._model: OmniVoice | None = None
        self._asr_enabled = False

    def load_model(self) -> None:
        if self._model is not None:
            return

        if self._config.hf_cache_dir is not None:
            os.environ.setdefault("HF_HOME", str(self._config.hf_cache_dir))
            os.environ.setdefault("HUGGINGFACE_HUB_CACHE", str(self._config.hf_cache_dir))

        dtype = self._resolve_dtype(self._config.dtype)

        self._logger.info(
            "loading OmniVoice model model_name=%s device=%s dtype=%s load_asr=%s",
            self._config.model_name,
            self._config.device,
            self._config.dtype,
            self._config.load_asr,
        )
        try:
            self._model = OmniVoice.from_pretrained(
                self._config.model_name,
                device_map=self._config.device,
                dtype=dtype,
                load_asr=self._config.load_asr,
                asr_model_name=self._config.asr_model_name,
            )
            self._asr_enabled = self._config.load_asr
        except Exception as ex:
            if not self._config.load_asr:
                raise

            self._logger.warning(
                "OmniVoice ASR warmup failed, retrying without ASR: %s",
                ex,
            )
            self._model = OmniVoice.from_pretrained(
                self._config.model_name,
                device_map=self._config.device,
                dtype=dtype,
                load_asr=False,
                asr_model_name=self._config.asr_model_name,
            )
            self._asr_enabled = False
        self._logger.info("OmniVoice model loaded sample_rate=%s", self._model.sampling_rate)

    def is_ready(self) -> bool:
        return self._model is not None

    def generate(self, request: TTSRequest) -> tuple[Any, int]:
        self.load_model()
        assert self._model is not None

        kwargs: dict[str, Any] = {"text": request.text}
        if request.language_id:
            kwargs["language"] = request.language_id
        if request.ref_audio:
            kwargs["ref_audio"] = request.ref_audio
        if request.ref_text:
            kwargs["ref_text"] = request.ref_text
        if request.instruct:
            kwargs["instruct"] = request.instruct
        if request.duration is not None:
            kwargs["duration"] = request.duration
        if request.speed is not None:
            kwargs["speed"] = request.speed

        try:
            audios = self._model.generate(**kwargs)
        except Exception as ex:  # pragma: no cover - wrapped for API stability
            raise ServiceError("INFERENCE_FAILED", f"OmniVoice inference failed: {ex}", 500) from ex

        return audios[0], int(self._model.sampling_rate)

    def can_transcribe_ref_audio(self) -> bool:
        return self._asr_enabled

    def transcribe_ref_audio(self, ref_audio: str) -> str:
        self.load_model()
        assert self._model is not None

        if not self._asr_enabled:
            raise ServiceError(
                "INVALID_REF_TEXT",
                "ref_text is required when ASR is unavailable.",
                400,
            )

        try:
            return self._model.transcribe(ref_audio)
        except Exception as ex:  # pragma: no cover - wrapped for API stability
            raise ServiceError(
                "INVALID_REF_TEXT",
                f"Failed to transcribe reference audio: {ex}",
                500,
            ) from ex

    @staticmethod
    def _resolve_dtype(dtype_name: str):
        if dtype_name == "float16":
            return torch.float16
        if dtype_name == "float32":
            return torch.float32
        raise ValueError(f"Unsupported dtype: {dtype_name}")
