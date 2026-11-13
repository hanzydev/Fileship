from dataclasses import dataclass
from typing import Literal

type PresetName = Literal['low', 'medium', 'high']
type Dtype = Literal['fp32', 'q8', 'q4']
type OcrTier = Literal['tiny', 'small']

SIGLIP = 'pulsejet/siglip-base-patch16-256-multilingual-onnx'
SIGLIP_KEY = 'msiglip-b16'
MINILM = 'Xenova/paraphrase-multilingual-MiniLM-L12-v2'
MINILM_KEY = 'minilm-l12'
FLORENCE = 'onnx-community/Florence-2-base'
PRIVACY_FILTER = 'openai/privacy-filter'

TEXT_MAX_CHARS = 2_000
QUERY_KEEP_ALIVE = 30 * 60


@dataclass(slots=True, frozen=True)
class Preset:
    query_dtype: Dtype
    caption_task: str
    caption_dtype: Dtype
    ocr: OcrTier
    keep_alive: float


PRESETS: dict[PresetName, Preset] = {
    'low': Preset(
        query_dtype='q8',
        caption_task='<CAPTION>',
        caption_dtype='q8',
        ocr='tiny',
        keep_alive=2 * 60,
    ),
    'medium': Preset(
        query_dtype='fp32',
        caption_task='<DETAILED_CAPTION>',
        caption_dtype='q8',
        ocr='small',
        keep_alive=5 * 60,
    ),
    'high': Preset(
        query_dtype='fp32',
        caption_task='<DETAILED_CAPTION>',
        caption_dtype='fp32',
        ocr='small',
        keep_alive=15 * 60,
    ),
}
