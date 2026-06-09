from __future__ import annotations

from pathlib import Path

import uvicorn

from .AppConfig import AppConfig
from .FastAPIAppFactory import FastAPIAppFactory
from .OmniVoiceEngine import OmniVoiceEngine
from .RequestIdGenerator import RequestIdGenerator
from .RequestValidator import RequestValidator
from .ServiceLogger import ServiceLogger
from .TTSController import TTSController
from .TTSService import TTSService
from .WavCacheManager import WavCacheManager


class RunServer:
    def __init__(self) -> None:
        self._service_root = Path(__file__).resolve().parent.parent

    def build_runtime(self):
        config = AppConfig.from_env(self._service_root)
        service_logger = ServiceLogger(config)
        logger = service_logger.get_logger()

        validator = RequestValidator()
        request_id_generator = RequestIdGenerator()
        cache_manager = WavCacheManager(config)
        engine = OmniVoiceEngine(config, logger)
        service = TTSService(
            validator=validator,
            request_id_generator=request_id_generator,
            cache_manager=cache_manager,
            engine=engine,
            service_logger=service_logger,
        )
        controller = TTSController(service)
        app_factory = FastAPIAppFactory(
            controller=controller,
            service=service,
            logger=logger,
            eager_load=config.eager_load,
        )
        return config, app_factory

    def main(self) -> None:
        config, app_factory = self.build_runtime()
        app = app_factory.create_app()
        uvicorn.run(app, host=config.host, port=config.port, log_level="info")
