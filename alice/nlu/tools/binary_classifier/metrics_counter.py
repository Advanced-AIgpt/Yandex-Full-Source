# coding: utf-8

class MetricsCounter:
    def __init__(self, tp=0., tn=0., fp=0., fn=0.):
        self.tp = tp
        self.tn = tn
        self.fp = fp
        self.fn = fn

    def copy(self):
        return MetricsCounter(self.tp, self.tn, self.fp, self.fn)

    def __iadd__(self, other):
        self.tp += other.tp
        self.tn += other.tn
        self.fp += other.fp
        self.fn += other.fn
        return self

    def __add__(self, other):
        result = self.copy()
        result += other
        return result

    def __isub__(self, other):
        self.tp -= other.tp
        self.tn -= other.tn
        self.fp -= other.fp
        self.fn -= other.fn
        return self

    def __sub__(self, other):
        result = self.copy()
        result -= other
        return result

    def add_metrics(self, tp=0., tn=0., fp=0., fn=0.):
        self.tp += tp
        self.tn += tn
        self.fp += fp
        self.fn += fn

    def add_single_prediction(self, target, pred, weight=1.):
        if pred:
            if target == pred:
                self.tp += weight
            else:
                self.fp += weight
        else:
            if target == pred:
                self.tn += weight
            else:
                self.fn += weight

    def add_vector_prediction(self, target, pred, weights=1.):
        assert target.shape == pred.shape
        self.tp += ((pred == 1) * (target == 1) * weights).sum()
        self.tn += ((pred == 0) * (target == 0) * weights).sum()
        self.fp += ((pred == 1) * (target == 0) * weights).sum()
        self.fn += ((pred == 0) * (target == 1) * weights).sum()

    @property
    def target_positive(self):
        return self.tp + self.fn

    @property
    def target_negative(self):
        return self.tn + self.fp

    @property
    def result_positive(self):
        return self.tp + self.fp

    @property
    def result_negative(self):
        return self.tn + self.fn

    @property
    def precision(self):
        return self.tp / max(1e-10, self.result_positive)

    @property
    def recall(self):
        return self.tp / max(1e-10, self.target_positive)

    @property
    def f1(self):
        return 2 * self.precision * self.recall / max(1e-10, self.precision + self.recall)


class TrainingMetricsCounter:
    def __init__(self):
        self.metrics = MetricsCounter()
        self.loss_sum = 0.
        self.batch_count = 0

    def add_batch(self, target, pred, loss):
        self.metrics.add_vector_prediction(target, pred)
        self.loss_sum += loss
        self.batch_count += 1

    @property
    def loss(self):
        return self.loss_sum / max(1, self.batch_count)

    @staticmethod
    def format_info_header():
        return '[   Loss   Prec  Rec   F1 ]'

    def format_info_line(self):
        return '[ %.6f %3.f%% %3.f%% %3.f%% ]' % (
            self.loss,
            self.metrics.precision * 100,
            self.metrics.recall * 100,
            self.metrics.f1 * 100)
