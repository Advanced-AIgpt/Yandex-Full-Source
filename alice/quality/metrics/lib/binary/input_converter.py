from typing import Iterator, NamedTuple, Sequence, Union


class BinaryClassificationResult(NamedTuple):
    target: int  # 0 or 1
    weight: float  # sample weight
    prediction: float  # classifier prediction


class BinaryClassificationInput(NamedTuple):
    target: Union[int, Sequence[int]]  # target can be list [0, 1] because of aggregation (toloka markup, etc...)
    weight: Union[None, float, Sequence[float]]
    prediction: float

    def _validate(self):
        if not isinstance(self.target, Sequence) and isinstance(self.weight, Sequence):
            raise ValueError('weight is sequence but target is not')
        elif not isinstance(self.target, Sequence):
            return

        # target is Sequence

        if isinstance(self.weight, Sequence) and len(self.target) != len(self.weight):
            raise ValueError('target and weight sequences must be aligned')

    def convert_to_result(self) -> Iterator[BinaryClassificationResult]:
        self._validate()

        if not isinstance(self.target, Sequence):
            targets = [self.target]
        else:
            targets = self.target

        if not isinstance(self.weight, Sequence):
            weight = 1 if self.weight is None else self.weight
            weights = [weight] * len(targets)
        else:
            weights = self.weight

        for t, w in zip(targets, weights):
            yield BinaryClassificationResult(t, w, self.prediction)
