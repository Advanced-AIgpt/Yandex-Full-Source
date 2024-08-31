from typing import Dict, Iterator, NamedTuple, Sequence, Union

import numpy as np

from alice.quality.metrics.lib.core.utils import IntentLabelEncoder


class MulticlassClassificationResult(NamedTuple):
    target: int  # class numeric label: 0, 1, ..., n - 1
    weight: float  # sample weight
    prediction: int  # predicted class numeric label: 0, 1, ..., n - 1


class MulticlassClassificationInput(NamedTuple):
    # target can be string (not-encoded) or int (encoded).
    # also list can be provided because of aggregation (toloka markup, etc...)
    target: Union[str, Sequence[str], int, Sequence[int]]
    weight: Union[None, float, Sequence[float]]

    # str -- predicted class name (not-encoded)
    # int -- predicted class index (encoded)
    # Sequence[float] -- score for each class (indexes aligned with label encoding)
    # Dict[str, float] -- score for each class
    prediction: Union[str, int, Sequence[float], Dict[str, float]]

    def _validate(self):
        if not isinstance(self.target, Sequence) and isinstance(self.weight, Sequence):
            raise ValueError('weight is sequence but target is not')
        elif not isinstance(self.target, Sequence):
            return

        # target is Sequence

        if isinstance(self.weight, Sequence) and len(self.target) != len(self.weight):
            raise ValueError('target and weight sequences must be aligned')

    def convert_to_result(self, label_encoder: IntentLabelEncoder) -> Iterator[MulticlassClassificationResult]:
        self._validate()

        predicted_label = None
        if isinstance(self.prediction, Sequence) and not isinstance(self.prediction, str):
            predicted_label = int(np.argmax(self.prediction))
        elif isinstance(self.prediction, dict):
            predicted_label = label_encoder.encode(max(self.prediction, key=self.prediction.get))
        elif isinstance(self.prediction, str):
            predicted_label = label_encoder.encode(self.prediction)
        elif isinstance(self.prediction, int):
            predicted_label = self.prediction

        assert predicted_label is not None and isinstance(predicted_label, int)

        if isinstance(self.target, str) or not isinstance(self.target, Sequence):
            raw_targets = [self.target]
        else:
            raw_targets = self.target

        target_labels = [label_encoder.encode(t, default=t) for t in raw_targets]

        if not isinstance(self.weight, Sequence):
            weight = 1 if self.weight is None else self.weight
            weights = [weight] * len(target_labels)
        else:
            weights = self.weight

        for t, w in zip(target_labels, weights):
            yield MulticlassClassificationResult(t, w, predicted_label)
