from enum import Enum


class MetricType(str, Enum):
    BINARY = 'binary'
    MULTICLASS = 'multiclass'
    MULTILABEL = 'multilabel'
