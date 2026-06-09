from __future__ import annotations

from itertools import count
from threading import Lock


class RequestIdGenerator:
    def __init__(self) -> None:
        self._counter = count(1)
        self._lock = Lock()

    def next_id(self) -> str:
        with self._lock:
            value = next(self._counter)
        return f"req_{value:06d}"
