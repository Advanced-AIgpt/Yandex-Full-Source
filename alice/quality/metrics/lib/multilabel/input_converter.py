from enum import Enum
from typing import Dict, Iterator, List, NamedTuple, Sequence, Tuple, Union


from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationResult
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder


class MissingWeightPolicy(str, Enum):
    """
    Weight policy is applied to set weights for negative classes if weights were provided for each positive class
    """

    EXISTING_SUM = 'existing_sum'
    SET_TO_1 = 'set_to_1'


def _existing_sum_weight_policy(label: int, known_weights: Dict[int, float]) -> float:
    return known_weights.get(label, sum(known_weights.values()))


def _set_to_1_weight_policy(label: int, known_weights: Dict[int, float]) -> float:
    return known_weights.get(label, 1)


_weight_policy_appliers = {
    MissingWeightPolicy.EXISTING_SUM: _existing_sum_weight_policy,
    MissingWeightPolicy.SET_TO_1: _set_to_1_weight_policy,
}


class MultilabelClassificationResult(NamedTuple):
    target: Tuple[int, ...]  # sequence of class numeric labels: 0, 1, ..., n - 1
    weight: Dict[int, float]  # class_label -> weight
    prediction: Dict[int, float]  # class_label -> prediction

    def convert_to_binary(self, weight_policy: MissingWeightPolicy) -> List[BinaryClassificationResult]:
        binary_samples = [None] * len(self.prediction)

        for class_label, prediction in self.prediction.items():
            binary_target = int(class_label in self.target)
            binary_weight = _weight_policy_appliers[weight_policy](class_label, self.weight)

            binary_samples[class_label] = BinaryClassificationResult(binary_target, binary_weight, prediction)

        return binary_samples


class MultilabelClassificationInput(NamedTuple):
    # target can be string (not-encoded) or int (encoded).
    target: Union[Sequence[str], Sequence[int]]

    # weight can be the same for all labels or be different across targets
    weight: Union[None, float, Sequence[float]]

    # Sequence[float] -- score for each class (indexes aligned with label encoding)
    # Dict[str, float] -- score for each class
    # Dict[int, float] -- score for each class label
    prediction: Union[Sequence[float], Dict[str, float], Dict[int, float]]

    def _validate(self):
        if isinstance(self.weight, Sequence) and len(self.target) != len(self.weight):
            raise ValueError('target and weight sequences must be aligned')

    def convert_to_result(self, label_encoder: IntentLabelEncoder) -> Iterator[MultilabelClassificationResult]:
        self._validate()

        if isinstance(self.prediction, Sequence):
            if len(label_encoder.intents) != len(self.prediction):
                raise ValueError("label encoder is not aligned with prediction")

            prediction_dict = dict(zip(label_encoder.intents, self.prediction))
        else:
            prediction_dict = self.prediction

        prediction = {label_encoder.encode(class_, default=class_): score for class_, score in prediction_dict.items()}

        target_labels = tuple(label_encoder.encode(t, default=t) for t in self.target)

        if not isinstance(self.weight, Sequence):
            weight = 1 if self.weight is None else self.weight
            weight_dict = dict.fromkeys(range(len(label_encoder.intents)), weight)
        else:
            weight_dict = {
                target_label: target_weight
                for target_label, target_weight in zip(target_labels, self.weight)
            }

        yield MultilabelClassificationResult(target_labels, weight_dict, prediction)
