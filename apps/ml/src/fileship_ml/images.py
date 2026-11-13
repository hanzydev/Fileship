import io

import numpy as np
from numpy.typing import NDArray
from PIL import Image

type Pixels = NDArray[np.uint8]
type Vector = NDArray[np.float64]

SUBPIXELS = 64
SHIFT = 12
HALF = 1 << (SHIFT - 1)


def _catmull_rom(x: NDArray[np.float64]) -> NDArray[np.float64]:
    rest = 1 - x
    scaled = rest * (-0.5 * x)
    left = rest * scaled
    right = x * scaled
    edge = right - left

    return np.stack([left, rest - left + edge, x - right - edge, right], -1)


COEFFICIENTS = np.trunc(_catmull_rom(np.arange(SUBPIXELS + 1) / SUBPIXELS) * (1 << SHIFT)).astype(
    np.int32
)


def _taps(size: int, source: int) -> tuple[NDArray[np.intp], NDArray[np.int32]]:
    x = np.arange(size) * (source / size) + 1
    start = np.floor(x).astype(np.intp)
    index = ((np.floor(x * SUBPIXELS * 2).astype(np.intp) & (SUBPIXELS * 2 - 1)) + 1) >> 1

    return start[:, None] + np.arange(-1, 3), COEFFICIENTS[index]


def resize(image: Pixels, width: int, height: int) -> Pixels:
    rows, columns = image.shape[:2]
    padded = np.zeros((rows + 4, columns + 4, 3), np.int32)
    padded[1 : rows + 1, 1 : columns + 1] = image

    x, x_weights = _taps(width, columns)
    y, y_weights = _taps(height, rows)
    horizontal = (np.einsum('hwkc,wk->hwc', padded[:, x], x_weights) + HALF) >> SHIFT
    vertical = (np.einsum('hkwc,hk->hwc', horizontal[y], y_weights) + HALF) >> SHIFT

    return np.clip(vertical, 0, 255).astype(np.uint8)


def decode(data: bytes) -> Pixels:
    return np.asarray(Image.open(io.BytesIO(data)).convert('RGB'))


def normalize(vector: NDArray[np.floating]) -> Vector:
    vector = vector.astype(np.float64)
    return vector / np.linalg.norm(vector)
