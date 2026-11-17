import asyncio
import os
from collections.abc import AsyncGenerator
from contextlib import asynccontextmanager
from typing import Annotated

import uvicorn
from fastapi import APIRouter, Depends, FastAPI, File, Header, UploadFile
from pydantic import BaseModel, Field

from . import florence, models, ocr, privacy, siglip, text
from .images import Pixels, decode
from .presets import MINILM_KEY, SIGLIP_KEY, PresetName


class Health(BaseModel):
    models_bytes: int = Field(serialization_alias='modelsBytes')


class TextInput(BaseModel):
    text: str


class EmbedTextInput(TextInput):
    query: bool = False


class Embedding(BaseModel):
    model: str
    vector: list[float] | None


class Caption(BaseModel):
    caption: str


class Text(BaseModel):
    text: str


class Entities(BaseModel):
    entities: list[privacy.Entity]


async def configure(
    preset: Annotated[PresetName, Header(alias='Fileship-AI-Preset')],
    threads: Annotated[int, Header(alias='Fileship-AI-Threads', ge=1)],
) -> None:
    models.configure(preset, threads)


async def pixels(file: UploadFile) -> Pixels:
    return await asyncio.to_thread(decode, await file.read())


@asynccontextmanager
async def lifespan(_: FastAPI) -> AsyncGenerator[None]:
    yield
    models.unload()


app = FastAPI(title='fileship-ml', lifespan=lifespan)
router = APIRouter(dependencies=[Depends(configure)])


@app.get('/health')
async def health() -> Health:
    return Health(models_bytes=await asyncio.to_thread(models.models_bytes))


@router.post('/warmup', status_code=204)
async def warm_up() -> None:
    await siglip.warm_up()
    await text.warm_up()


@router.post('/image/embed')
async def embed_image(image: Annotated[list[UploadFile], File()]) -> Embedding:
    images = [await pixels(file) for file in image]
    return Embedding(model=SIGLIP_KEY, vector=(await siglip.embed_images(images)).tolist())


@router.post('/image/caption')
async def caption_image(image: Annotated[UploadFile, File()]) -> Caption:
    return Caption(caption=await florence.caption(await pixels(image)))


@router.post('/image/ocr')
async def read_image(image: Annotated[UploadFile, File()]) -> Text:
    return Text(text=await ocr.read(await pixels(image)))


@router.post('/text/embed')
async def embed_text(body: EmbedTextInput) -> Embedding:
    vector = await text.embed(body.text, body.query)
    return Embedding(model=MINILM_KEY, vector=None if vector is None else vector.tolist())


@router.post('/text/image-query')
async def embed_image_query(body: TextInput) -> Embedding:
    return Embedding(model=SIGLIP_KEY, vector=(await siglip.embed_query(body.text)).tolist())


@router.post('/text/entities')
async def find_entities(body: TextInput) -> Entities:
    return Entities(entities=await privacy.find(body.text))


app.include_router(router)


def main() -> None:
    uvicorn.run(
        app,
        host=os.environ.get('FILESHIP_ML_HOST', '127.0.0.1'),
        port=int(os.environ.get('FILESHIP_ML_PORT', '3003')),
        access_log=False,
    )
