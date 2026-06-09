from __future__ import annotations

import logging
from pathlib import Path

from .AppConfig import AppConfig


class ServiceLogger:
    def __init__(self, config: AppConfig) -> None:
        self._config = config
        self._logger = logging.getLogger("local_tts_service")
        self._logger.setLevel(logging.INFO)
        self._logger.propagate = False
        self._configured = False

    def get_logger(self) -> logging.Logger:
        if not self._configured:
            self._configure()
        return self._logger

    def _configure(self) -> None:
        self._config.log_dir.mkdir(parents=True, exist_ok=True)
        formatter = logging.Formatter(
            "%(asctime)s %(levelname)s [%(name)s] %(message)s"
        )

        log_file = Path(self._config.log_dir) / "tts_service.log"
        file_handler = logging.FileHandler(log_file, encoding="utf-8")
        file_handler.setFormatter(formatter)

        stream_handler = logging.StreamHandler()
        stream_handler.setFormatter(formatter)

        self._logger.handlers.clear()
        self._logger.addHandler(file_handler)
        self._logger.addHandler(stream_handler)
        self._configured = True

    def log_request_start(self, request_id: str, mode: str) -> None:
        self.get_logger().info("request started request_id=%s mode=%s", request_id, mode)

    def log_request_finish(self, request_id: str, wav_path: str, duration_ms: int) -> None:
        self.get_logger().info(
            "request finished request_id=%s wav_path=%s duration_ms=%s",
            request_id,
            wav_path,
            duration_ms,
        )

    def log_exception(self, request_id: str, ex: Exception) -> None:
        self.get_logger().exception("request failed request_id=%s error=%s", request_id, ex)
