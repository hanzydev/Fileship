import asyncio
from dataclasses import dataclass
from functools import partial

import numpy as np
import onnxruntime as ort
from numpy.lib.stride_tricks import sliding_window_view
from numpy.typing import NDArray
from tokenizers import Tokenizer

from . import models
from .images import Pixels, resize
from .presets import FLORENCE, Dtype
from .tokens import decode

IMAGE_SIZE = 768
MEAN = np.array([0.485, 0.456, 0.406], np.float32)
STD = np.array([0.229, 0.224, 0.225], np.float32)
PROMPTS = {
    '<CAPTION>': 'What does the image describe?',
    '<DETAILED_CAPTION>': 'Describe in detail what is shown in the image.',
}
BOS = 0
EOS = 2
DECODER_START = 2
MAX_NEW_TOKENS = 256
NO_REPEAT_NGRAM = 3

type Tensor = NDArray[np.float32]


@dataclass(slots=True, frozen=True)
class Florence:
    tokenizer: Tokenizer
    vision: ort.InferenceSession
    embed: ort.InferenceSession
    encoder: ort.InferenceSession
    decoder: ort.InferenceSession


def _load(dtype: Dtype) -> Florence:
    return Florence(
        Tokenizer.from_file(str(models.hf_file(FLORENCE, 'tokenizer.json'))),
        models.hf_session(FLORENCE, 'vision_encoder', dtype),
        models.hf_session(FLORENCE, 'embed_tokens', 'q8'),
        models.hf_session(FLORENCE, 'encoder_model', 'q8'),
        models.hf_session(FLORENCE, 'decoder_model_merged', 'q8'),
    )


def _embed(florence: Florence, ids: NDArray[np.int64]) -> Tensor:
    (embeds,) = florence.embed.run(['inputs_embeds'], {'input_ids': ids})
    return embeds


def _encode(florence: Florence, image: Pixels, task: str) -> Tensor:
    pixels = (resize(image, IMAGE_SIZE, IMAGE_SIZE) / np.float32(255) - MEAN) / STD
    (features,) = florence.vision.run(
        ['image_features'], {'pixel_values': pixels.transpose(2, 0, 1)[None]}
    )
    prompt = _embed(florence, np.array([florence.tokenizer.encode(PROMPTS[task]).ids], np.int64))
    embeds = np.concatenate([features, prompt], 1)
    (hidden,) = florence.encoder.run(
        ['last_hidden_state'],
        {
            'inputs_embeds': embeds,
            'attention_mask': np.ones(embeds.shape[:2], np.int64),
        },
    )

    return hidden


def _banned(tokens: list[int]) -> NDArray[np.int64]:
    if len(tokens) < NO_REPEAT_NGRAM:
        return np.empty(0, np.int64)

    windows = sliding_window_view(np.array(tokens, np.int64), NO_REPEAT_NGRAM)
    return windows[(windows[:, :-1] == tokens[1 - NO_REPEAT_NGRAM :]).all(1), -1]


def _next_token(logits: Tensor, tokens: list[int]) -> int:
    if len(tokens) in (1, MAX_NEW_TOKENS):
        return BOS if len(tokens) == 1 else EOS

    scores = logits.copy()
    scores[_banned(tokens)] = -np.inf

    return int(scores.argmax())


def _generate(florence: Florence, hidden: Tensor) -> list[int]:
    mask = np.ones(hidden.shape[:2], np.int64)
    past = {
        node.name: np.zeros((1, node.shape[1], 0, node.shape[3]), np.float32)
        for node in florence.decoder.get_inputs()
        if node.name.startswith('past_key_values')
    }
    names = [node.name for node in florence.decoder.get_outputs()]
    encoder_past: dict[str, Tensor] = {}
    tokens = [DECODER_START]

    for _ in range(MAX_NEW_TOKENS):
        logits, *presents = florence.decoder.run(
            names,
            {
                'inputs_embeds': _embed(florence, np.array([tokens[-1:]], np.int64)),
                'encoder_hidden_states': hidden,
                'encoder_attention_mask': mask,
                'use_cache_branch': np.array([bool(encoder_past)]),
                **past,
            },
        )
        cache = {
            name.replace('present', 'past_key_values'): value
            for name, value in zip(names[1:], presents, strict=True)
        }
        encoder_past = encoder_past or {
            name: value for name, value in cache.items() if '.encoder.' in name
        }
        past = {name: value for name, value in cache.items() if '.decoder.' in name} | encoder_past
        tokens.append(_next_token(logits[0, -1], tokens))

        if tokens[-1] == EOS:
            break

    return tokens


def _caption(florence: Florence, image: Pixels, task: str) -> str:
    tokens = _generate(florence, _encode(florence, image, task))
    text = decode(florence.tokenizer, tokens, skip_special_tokens=False)

    return ' '.join(text.replace('<s>', '').replace('</s>', '').split())


async def caption(image: Pixels) -> str:
    preset = models.preset()
    async with models.use(
        f'caption:{preset.caption_dtype}', partial(_load, preset.caption_dtype)
    ) as florence:
        return await asyncio.to_thread(_caption, florence, image, preset.caption_task)
