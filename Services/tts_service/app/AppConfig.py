from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


def _env_flag(name: str, default: bool) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on"}


@dataclass(slots=True)
class AppConfig:
    host: str
    port: int
    model_name: str
    device: str
    dtype: str
    cache_dir: Path
    log_dir: Path
    hf_cache_dir: Path | None
    request_timeout_sec: float
    eager_load: bool
    load_asr: bool
    asr_model_name: str

    @classmethod
    def from_env(cls, service_root: Path) -> "AppConfig":
        cache_dir = Path(os.getenv("LOCAL_TTS_CACHE_DIR", service_root / "cache"))
        log_dir = Path(os.getenv("LOCAL_TTS_LOG_DIR", service_root / "logs"))

        hf_cache_value = os.getenv("LOCAL_TTS_HF_CACHE_DIR")
        hf_cache_dir = Path(hf_cache_value) if hf_cache_value else None

        return cls(
            host=os.getenv("LOCAL_TTS_HOST", "127.0.0.1"),
            port=int(os.getenv("LOCAL_TTS_PORT", "50021")),
            model_name=os.getenv("LOCAL_TTS_MODEL_NAME", "k2-fsa/OmniVoice"),
            device=os.getenv("LOCAL_TTS_DEVICE", "cuda"),
            dtype=os.getenv("LOCAL_TTS_DTYPE", "float16"),
            cache_dir=cache_dir,
            log_dir=log_dir,
            hf_cache_dir=hf_cache_dir,
            request_timeout_sec=float(os.getenv("LOCAL_TTS_REQUEST_TIMEOUT_SEC", "300")),
            eager_load=_env_flag("LOCAL_TTS_EAGER_LOAD", True),
            load_asr=_env_flag("LOCAL_TTS_LOAD_ASR", True),
            asr_model_name=os.getenv(
                "LOCAL_TTS_ASR_MODEL_NAME", "openai/whisper-large-v3-turbo"
            ),
        )
