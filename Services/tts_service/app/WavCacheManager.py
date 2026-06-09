from __future__ import annotations

from pathlib import Path

import soundfile as sf

from .AppConfig import AppConfig


class WavCacheManager:
    def __init__(self, config: AppConfig) -> None:
        self._config = config

    def ensure_dirs(self) -> None:
        self._config.cache_dir.mkdir(parents=True, exist_ok=True)
        self._config.log_dir.mkdir(parents=True, exist_ok=True)

    def build_wav_path(self, request_id: str) -> Path:
        return self._config.cache_dir / f"{request_id}.wav"

    def save_audio(self, audio, sample_rate: int, wav_path: Path) -> str:
        wav_path.parent.mkdir(parents=True, exist_ok=True)
        sf.write(str(wav_path), audio, sample_rate)
        return str(wav_path.resolve())

    def exists(self, wav_path: str | Path) -> bool:
        return Path(wav_path).exists()
