from typing import Any

import numpy as np
from harness import INPUTS, REFERENCE, VIDEO_FRAMES, Run, cosine, error_rate, vector

from fileship_ml.presets import PRESETS

VECTOR_SIMILARITY = 0.999
TEXT_SIMILARITY = 0.9999
SCORE_TOLERANCE = 1e-3


def _entities(entities: list[dict[str, Any]]) -> dict[tuple[str, str], float]:
    return {(entity['group'], entity['word'].strip()): entity['score'] for entity in entities}


def test_image_vectors(run: Run) -> None:
    for name, embedding in run.images.items():
        assert cosine(embedding, vector(REFERENCE['images'][name])) >= VECTOR_SIMILARITY, name


def test_video_vector(run: Run) -> None:
    frames = np.mean([vector(REFERENCE['images'][name]) for name in VIDEO_FRAMES], axis=0)
    assert cosine(run.video, frames) >= VECTOR_SIMILARITY


def test_query_vectors(run: Run) -> None:
    references = REFERENCE['queries'][PRESETS[run.preset].query_dtype]
    for query, embedding in run.queries.items():
        assert cosine(embedding, vector(references[query])) >= VECTOR_SIMILARITY, query


def test_text_vectors(run: Run) -> None:
    for embedding, expected in zip(run.texts, REFERENCE['texts'], strict=True):
        assert (embedding is None) == (expected is None)
        if embedding is not None:
            assert cosine(embedding, vector(expected)) >= TEXT_SIMILARITY


def test_privacy_entities(run: Run) -> None:
    for found, expected in zip(run.privacy, REFERENCE['privacy'], strict=True):
        found_scores, expected_scores = _entities(found), _entities(expected)
        assert found_scores.keys() == expected_scores.keys()
        for key, score in found_scores.items():
            assert abs(score - expected_scores[key]) < SCORE_TOLERANCE, key


def test_ocr_not_worse(run: Run) -> None:
    references = REFERENCE['ocr'][PRESETS[run.preset].ocr]
    for name, truth in INPUTS['ocr'].items():
        assert error_rate(run.ocr[name], truth) <= error_rate(references[name], truth), name


def test_captions(run: Run) -> None:
    assert all(run.captions.values())


def test_peak_rss(run: Run) -> None:
    assert run.peak_rss <= REFERENCE['stats'][run.preset]['peakRss']
