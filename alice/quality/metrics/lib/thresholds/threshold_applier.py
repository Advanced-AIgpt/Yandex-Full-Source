from typing import Dict

import logging

from alice.quality.metrics.lib.core.metric_type import MetricType

logger = logging.getLogger(__name__)


class BinaryThresholdApplier:
    def __init__(self, predict_column: str, threshold: float):
        self._predict_column = predict_column
        self._threshold = threshold

    def __call__(self, row: dict):
        thresholded = 1 if row[self._predict_column] >= self._threshold else 0
        row[self._predict_column] = thresholded
        yield row


class MultilabelThresholdApplier:
    def __init__(self, predict_column: str, class_to_threshold: Dict[str, float]):
        self._predict_column = predict_column
        self._class_to_threshold = dict(class_to_threshold)

    def __call__(self, row):
        thresholded = {}

        for class_, prediction in row[self._predict_column].items():
            thresholded[class_] = 1 if prediction >= self._class_to_threshold[class_] else 0

        row[self._predict_column] = thresholded
        yield row


THRESHOLD_APPLIERS = {
    MetricType.BINARY: BinaryThresholdApplier,
    MetricType.MULTILABEL: MultilabelThresholdApplier,
}
