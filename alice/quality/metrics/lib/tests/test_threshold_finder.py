import math

import pytest

from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationResult
from alice.quality.metrics.lib.thresholds import threshold_finder


def test_empty():
    samples = []

    with pytest.raises(ValueError):
        threshold_finder.build_curves_and_find_thresholds(samples)


def test_constant_target():
    samples = [
        BinaryClassificationResult(target, weight=1, prediction=prediction)
        for target, prediction in [
            (1, 0.9),
            (1, 0.5),
            (1, 0.4),
        ]
    ]

    _, _, optimums = threshold_finder.build_curves_and_find_thresholds(samples)

    assert math.isnan(optimums[threshold_finder.OptimumType.G_MEAN].optimal_metric_value)
    assert math.isnan(optimums[threshold_finder.OptimumType.YOUDEN_INDEX].optimal_metric_value)


def test_without_weights():
    samples = [
        BinaryClassificationResult(target, weight=1, prediction=prediction)
        for target, prediction in [
            (0, 0.9),
            (1, 0.5),
            (0, 0.4),
        ]
    ]

    _, _, optimums = threshold_finder.build_curves_and_find_thresholds(samples)

    assert all(o.optimal_threshold == 0.5 for o in optimums.values())


def test_with_weights():
    samples = [
        BinaryClassificationResult(target, weight, prediction)
        for target, weight, prediction in [
            (1, 10, 0.9),
            (0, 1, 0.8),
            (0, 1, 0.75),
            (1, 1, 0.7),
            (0, 1, 0.6)
        ]
    ]

    _, _, optimums = threshold_finder.build_curves_and_find_thresholds(samples)
    assert all(o.optimal_threshold == 0.9 for o in optimums.values())

    samples = [
        BinaryClassificationResult(target, weight, prediction)
        for target, weight, prediction in [
            (1, 1, 0.9),
            (0, 1, 0.8),
            (0, 1, 0.75),
            (1, 10, 0.7),
            (0, 1, 0.6)
        ]
    ]

    _, _, optimums = threshold_finder.build_curves_and_find_thresholds(samples)
    assert all(o.optimal_threshold == 0.7 for o in optimums.values())
