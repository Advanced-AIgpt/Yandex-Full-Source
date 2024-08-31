# coding: utf-8

import logging
import numpy as np


logger = logging.getLogger(__name__)


def _probs_to_labels(predictions, is_binary):
    if not isinstance(predictions.flat[0], np.floating):
        return predictions
    if is_binary:
        return predictions > 0.5
    return np.argmax(predictions, -1)


class F1ScoreCounter(object):
    def __init__(self, name=None, positive_class_index=1, compact=False, is_binary=True):
        self.name = name
        self.true_positives_count = 0.
        self.false_positives_count = 0.
        self.false_negatives_count = 0.
        self._positive_class_index = positive_class_index
        self._compact = compact
        self._is_binary = is_binary

    def update(self, predictions, labels, mask=None):
        # TODO: use mask
        predictions = _probs_to_labels(predictions, self._is_binary)
        self.true_positives_count += (
            (predictions == self._positive_class_index) * (labels == self._positive_class_index)
        ).sum()

        self.false_positives_count += (
            (predictions == self._positive_class_index) * (labels != self._positive_class_index)
        ).sum()

        self.false_negatives_count += (
            (predictions != self._positive_class_index) * (labels == self._positive_class_index)
        ).sum()

    @property
    def value(self):
        precision, recall, f1 = 0., 0., 0.

        if self.true_positives_count + self.false_positives_count != 0:
            precision = self.true_positives_count / (self.true_positives_count + self.false_positives_count)

        if self.true_positives_count + self.false_negatives_count != 0.:
            recall = self.true_positives_count / (self.true_positives_count + self.false_negatives_count)

        if precision + recall != 0.:
            f1 = 2 * precision * recall / (precision + recall)

        return precision, recall, f1

    def __str__(self):
        prefix = '{}: '.format(self.name) if self.name else 'F1: '
        if self._compact:
            return prefix + '{:.2%}'.format(self.value[-1])
        return prefix + '({:.2%} / {:.2%} / {:.2%})'.format(*self.value)


class AccuracyCounter(object):
    def __init__(self, name=None, masked_values=None, is_binary=True):
        self.name = name
        self.correct_count = 0.
        self.total_count = 0.
        self._masked_values = masked_values or []
        self._is_binary = is_binary

    def update(self, predictions, labels, mask=None):
        predictions = _probs_to_labels(predictions, self._is_binary)

        mask = np.ones_like(labels, dtype=np.bool) if mask is None else mask
        for masked_value in self._masked_values:
            mask &= labels != masked_value

        self.correct_count += ((predictions == labels) * mask).sum()
        self.total_count += mask.sum()

    @property
    def value(self):
        return self.correct_count / self.total_count

    def __str__(self):
        prefix = '{}: '.format(self.name) if self.name else 'Acc: '
        return prefix + '{:.2%}'.format(self.value)


class AucCounter(object):
    def __init__(self, name=None, is_binary=True, **kwargs):
        assert is_binary

        self.name = name
        self._predictions = []
        self._labels = []
        self._kwargs = kwargs

    def update(self, predictions, labels, mask=None):
        # TODO: use mask

        if predictions.shape == labels.shape + (2,):
            predictions = predictions[..., 1]
        else:
            assert predictions.shape == labels.shape

        predictions = np.reshape(predictions, (-1,))
        labels = np.reshape(labels, (-1,))

        self._predictions.extend(predictions)
        self._labels.extend(labels)

    @property
    def value(self):
        from sklearn.metrics import roc_auc_score
        try:
            return roc_auc_score(self._labels, self._predictions, **self._kwargs)
        except ValueError as e:
            logger.warning("exception in AUC training metric, returning 0.0: %s", e)
            return 0.0

    def __str__(self):
        prefix = '{}: '.format(self.name) if self.name else 'AUC: '
        return prefix + '{:.2%}'.format(self.value)


def create_metric(metric_name, **kwargs):
    if metric_name == 'accuracy':
        return AccuracyCounter(**kwargs)
    if metric_name == 'f1':
        return F1ScoreCounter(**kwargs)
    if metric_name == 'auc':
        return AucCounter(**kwargs)

    assert False, 'Unknown metric name {}'.format(metric_name)
