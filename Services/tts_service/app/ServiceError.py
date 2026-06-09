from __future__ import annotations


class ServiceError(Exception):
    def __init__(
        self,
        error_code: str,
        error_message: str,
        http_status: int = 400,
        request_id: str | None = None,
    ) -> None:
        super().__init__(error_message)
        self.error_code = error_code
        self.error_message = error_message
        self.http_status = http_status
        self.request_id = request_id or ""
