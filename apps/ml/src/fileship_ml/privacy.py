import asyncio
import json
from dataclasses import dataclass

import numpy as np
import onnxruntime as ort
from tokenizers import Tokenizer

from . import models
from .presets import PRIVACY_FILTER
from .tokens import decode

CHUNK_CHARS = 2_000
CHUNK_TOKENS = 512
MIN_CHUNK_CHARS = 64
OVERLAP_CHARS = 200


@dataclass(slots=True, frozen=True)
class Filter:
    tokenizer: Tokenizer
    session: ort.InferenceSession
    labels: list[str]


@dataclass(slots=True, frozen=True)
class Entity:
    group: str
    score: float
    word: str


@dataclass(slots=True)
class _Group:
    tag: str
    scores: list[float]
    ids: list[int]


_turn = asyncio.Lock()


def _load() -> Filter:
    labels = json.loads(models.hf_file(PRIVACY_FILTER, 'config.json').read_text())['id2label']

    return Filter(
        Tokenizer.from_file(str(models.hf_file(PRIVACY_FILTER, 'tokenizer.json'))),
        models.hf_session(PRIVACY_FILTER, 'model', 'q4', external=True),
        [labels[str(index)] for index in range(len(labels))],
    )


def _chunk_end(text: str, start: int, size: int) -> int:
    end = start + size
    if end >= len(text):
        return len(text)

    return next((i for i in range(end, start + size // 2, -1) if text[i - 1].isspace()), end)


def _next_start(text: str, start: int, end: int) -> int:
    overlap = min(OVERLAP_CHARS, (end - start) // 4)
    return next((i + 1 for i in range(end - overlap, end) if text[i].isspace()), end - overlap)


def _split(text: str, tokenizer: Tokenizer) -> list[str]:
    chunks: list[str] = []
    start = 0
    size = CHUNK_CHARS

    while start < len(text):
        end = _chunk_end(text, start, size)
        chunk = text[start:end]

        if size > MIN_CHUNK_CHARS and len(tokenizer.encode(chunk).ids) > CHUNK_TOKENS:
            size //= 2
            continue

        if chunk.strip():
            chunks.append(chunk)
        if end >= len(text):
            break

        start = _next_start(text, start, end)
        size = CHUNK_CHARS

    return chunks


def _entities(filter: Filter, chunk: str) -> list[Entity]:
    ids = filter.tokenizer.encode(chunk).ids
    inputs = np.array([ids], np.int64)
    (logits,) = filter.session.run(
        ['logits'], {'input_ids': inputs, 'attention_mask': np.ones_like(inputs)}
    )

    logits = logits[0].astype(np.float64)
    tops = logits.argmax(1)
    exp = np.exp(logits - logits[np.arange(len(tops)), tops][:, None])
    scores = 1 / exp.sum(1)

    groups: list[_Group] = []
    open_tag: str | None = None

    for index, top in enumerate(tops):
        label = filter.labels[int(top)]
        if label == 'O' or not decode(filter.tokenizer, [ids[index]]):
            continue

        prefix, tag = label.split('-', 1)
        if open_tag == tag and prefix not in ('B', 'S'):
            groups[-1].scores.append(scores[index])
            groups[-1].ids.append(ids[index])
            if prefix == 'E':
                open_tag = None
        else:
            groups.append(_Group(tag, [scores[index]], [ids[index]]))
            open_tag = None if prefix == 'S' else tag

    return [
        Entity(
            group.tag,
            sum(group.scores) / len(group.scores),
            decode(filter.tokenizer, group.ids),
        )
        for group in groups
    ]


def _find(filter: Filter, text: str) -> list[Entity]:
    found: dict[tuple[str, str], Entity] = {}

    for chunk in _split(text, filter.tokenizer):
        for entity in _entities(filter, chunk):
            key = (entity.group, entity.word.strip())
            if key not in found or found[key].score < entity.score:
                found[key] = entity

    return list(found.values())


async def find(text: str) -> list[Entity]:
    async with _turn, models.use('privacy-filter', _load) as filter:
        return await asyncio.to_thread(_find, filter, text)
