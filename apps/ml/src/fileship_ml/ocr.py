import asyncio
import math
import re
from collections import Counter
from dataclasses import dataclass, replace
from functools import partial
from pathlib import Path

import cv2
import numpy as np
import onnxruntime as ort
import pyclipper
from cv2.typing import MatLike, Rect
from numpy.typing import NDArray

from . import models
from .images import Pixels
from .presets import OcrTier

REPOSITORY = 'https://huggingface.co/snowfluke/ppu-paddle-ocr-models/resolve/main/'
BASE_MODELS: dict[OcrTier, tuple[str, str, str]] = {
    'tiny': (
        'detection/ort/PP-OCRv6_tiny_det.ort',
        'recognition/ort/PP-OCRv6_tiny_rec.ort',
        'recognition/ppocrv6_tiny_dict.txt',
    ),
    'small': (
        'detection/ort/PP-OCRv6_small_det.ort',
        'recognition/ort/PP-OCRv6_small_rec.ort',
        'recognition/ppocrv6_dict.txt',
    ),
}
SCRIPT_MODELS = {
    name: (
        f'recognition/multi/{folder}/v5/{folder}_PP-OCRv5_mobile_rec_infer.onnx',
        f'recognition/multi/{folder}/v5/ppocrv5_{folder}_dict.txt',
    )
    for name, folder in {
        'cyrillic': 'cyrillic',
        'arabic': 'arabic',
        'korean': 'korean',
        'thai': 'th',
        'greek': 'el',
    }.items()
}
ORIENTATION_MODEL = 'correction/PP-LCNet_x1_0_textline_ori.onnx'

MEAN = np.array([0.485, 0.456, 0.406], np.float32)
STD = np.array([0.229, 0.224, 0.225], np.float32)
DETECTION_THRESHOLD = 0.3
BOX_THRESHOLD = 0.6
UNCLIP_RATIO = 1.5
MIN_BOX_SIZE = 3
MAX_BOXES = 1000
LINE_HEIGHT = 48
MIN_LINE_WIDTH = 8
ORIENTATION_BATCH = 32
ORIENTATION_SIZE = (160, 80)
ORIENTATION_CONFIDENCE = 0.9
READ_CONFIDENCE = 0.3
MIN_CONFIDENCE = 0.7
GOOD_CONFIDENCE = 0.85
UNREAD_AREA = 0.01
LETTERS = re.compile(r'[^\W_]')
RIGHT_TO_LEFT_RUNS = re.compile(r'[0-9\u0660-\u0669.,:/-]+')

type Box = NDArray[np.float32]
type Orientation = tuple[bool, bool]


@dataclass(slots=True, frozen=True)
class Recognizer:
    session: ort.InferenceSession
    characters: NDArray[np.str_]


@dataclass(slots=True, frozen=True)
class Reader:
    detector: ort.InferenceSession
    recognizer: Recognizer
    orientation: ort.InferenceSession


@dataclass(slots=True, frozen=True)
class Line:
    crop: MatLike
    box: Box
    text: str
    confidence: float


@dataclass(slots=True, frozen=True)
class Page:
    lines: list[Line]
    orientation: Orientation


def _file(path: str) -> Path:
    return models.fetch(REPOSITORY + path, 'paddle/' + '_'.join(f'main/{path}'.split('/')[-3:]))


def _recognizer(model: str, dictionary: str) -> Recognizer:
    characters = [*_file(dictionary).read_text('utf-8').splitlines(), ' ']
    return Recognizer(models.session(_file(model)), np.array(characters))


def _load_reader(tier: OcrTier) -> Reader:
    detection, recognition, dictionary = BASE_MODELS[tier]
    return Reader(
        models.session(_file(detection)),
        _recognizer(recognition, dictionary),
        models.session(_file(ORIENTATION_MODEL)),
    )


def _min_box(points: NDArray[np.float32]) -> tuple[Box, float]:
    rect = cv2.minAreaRect(points)
    corners = sorted(cv2.boxPoints(rect).tolist(), key=lambda point: point[0])
    left = sorted(corners[:2], key=lambda point: point[1])
    right = sorted(corners[2:], key=lambda point: point[1])

    return np.array([left[0], right[0], right[1], left[1]], np.float32), min(rect[1])


