from enum import Enum
from typing import NamedTuple, Sequence

import numpy as np

from sklearn import metrics

from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationResult
from alice.quality.metrics.lib.core import utils


class OptimumType(str, Enum):
    G_MEAN = 'g-mean'
    YOUDEN_INDEX = 'youden'
    F1 = 'f1'


class OptimalThreshold(NamedTuple):
    optimum_type: OptimumType
    optimal_metric_value: float
    optimal_threshold: float


def _optimize_g_mean(roc_curve, precision_recall_curve) -> OptimalThreshold:
    fpr, tpr, thresholds = roc_curve

    g_mean = np.sqrt(tpr * (1 - fpr))
    max_index = np.argmax(g_mean)

    return OptimalThreshold(OptimumType.G_MEAN, g_mean[max_index], thresholds[max_index])


def _optimize_youden(roc_curve, precision_recall_curve) -> OptimalThreshold:
    fpr, tpr, thresholds = roc_curve

    youden = tpr - fpr
    max_index = np.argmax(youden)

    return OptimalThreshold(OptimumType.YOUDEN_INDEX, youden[max_index], thresholds[max_index])


def _optimize_f1(roc_curve, precision_recall_curve) -> OptimalThreshold:
    precision, recall, thresholds = precision_recall_curve

    f1 = [utils.f1_score(p, r) for p, r in zip(precision, recall)]
    max_index = np.argmax(f1)

    return OptimalThreshold(OptimumType.F1, f1[max_index], thresholds[max_index])


_OPTIMUM_FINDERS = (
    _optimize_g_mean,
    _optimize_youden,
    _optimize_f1,
)


def build_curves_and_find_thresholds(samples: Sequence[BinaryClassificationResult]):
    if not samples:
        raise ValueError('empty samples provided')

    targets, weights, predictions = zip(*samples)

    roc_curve = metrics.roc_curve(targets, predictions, sample_weight=weights)
    precision_recall_curve = metrics.precision_recall_curve(targets, predictions, sample_weight=weights)

    optimums = {}
    for optimum_finder in _OPTIMUM_FINDERS:
        optimum = optimum_finder(roc_curve, precision_recall_curve)
        optimums[optimum.optimum_type] = optimum

    return roc_curve, precision_recall_curve, optimums
