# -*- coding: utf-8 -*-

import alice.quality.metrics.lib.core.metrics as metrics
from alice.quality.metrics.lib.multiclass.input_converter import MulticlassClassificationInput


class MulticlassF1Metric(metrics.MultiF1Metric):
    def __init__(self, class_count, threshold=0.5):
        super().__init__(class_count, threshold)

    def add(self, target, prediction, weight):
        """Update multiclass F1 metric with single sample

        Attributes
        ----------
        target : int
            binary label
        prediction : int
            binary label
        weight : int
            sample weight
        """

        if target == prediction:
            self.add_to_single_class(target, 1, 1, weight)
        else:
            self.add_to_single_class(target, 1, 0, weight)
            self.add_to_single_class(prediction, 0, 1, weight)


class MulticlassMetricsAccumulator(object):
    def __init__(self, intent_label_encoder, threshold=0.5):
        self._intent_label_encoder = intent_label_encoder
        self._metrics = {}
        self._metrics["F1"] = MulticlassF1Metric(len(intent_label_encoder.intents), threshold)

    def add(self, true_intent, prediction, weight=1):
        """Update multiclass metrics with new sample(s)

        Attributes
        ----------
        true_intent : string / list of strings / int / list of ints
            target(s), provided either as intent names or as labels
        prediction : string / list of floats / dict[string, float]
            predict values provided as:
                string - intent name

                list of floats / dict[string, float] - predict values for each class

        weight : int / list[int]
            sample weight(s)
        """

        clf_input = MulticlassClassificationInput(true_intent, weight, prediction)

        for clf_result in clf_input.convert_to_result(self._intent_label_encoder):
            for metric in self._metrics.values():
                metric.add(clf_result.target, clf_result.prediction, clf_result.weight)

    def get_metric_by_name(self, metric_name):
        return self._metrics[metric_name]

    def get_classification_report(self):
        report = {}
        for intent in self._intent_label_encoder.intents:
            label = self._intent_label_encoder.encode(intent)
            report[intent] = {}
            for metric in self._metrics.values():
                report[intent].update(metric.get_report(label))

        report['Total'] = {}
        for metric in self._metrics.values():
            report['Total'].update(metric.get_report())
        return report
