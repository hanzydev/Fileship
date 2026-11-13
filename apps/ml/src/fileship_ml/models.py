import asyncio
import logging
import os
import threading
from collections.abc import AsyncGenerator, Callable
from contextlib import asynccontextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import httpx
import onnxruntime as ort

from .presets import PRESETS, Dtype, Preset, PresetName

log = logging.getLogger('uvicorn.error')

MODELS_DIR = Path(os.environ.get('FILESHIP_ML_MODELS_DIR', '../../data/models'))
SUFFIXES: dict[Dtype, str] = {'fp32': '', 'q8': '_quantized', 'q4': '_q4'}


@dataclass(slots=True, eq=False)
class _Entry:
    value: asyncio.Future[Any]
    keep_alive: float | None
    users: int = 0
    timer: asyncio.TimerHandle | None = None


@dataclass(slots=True)
class _Config:
    preset: PresetName = 'low'
    threads: int = 1


_entries: dict[str, _Entry] = {}
_config = _Config()
_download = threading.Lock()


def preset() -> Preset:
    return PRESETS[_config.preset]


def configure(name: PresetName, threads: int) -> None:
    if (name, threads) != (_config.preset, _config.threads):
        _config.preset, _config.threads = name, threads
        unload()


def unload() -> None:
    for entry in _entries.values():
        if entry.timer:
            entry.timer.cancel()
    _entries.clear()


def models_bytes() -> int:
    return sum(path.stat().st_size for path in MODELS_DIR.rglob('*') if path.is_file())


def fetch(url: str, name: str) -> Path:
    path = MODELS_DIR / name
    if path.exists():
        return path

    with _download:
        if not path.exists():
            log.info('Downloading %s...', name)
            path.parent.mkdir(parents=True, exist_ok=True)
            temp = path.with_name(f'{path.name}.tmp')

            with httpx.stream('GET', url, follow_redirects=True, timeout=60) as response:
                response.raise_for_status()
                with temp.open('wb') as file:
                    for chunk in response.iter_bytes(1 << 20):
                        file.write(chunk)

            temp.replace(path)

    return path


def hf_file(repo: str, file: str) -> Path:
    return fetch(f'https://huggingface.co/{repo}/resolve/main/{file}', f'{repo}/{file}')


def session(path: Path) -> ort.InferenceSession:
    options = ort.SessionOptions()
    options.intra_op_num_threads = min(os.process_cpu_count() or 1, _config.threads)
    options.inter_op_num_threads = 1
    options.enable_cpu_mem_arena = False
    options.enable_mem_pattern = False
    options.log_severity_level = 3

    return ort.InferenceSession(path, options, providers=['CPUExecutionProvider'])


def hf_session(repo: str, name: str, dtype: Dtype, external: bool = False) -> ort.InferenceSession:
    file = f'onnx/{name}{SUFFIXES[dtype]}.onnx'
    if external:
        hf_file(repo, f'{file}_data')

    return session(hf_file(repo, file))


def _forget(key: str, entry: _Entry) -> None:
    if _entries.get(key) is entry:
        del _entries[key]


@asynccontextmanager
async def use[T](
    key: str, load: Callable[[], T], keep_alive: float | None = None
) -> AsyncGenerator[T]:
    entry = _entries.get(key)

    if entry is None:
        log.info('Loading %s...', key)
        future = asyncio.ensure_future(asyncio.to_thread(load))
        entry = _entries[key] = _Entry(future, keep_alive)
        loading = entry
        future.add_done_callback(lambda done: done.exception() and _forget(key, loading))
    elif entry.timer:
        entry.timer.cancel()

    if keep_alive is not None:
        entry.keep_alive = keep_alive
    entry.users += 1

    try:
        yield await entry.value
    finally:
        entry.users -= 1
        if not entry.users and _entries.get(key) is entry:
            entry.timer = asyncio.get_running_loop().call_later(
                entry.keep_alive or preset().keep_alive, _forget, key, entry
            )
