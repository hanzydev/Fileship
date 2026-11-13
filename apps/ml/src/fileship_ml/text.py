import asyncio
from contextlib import AbstractAsyncContextManager
from dataclasses import dataclass

import numpy as np
import onnxruntime as ort
from tokenizers import Tokenizer

from . import models
from .images import Vector, normalize
from .presets import MINILM, MINILM_KEY, QUERY_KEEP_ALIVE, TEXT_MAX_CHARS

MAX_TOKENS = 512


@dataclass(slots=True, frozen=True)
class Encoder:
    tokenizer: Tokenizer
    session: ort.InferenceSession


def _load() -> Encoder:
    tokenizer = Tokenizer.from_file(str(models.hf_file(MINILM, 'tokenizer.json')))
    tokenizer.no_truncation()

    return Encoder(tokenizer, models.hf_session(MINILM, 'model', 'q8'))


def _encoder(query: bool) -> AbstractAsyncContextManager[Encoder]:
    return models.use(f'text:{MINILM_KEY}', _load, QUERY_KEEP_ALIVE if query else None)


def _embed(encoder: Encoder, text: str) -> Vector:
    ids = np.array([encoder.tokenizer.encode(text).ids[:MAX_TOKENS]], np.int64)
    (hidden,) = encoder.session.run(
        ['last_hidden_state'],
        {
            'input_ids': ids,
            'attention_mask': np.ones_like(ids),
            'token_type_ids': np.zeros_like(ids),
        },
    )

    return normalize(hidden[0].mean(0))


async def embed(text: str, query: bool) -> Vector | None:
    text = ' '.join(text.split())[:TEXT_MAX_CHARS]
    if not text:
        return None

    async with _encoder(query) as encoder:
        return await asyncio.to_thread(_embed, encoder, text)


async def warm_up() -> None:
    async with _encoder(True):
        pass
