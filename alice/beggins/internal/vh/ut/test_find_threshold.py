import pytest
import numpy as np
from alice.beggins.internal.vh.scripts.python.find_thresholds_script import (
    get_threshold_max_f1_condition_recall,
    get_threshold_max_precision_condition_recall,
    get_threshold_max_f1_no_condition
)

# Attention:
# 1. in alice/beggins/internal/vh.scripts/python/find_thresholds_script.py
#    we get recall array from funtion sklearn.metrics.precision_recall_curve so recall sorted in reversed order.
# 2. len(recall) == len(thresholds) + 1, so result never can be len(recall) - 1
# 3. recall_threshold in [0, 1)

EPS = 1e-9


def concatenate(left: list, right: list):
    return [list(l) + [r] for l, r in zip(left, right)]


TestCases = [
    ([1., 1.], [1., 0.], [0.5], 0.93),
    ([1., 1.], [1., 0.], [0.7], 0.82),
    ([0.5, 0. , 1.], [1., 0., 0.], [0.5, 0.8], 0.86),
    ([1., 1.], [1., 0.], [0.5], 0.86),
    ([1., 1., 1.], [1. , 0.5, 0.], [0.45, 0.5], 0.45),
    ([0.67, 0.5, 1., 1.], [1. , 0.5, 0.5, 0.], [0.3, 0.4, 0.5], 0.93),
    ([0.67, 0.5, 0., 1.], [1. , 0.5, 0. , 0.], [0.3, 0.7, 0.9], 0.88),
    ([0.75, 0.67, 0.5, 0., 1.], [1., 0.67, 0.3, 0., 0.], [0.3 , 0.45, 0.7 , 0.9], 0.89),
    ([0.75, 0.67, 1., 1., 1.], [1., 0.67, 0.67, 0.3, 0.], [0.3 , 0.33, 0.45, 0.7], 0.95),
    ([1., 1., 1., 1.], [1., 0.67, 0.3, 0.], [0.43, 0.45, 0.7], 0.9),
    ([0.75, 0.67, 0.5, 1., 1.], [1., 0.67, 0.3, 0.3, 0.], [0.43, 0.45, 0.6 , 0.7], 0.89),
    ([0.8, 0.75, 1., 1., 1., 1.], [1., 0.75, 0.75, 0.5 , 0.25, 0.], [0.23, 0.3 , 0.45, 0.6 , 0.7], 0.99),
    ([1., 1., 1., 1., 1., 1.], [1. , 0.8, 0.6, 0.4, 0.2, 0.], [0.23, 0.3 , 0.45, 0.6 , 0.7], 0.84)
    ]

AnswersConditionF1 = [0.5, 0.7, 0.5, 0.5, 0.45, 0.3, 0.3, 0.3, 0.3, 0.43, 0.43, 0.23, 0.23]

AnswersNoCondition = [0.5, 0.7, 0.8, 0.5, 0.45, 0.3, 0.9, 0.9, 0.3, 0.43, 0.43, 0.23, 0.23]

AnswersConditionPrecision = [0.5, 0.7, 0.5, 0.5, 0.45, 0.3, 0.3, 0.3, 0.3, 0.43, 0.43, 0.23, 0.23]


@pytest.mark.parametrize('precision,recall,thresholds,recall_threshold,result', concatenate(TestCases, AnswersConditionF1))
def test_get_threshold_max_f1_condition_recall(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray,
                                                recall_threshold: float, result: float):
    assert np.abs(get_threshold_max_f1_condition_recall(precision, recall, thresholds, recall_threshold) - result) < EPS


@pytest.mark.parametrize('precision,recall,thresholds,recall_threshold,result', concatenate(TestCases, AnswersConditionF1))
def test_get_threshold_max_f1_no_condition(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray,
                                                recall_threshold: float, result: float):
    assert np.abs(get_threshold_max_f1_no_condition(precision, recall, thresholds) - result) < EPS


@pytest.mark.parametrize('precision,recall,thresholds,recall_threshold,result', concatenate(TestCases, AnswersConditionPrecision))
def test_get_threshold_max_precision_condition_recall(precision: np.ndarray, recall: np.ndarray, thresholds: np.ndarray,
                                                        recall_threshold: float, result: float):
    assert np.abs(get_threshold_max_precision_condition_recall(precision, recall, thresholds, recall_threshold) - result) < EPS
