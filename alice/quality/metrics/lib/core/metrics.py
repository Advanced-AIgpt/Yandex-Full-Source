import alice.quality.metrics.lib.core.utils as utils
from sklearn import metrics as sk_metrics


class F1Metric(object):
    """F1 Metric class for binary classification."""

    def __init__(self, threshold):
        self._threshold = threshold
        self._tp, self._fp, self._fn = 0, 0, 0

    def add(self, target, prediction, weight):
        """Adds single binary sample to F1

        Attributes
        ----------
        target : int
            binary label (either 1 or 0)
        prediction : float
            non-thresholded predict value
        weight : int
            sample weight
        """

        prediction = 1 if prediction >= self._threshold else 0
        if target == 1:
            if prediction == 1:
                self._tp += weight
            else:
                self._fn += weight
        elif prediction == 1:
            self._fp += weight

    def get_recall(self):
        if self._tp + self._fn == 0:
            return 0
        return self._tp / (self._tp + self._fn)

    def get_precision(self):
        if self._tp + self._fp == 0:
            return 0
        return self._tp / (self._tp + self._fp)

    def get_f1(self):
        return utils.f1_score(self.get_precision(), self.get_recall())

    def get_support(self):
        return self._tp + self._fn

    def get_report(self):
        report = {}
        report['Precision'] = self.get_precision()
        report['Recall'] = self.get_recall()
        report['Support'] = self.get_support()
        report['F1'] = self.get_f1()
        return report


class MultiF1Metric(object):
    """F1 Metric class for multiclass/multilabel classification."""

    def __init__(self, class_count, threshold=0.5):
        self._threshold = threshold
        self._class_metrics = {}
        for label in range(class_count):
            self._class_metrics[label] = F1Metric(threshold)

    def add_to_single_class(self, label, target, predict, weight):
        self._class_metrics[label].add(target, predict, weight)

    def get_recall(self):
        total_recall = 0.0
        for _, metric in self._class_metrics.items():
            total_recall += metric.get_recall()
        return total_recall / len(self._class_metrics)

    def get_precision(self):
        total_precision = 0.0
        for _, metric in self._class_metrics.items():
            total_precision += metric.get_precision()
        return total_precision / len(self._class_metrics)

    def get_support(self):
        total_support = 0
        for _, metric in self._class_metrics.items():
            total_support += metric.get_support()
        return total_support

    def get_f1(self, label=None):
        if label is not None:
            return self._class_metrics[label].get_f1()
        total_f1 = 0
        for _, metric in self._class_metrics.items():
            total_f1 += metric.get_f1()
        return total_f1 / len(self._class_metrics)

    def get_micro_f1(self):
        tp, fp, fn = 0, 0, 0
        for _, metric in self._class_metrics.items():
            tp += metric._tp
            fp += metric._fp
            fn += metric._fn
        precision = 0.0 if tp + fp == 0 else tp / (tp + fp)
        recall = 0.0 if tp + fn == 0 else tp / (tp + fn)
        return utils.f1_score(precision, recall)

    def get_report(self, label=None):
        report = {}
        if label is None:
            report['Precision'] = self.get_precision()
            report['Recall'] = self.get_recall()
            report['Support'] = self.get_support()
            report['Micro_F1'] = self.get_micro_f1()
            report['F1'] = self.get_f1()
        else:
            report['Precision'] = self._class_metrics[label].get_precision()
            report['Recall'] = self._class_metrics[label].get_recall()
            report['Support'] = self._class_metrics[label].get_support()
            report['F1'] = self._class_metrics[label].get_f1()
        return report


class AucMetric(object):
    """AUC Metric class for binary classification."""

    def __init__(self):
        self._predictions = []
        self._labels = []
        self._weights = []

    def add(self, target, prediction, weight):
        self._predictions.append(prediction)
        self._labels.append(target)
        self._weights.append(weight)

    def get_auc(self):
        return sk_metrics.roc_auc_score(self._labels, self._predictions, sample_weight=self._weights)

    def get_report(self):
        return {'AUC' : self.get_auc()}
