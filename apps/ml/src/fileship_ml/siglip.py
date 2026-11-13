import asyncio
from contextlib import AbstractAsyncContextManager
from dataclasses import dataclass
from functools import partial

import numpy as np
import onnxruntime as ort
from tokenizers import Tokenizer

from . import models
from .images import Pixels, Vector, normalize, resize
from .presets import QUERY_KEEP_ALIVE, SIGLIP, SIGLIP_KEY, Dtype

IMAGE_SIZE = 256
QUERY_TOKENS = 64
PAD_ID = 1


@dataclass(slots=True, frozen=True)
class TextTower:
    tokenizer: Tokenizer
    session: ort.InferenceSession


def _load_vision() -> ort.InferenceSession:
    return models.hf_session(SIGLIP, 'vision_model', 'q8')


def _load_text(dtype: Dtype) -> TextTower:
    return TextTower(
        Tokenizer.from_file(str(models.hf_file(SIGLIP, 'tokenizer.json'))),
        models.hf_session(SIGLIP, 'text_model', dtype),
    )


def _text_tower() -> AbstractAsyncContextManager[TextTower]:
    dtype = models.preset().query_dtype
    return models.use(
        f'image-query:{SIGLIP_KEY}:{dtype}',
        partial(_load_text, dtype),
        QUERY_KEEP_ALIVE,
    )


def _embed_image(session: ort.InferenceSession, image: Pixels) -> Vector:
    pixels = resize(image, IMAGE_SIZE, IMAGE_SIZE).astype(np.float32) / 127.5 - 1
    (pooled,) = session.run(['pooler_output'], {'pixel_values': pixels.transpose(2, 0, 1)[None]})

    return normalize(pooled[0])


def _embed_query(tower: TextTower, query: str) -> Vector:
    ids = tower.tokenizer.encode(query.lower()).ids[:QUERY_TOKENS]
    padded = np.full((1, QUERY_TOKENS), PAD_ID, np.int64)
    padded[0, : len(ids)] = ids
    (pooled,) = tower.session.run(['pooler_output'], {'input_ids': padded})

    return normalize(pooled[0])


async def embed_images(images: list[Pixels]) -> Vector:
    async with models.use(f'image:{SIGLIP_KEY}', _load_vision) as session:
        vectors = [await asyncio.to_thread(_embed_image, session, image) for image in images]

    return normalize(np.mean(vectors, axis=0))


async def embed_query(query: str) -> Vector:
    async with _text_tower() as tower:
        return await asyncio.to_thread(_embed_query, tower, query)


async def warm_up() -> None:
    async with _text_tower():
        pass
