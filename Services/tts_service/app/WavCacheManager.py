from __future__ import annotations

from pathlib import Path
from typing import Iterable

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

    def get_max_cache_wavs(self) -> int:
        return self._config.max_cache_wavs

    def save_audio(self, audio, sample_rate: int, wav_path: Path) -> str:
        wav_path.parent.mkdir(parents=True, exist_ok=True)
        sf.write(str(wav_path), audio, sample_rate)
        return str(wav_path.resolve())

    def exists(self, wav_path: str | Path) -> bool:
        return Path(wav_path).exists()

    def cleanup_old_wavs(
        self,
        max_cache_wavs: int,
        protected_paths: Iterable[str | Path] | None = None,
    ) -> list[str]:
        if max_cache_wavs <= 0:
            return []

        self._config.cache_dir.mkdir(parents=True, exist_ok=True)

        protected = {
            str(Path(path).resolve())
            for path in (protected_paths or [])
        }

        wav_files = sorted(
            self._config.cache_dir.glob("*.wav"),
            key=lambda path: (path.stat().st_mtime, path.name),
            reverse=True,
        )

        removed_paths: list[str] = []
        kept_count = 0
        for wav_file in wav_files:
            resolved_path = str(wav_file.resolve())
            if resolved_path in protected:
                kept_count += 1
                continue

            if kept_count < max_cache_wavs:
                kept_count += 1
                continue

            wav_file.unlink(missing_ok=True)
            removed_paths.append(resolved_path)

        return removed_paths