def _box_score(probabilities: NDArray[np.float32], box: Box) -> float:
    height, width = probabilities.shape
    low = np.clip(np.floor(box.min(0)).astype(np.int32), 0, [width - 1, height - 1])
    high = np.clip(np.ceil(box.max(0)).astype(np.int32), 0, [width - 1, height - 1])
    mask = np.zeros((high[1] - low[1] + 1, high[0] - low[0] + 1), np.uint8)
    cv2.fillPoly(mask, [(box - low).astype(np.int32)], 1)

    return cv2.mean(probabilities[low[1] : high[1] + 1, low[0] : high[0] + 1], mask)[0]


def _unclip(box: Box) -> NDArray[np.float32]:
    distance = cv2.contourArea(box) * UNCLIP_RATIO / cv2.arcLength(box, True)
    offset = pyclipper.PyclipperOffset()
    offset.AddPath(box.tolist(), pyclipper.JT_ROUND, pyclipper.ET_CLOSEDPOLYGON)

    return np.array(offset.Execute(distance)[0], np.float32)


def _detect(detector: ort.InferenceSession, image: MatLike) -> list[Box]:
    height, width = image.shape[:2]
    size = (max(32, round(width / 32) * 32), max(32, round(height / 32) * 32))
    pixels = (cv2.resize(image, size) / np.float32(255) - MEAN) / STD
    (output,) = detector.run(None, {'x': pixels.transpose(2, 0, 1)[None]})
    probabilities: NDArray[np.float32] = output[0, 0]

    contours, _ = cv2.findContours(
        (probabilities > DETECTION_THRESHOLD).astype(np.uint8),
        cv2.RETR_LIST,
        cv2.CHAIN_APPROX_SIMPLE,
    )
    scale = np.array([width / size[0], height / size[1]], np.float32)
    boxes: list[Box] = []

    for contour in contours[:MAX_BOXES]:
        box, side = _min_box(contour.reshape(-1, 2).astype(np.float32))
        if side < MIN_BOX_SIZE or _box_score(probabilities, box) < BOX_THRESHOLD:
            continue

        box, side = _min_box(_unclip(box))
        if side >= MIN_BOX_SIZE + 2:
            boxes.append(np.clip(np.round(box * scale), 0, [width, height]).astype(np.float32))

    return boxes


def _crop(image: MatLike, box: Box) -> tuple[MatLike, bool]:
    edges = np.linalg.norm(np.roll(box, -1, 0) - box, axis=1)
    width, height = edges.reshape(2, 2).max(0).astype(int).tolist()
    target = np.array([[0, 0], [width, 0], [width, height], [0, height]], np.float32)
    crop = cv2.warpPerspective(
        image,
        cv2.getPerspectiveTransform(box, target),
        (width, height),
        flags=cv2.INTER_CUBIC,
        borderMode=cv2.BORDER_REPLICATE,
    )
    vertical = height >= 1.5 * width

    return (cv2.rotate(crop, cv2.ROTATE_90_COUNTERCLOCKWISE) if vertical else crop), vertical


def _flipped(orientation: ort.InferenceSession, crops: list[MatLike]) -> list[bool]:
    flipped: list[bool] = []

    for start in range(0, len(crops), ORIENTATION_BATCH):
        batch = np.stack(
            [
                cv2.resize(crop, ORIENTATION_SIZE)
                for crop in crops[start : start + ORIENTATION_BATCH]
            ]
        )
        pixels = (batch / np.float32(255) - MEAN) / STD
        (probabilities,) = orientation.run(None, {'x': pixels.transpose(0, 3, 1, 2)})
        flipped.extend((probabilities[:, 1] > ORIENTATION_CONFIDENCE).tolist())

    return flipped


def _recognize(recognizer: Recognizer, crop: MatLike) -> tuple[str, float]:
    height, width = crop.shape[:2]
    resized = cv2.resize(
        crop,
        (max(MIN_LINE_WIDTH, math.ceil(LINE_HEIGHT * width / height)), LINE_HEIGHT),
    )
    (output,) = recognizer.session.run(
        None, {'x': (resized / np.float32(127.5) - 1).transpose(2, 0, 1)[None]}
    )
    indices = output[0].argmax(1)
    keep = (indices != 0) & np.concatenate([[True], indices[1:] != indices[:-1]])

    if not keep.any():
        return '', 0.0

    text = ' '.join(''.join(recognizer.characters[indices[keep]]).split())
    return text, float(output[0].max(1)[keep].mean())


