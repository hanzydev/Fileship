import base64
import json
import os
import socket
import statistics
import subprocess
import sys
import threading
import time
from contextlib import suppress
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import httpx
import numpy as np
import psutil
from numpy.typing import NDArray

from fileship_ml.presets import PRESETS, PresetName

TESTS = Path(__file__).parent
FIXTURES = TESTS / 'fixtures'
INPUTS: dict[str, Any] = json.loads((FIXTURES / 'inputs.json').read_text('utf-8'))
REFERENCE: dict[str, Any] = json.loads((TESTS / 'reference.json').read_text('utf-8'))
MODELS_DIR = os.environ.get('FILESHIP_ML_MODELS_DIR', str(TESTS.parents[2] / 'data' / 'models'))
THREADS = 2
VIDEO_FRAMES = ('sunset', 'beach', 'city')
REFERENCE_METHODS = {
    '/image/embed': 'embedImage',
    '/text/image-query': 'embedImageQuery',
    '/text/embed': 'embedText',
    '/text/entities': 'findPrivacyEntities',
    '/image/caption': 'captionImage',
    '/image/ocr': 'recognizeText',
}

report: list[str] = []


@dataclass(slots=True)
class Run:
    preset: PresetName
    images: dict[str, list[float]] = field(default_factory=dict[str, list[float]])
    video: list[float] = field(default_factory=list[float])
    queries: dict[str, list[float]] = field(default_factory=dict[str, list[float]])
    texts: list[list[float] | None] = field(default_factory=list[list[float] | None])
    privacy: list[list[dict[str, Any]]] = field(default_factory=list[list[dict[str, Any]]])
    captions: dict[str, str] = field(default_factory=dict[str, str])
    ocr: dict[str, str] = field(default_factory=dict[str, str])
    timings: dict[str, list[float]] = field(default_factory=dict[str, list[float]])
    peak_rss: int = 0


def vector(encoded: str) -> NDArray[np.float64]:
    return np.frombuffer(base64.b64decode(encoded), '<f4').astype(np.float64)


def cosine(a: list[float] | NDArray[np.float64], b: NDArray[np.float64]) -> float:
    left = np.asarray(a, np.float64)
    return float(left @ b / np.linalg.norm(left) / np.linalg.norm(b))


def error_rate(text: str, truth: str) -> float:
    text, truth = ' '.join(text.split()), ' '.join(truth.split())
    row = list(range(len(truth) + 1))

    for i, char in enumerate(text, 1):
        previous, row[0] = row[0], i
        for j, expected in enumerate(truth, 1):
            previous, row[j] = (
                row[j],
                min(row[j] + 1, row[j - 1] + 1, previous + (char != expected)),
            )

    return row[-1] / len(truth)


def _free_port() -> int:
    with socket.socket() as server:
        server.bind(('127.0.0.1', 0))
        return server.getsockname()[1]


def _image(name: str) -> tuple[str, tuple[str, bytes, str]]:
    return 'image', (f'{name}.png', (FIXTURES / f'{name}.png').read_bytes(), 'image/png')


def _collect(run: Run, client: httpx.Client) -> None:
    def call(endpoint: str, **request: Any) -> Any:
        start = time.perf_counter()
        response = client.post(endpoint, **request)
        response.raise_for_status()
        run.timings.setdefault(endpoint, []).append(time.perf_counter() - start)

        return response.json()

    assert client.post('/warmup').status_code == httpx.codes.NO_CONTENT

    for name in INPUTS['scenes']:
        run.images[name] = call('/image/embed', files=[_image(name)])['vector']

    run.video = call('/image/embed', files=[_image(name) for name in VIDEO_FRAMES])['vector']

    for query in INPUTS['queries']:
        run.queries[query] = call('/text/image-query', json={'text': query})['vector']

    for text in INPUTS['texts']:
        run.texts.append(call('/text/embed', json={'text': text, 'query': False})['vector'])

    for text in INPUTS['privacy']:
        run.privacy.append(call('/text/entities', json={'text': text})['entities'])

    for name in INPUTS['scenes']:
        run.captions[name] = call('/image/caption', files=[_image(name)])['caption']

    for name in INPUTS['ocr']:
        run.ocr[name] = call('/image/ocr', files=[_image(name)])['text']


def _summary(run: Run) -> list[str]:
    stats = REFERENCE['stats'][run.preset]
    tier = PRESETS[run.preset].ocr
    timings = [
        f'{endpoint} {statistics.median(times) * 1000:.0f}'
        f' ({statistics.median(stats["timings"][REFERENCE_METHODS[endpoint]]):.0f})'
        for endpoint, times in run.timings.items()
    ]
    rates = {
        name: (error_rate(REFERENCE['ocr'][tier][name], truth), error_rate(run.ocr[name], truth))
        for name, truth in INPUTS['ocr'].items()
    }

    return [
        f'== {run.preset}',
        f'peak RSS {run.peak_rss / 2**30:.2f} GB (today {stats["peakRss"] / 2**30:.2f} GB)',
        f'median ms (today): {", ".join(timings)}',
        f'OCR CER {statistics.mean(new for _, new in rates.values()):.3f}'
        f' (today {statistics.mean(old for old, _ in rates.values()):.3f})',
        *(f'  {name}: {new:.3f} ({old:.3f})' for name, (old, new) in rates.items() if old != new),
        'captions (today):',
        *(
            f'  {name}: {caption}\n  {" " * len(name)}  ({REFERENCE["captions"][run.preset][name]})'
            for name, caption in run.captions.items()
        ),
    ]


def measure(preset: PresetName) -> Run:
    port = _free_port()
    process = subprocess.Popen(
        [sys.executable, '-c', 'from fileship_ml.app import main; main()'],
        env={**os.environ, 'FILESHIP_ML_PORT': str(port), 'FILESHIP_ML_MODELS_DIR': MODELS_DIR},
    )
    run = Run(preset)
    stopped = threading.Event()

    def sample() -> None:
        service = psutil.Process(process.pid)
        while not stopped.wait(0.05):
            processes = [service, *service.children(recursive=True)]
            run.peak_rss = max(run.peak_rss, sum(each.memory_info().rss for each in processes))

    sampler = threading.Thread(target=sample)
    sampler.start()

    try:
        with httpx.Client(
            base_url=f'http://127.0.0.1:{port}',
            headers={'Fileship-AI-Preset': preset, 'Fileship-AI-Threads': str(THREADS)},
            timeout=600,
        ) as client:
            while process.poll() is None:
                with suppress(httpx.TransportError):
                    if client.get('/health').is_success:
                        break
                time.sleep(0.1)

            _collect(run, client)
    finally:
        stopped.set()
        sampler.join()
        process.kill()
        process.wait()

    report.extend(_summary(run))
    return run
