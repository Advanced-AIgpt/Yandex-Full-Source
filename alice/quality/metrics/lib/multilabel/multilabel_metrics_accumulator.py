# -*- coding: utf-8 -*-

import alice.quality.metrics.lib.core.metrics as metrics
from alice.quality.metrics.lib.multilabel.input_converter import (
    MissingWeightPolicy,
    MultilabelClassificationInput,
    MultilabelClassificationResult,
)


class MultilabelF1Metric(metrics.MultiF1Metric):
    def __init__(self, class_count, weight_policy: MissingWeightPolicy, threshold):
        super().__init__(class_count, threshold)
        self._weight_policy = weight_policy

    def add(self, sample: MultilabelClassificationResult):
        """
        Add single sample to multiclass F1 metric
        """

        for label, binary_sample in enumerate(sample.convert_to_binary(self._weight_policy)):
            self.add_to_single_class(
                label,
                binary_sample.target,
                binary_sample.prediction,
                binary_sample.weight
            )


class MultilabelMetricsAccumulator(object):
    def __init__(self, intent_label_encoder, weight_policy: MissingWeightPolicy, threshold):
        self._intent_label_encoder = intent_label_encoder
        self._metrics = {}
        self._metrics["F1"] = MultilabelF1Metric(len(intent_label_encoder.intents), weight_policy, threshold)

    def add(self, true_intents, prediction, weight):
        """Add samples for multilabel metrics

        Attributes
        ----------
        true_intents : list of ints / list of strings
            target list of intents or labels
        prediction : list of floats / dict[int, float] / dict[string, float]
            predict values for each intent or label
        weight : list of ints / int
            weights for each class in target list / sample weight
        """

        clf_input = MultilabelClassificationInput(true_intents, weight, prediction)

        for clf_result in clf_input.convert_to_result(self._intent_label_encoder):
            for metric in self._metrics.values():
                metric.add(clf_result)

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