def _letters(text: str) -> int:
    return len(LETTERS.findall(text))


def _area(crop: MatLike) -> int:
    return crop.shape[0] * crop.shape[1]


def _score(line: Line) -> float:
    return _letters(line.text) * line.confidence


def _is_good(line: Line) -> bool:
    height, width = line.crop.shape[:2]
    return line.confidence >= GOOD_CONFIDENCE and _letters(line.text) >= 0.45 * width / height


def _read(reader: Reader, image: Pixels) -> Page:
    bgr = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    boxes = _detect(reader.detector, bgr)
    crops = [_crop(bgr, box) for box in boxes]
    flips = _flipped(reader.orientation, [crop for crop, _ in crops])
    votes: Counter[Orientation] = Counter()

    for (crop, vertical), flip in zip(crops, flips, strict=True):
        votes[vertical, flip] += _area(crop)

    orientation = max(votes, key=votes.__getitem__, default=(False, False))
    lines: list[Line] = []

    for box, (crop, vertical), flip in zip(boxes, crops, flips, strict=True):
        flipped = orientation[1] if vertical == orientation[0] else flip
        upright = cv2.rotate(crop, cv2.ROTATE_180) if flipped else crop
        lines.append(Line(upright, box, *_recognize(reader.recognizer, upright)))

    return Page(lines, orientation)


def _fix_right_to_left(text: str) -> str:
    return RIGHT_TO_LEFT_RUNS.sub(lambda run: run[0][::-1], text[::-1])


def _reread(recognizer: Recognizer, lines: list[Line], right_to_left: bool) -> list[Line]:
    alternatives: list[Line] = []

    for line in lines:
        text, confidence = _recognize(recognizer, line.crop)
        alternative = replace(
            line,
            text=_fix_right_to_left(text) if right_to_left else text,
            confidence=confidence,
        )
        alternatives.append(alternative if _score(alternative) > _score(line) else line)

    return alternatives


def _upright(box: Box, orientation: Orientation, width: int, height: int) -> Box:
    x, y = box[:, 0], box[:, 1]
    points = {
        (False, False): (x, y),
        (True, False): (y, width - x),
        (False, True): (width - x, height - y),
        (True, True): (height - y, x),
    }[orientation]

    return np.stack(points, 1)


def _height(group: list[tuple[Rect, Line]]) -> float:
    return sum(rect[3] for rect, _ in group) / len(group)


def _text(lines: list[Line], orientation: Orientation, width: int, height: int) -> str:
    items = sorted(
        (
            (cv2.boundingRect(_upright(line.box, orientation, width, height)), line)
            for line in lines
            if line.confidence >= READ_CONFIDENCE
        ),
        key=lambda item: item[0][1],
    )
    groups: list[list[tuple[Rect, Line]]] = []

    for rect, line in items:
        if groups and abs(rect[1] - groups[-1][0][0][1]) < _height(groups[-1]) / 2:
            groups[-1].append((rect, line))
        else:
            groups.append([(rect, line)])

    texts: list[str] = []
    for group in groups:
        group.sort(key=lambda item: item[0][0])
        text = ' '.join(line.text for _, line in group).strip()
        confidence = sum(line.confidence for _, line in group) / len(group)

        if confidence >= MIN_CONFIDENCE and _letters(text):
            texts.append(text)

    return '\n'.join(texts)


async def read(image: Pixels) -> str:
    height, width = image.shape[:2]
    tier = models.preset().ocr

    async with models.use(f'ocr:{tier}', partial(_load_reader, tier)) as reader:
        page = await asyncio.to_thread(_read, reader, image)

    lines = page.lines
    unread = sum(
        _area(line.crop)
        for line in lines
        if not _is_good(line) and line.crop.shape[1] >= 2.5 * line.crop.shape[0]
    )

    if unread >= UNREAD_AREA * width * height:
        for name, (model, dictionary) in SCRIPT_MODELS.items():
            async with models.use(
                f'ocr:script:{name}', partial(_recognizer, model, dictionary)
            ) as recognizer:
                lines = await asyncio.to_thread(_reread, recognizer, lines, name == 'arabic')

    return _text(lines, page.orientation, width, height)
