from enum import Enum
from typing import Iterator, Optional, Tuple

from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationInput, BinaryClassificationResult
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder
from alice.quality.metrics.lib.multilabel.input_converter import MultilabelClassificationInput, MissingWeightPolicy

from yt.wrapper.schema import TableSchema

import yandex.type_info.typing as ti


class ErrorType(str, Enum):
    FALSE_POSITIVE = 'false_positive'
    FALSE_NEGATIVE = 'false_negative'


def _calculate_error_type(result: BinaryClassificationResult, threshold: float) -> Optional[ErrorType]:
    predicted_label = int(result.prediction >= threshold)

    if predicted_label == result.target:
        return None

    if predicted_label == 1:
        return ErrorType.FALSE_POSITIVE

    if predicted_label == 0:
        return ErrorType.FALSE_NEGATIVE

    raise ValueError(f'binary target is incorrect: {result.target}')


class BinaryMispredictionFinder:
    _ERROR_TYPE_COLUMN = 'error_type'

    def __init__(self, prediction_column: str, target_column: str, threshold: float = 0.5) -> None:
        self._prediction_column = prediction_column
        self._target_column = target_column
        self._threshold = threshold

    def __call__(self, row: dict) -> Iterator[dict]:
        sample = BinaryClassificationInput(
            target=row[self._target_column],
            prediction=row[self._prediction_column],
            weight=None,  # weight is not necessary to identify mispreds
        )

        for clf_result in sample.convert_to_result():
            error_type = _calculate_error_type(clf_result, self._threshold)

            if error_type is None:
                continue

            row[self._ERROR_TYPE_COLUMN] = error_type
            yield row

    @staticmethod
    def update_schema(schema: TableSchema):
        if any(column.name == BinaryMispredictionFinder._ERROR_TYPE_COLUMN for column in schema.columns):
            raise ValueError(f'{BinaryMispredictionFinder._ERROR_TYPE_COLUMN} is already in table')

        schema.add_column(BinaryMispredictionFinder._ERROR_TYPE_COLUMN, ti.String)

    @staticmethod
    def sort_by_columns() -> Tuple[str, ...]:
        return (BinaryMispredictionFinder._ERROR_TYPE_COLUMN, )


class MultilabelMispredictionFinder:
    _ERROR_TYPE_COLUMN = 'error_type'
    _CLASS_COLUMN = 'gold_class'

    def __init__(self, prediction_column: str, target_column: str, label_encoder: IntentLabelEncoder, threshold: float = 0.5) -> None:
        self._label_encoder = label_encoder
        self._prediction_column = prediction_column
        self._target_column = target_column
        self._threshold = threshold

    def __call__(self, row: dict) -> Iterator[dict]:
        sample = MultilabelClassificationInput(
            target=row[self._target_column],
            prediction=row[self._prediction_column],
            weight=None,  # weight and weight policy is not necessary to identify mispreds
        )

        for clf_result in sample.convert_to_result(self._label_encoder):
            for label, binary_result in enumerate(clf_result.convert_to_binary(MissingWeightPolicy.SET_TO_1)):
                error_type = _calculate_error_type(binary_result, self._threshold)

                if error_type is None:
                    continue

                row[self._CLASS_COLUMN] = self._label_encoder.decode(label)
                row[self._ERROR_TYPE_COLUMN] = error_type

                yield row

    @staticmethod
    def update_schema(schema: TableSchema):
        for added_column in [MultilabelMispredictionFinder._CLASS_COLUMN, MultilabelMispredictionFinder._ERROR_TYPE_COLUMN]:
            if any(column.name == added_column for column in schema.columns):
                raise ValueError(f'{added_column} is already in table')

            schema.add_column(added_column, ti.String)

    @staticmethod
    def sort_by_columns() -> Tuple[str, ...]:
        return (MultilabelMispredictionFinder._CLASS_COLUMN, MultilabelMispredictionFinder._ERROR_TYPE_COLUMN, )
