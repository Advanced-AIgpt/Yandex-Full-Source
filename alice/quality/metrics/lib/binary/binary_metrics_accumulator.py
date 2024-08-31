# -*- coding: utf-8 -*-

from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationInput
import alice.quality.metrics.lib.core.metrics as metrics


class BinaryMetricsAccumulator(object):
    def __init__(self, threshold=0.5, calculate_auc=False):
        self._calculate_auc = calculate_auc
        self._metrics = {}
        self._metrics["F1"] = metrics.F1Metric(threshold)
        if self._calculate_auc:
            self._metrics["AUC"] = metrics.AucMetric()

    def add(self, target, pred_value, weight):
        """ Update binary metrics with new sample(s).

        Attributes
        -----------
        target : int / list of ints
            binary target(s)
        pred_value : float
            non-thresholded predict value
        weight : int / list of int
            sample weight(s)
        """

        for clf_result in BinaryClassificationInput(target, weight, pred_value).convert_to_result():
            for metric in self._metrics.values():
                metric.add(clf_result.target, clf_result.prediction, clf_result.weight)

    def get_metric_by_name(self, metric_name):
        return self._metrics[metric_name]

    def get_classification_report(self):
        report = {}
        report['Total'] = {}
        for metric in self._metrics.values():
            report['Total'].update(metric.get_report())
        return report
