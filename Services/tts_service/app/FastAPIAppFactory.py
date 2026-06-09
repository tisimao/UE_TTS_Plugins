from __future__ import annotations

from contextlib import asynccontextmanager

from fastapi import Request
from fastapi import FastAPI

from .HealthResponse import HealthResponse
from .TTSController import TTSController
from .TTSResponse import TTSResponse


class FastAPIAppFactory:
    def __init__(self, controller: TTSController, service, logger, eager_load: bool) -> None:
        self._controller = controller
        self._service = service
        self._logger = logger
        self._eager_load = eager_load

    def create_app(self) -> FastAPI:
        @asynccontextmanager
        async def lifespan(_: FastAPI):
            self._logger.info("service startup begin")
            self._service._cache_manager.ensure_dirs()
            if self._eager_load:
                self._service.warmup()
            self._logger.info("service startup complete")
            yield
            self._logger.info("service shutdown complete")

        app = FastAPI(title="LocalTTS Service", version="0.1.0", lifespan=lifespan)
        self.register_middleware(app)
        self.register_routes(app)
        return app

    def register_middleware(self, app: FastAPI) -> None:
        @app.middleware("http")
        async def log_requests(request: Request, call_next):
            self._logger.info("http request begin method=%s path=%s", request.method, request.url.path)
            response = await call_next(request)
            self._logger.info(
                "http request end method=%s path=%s status_code=%s",
                request.method,
                request.url.path,
                response.status_code,
            )
            return response

    def register_routes(self, app: FastAPI) -> None:
        app.get("/health", response_model=HealthResponse)(self._controller.get_health)
        app.post("/tts", response_model=TTSResponse)(self._controller.post_tts)
