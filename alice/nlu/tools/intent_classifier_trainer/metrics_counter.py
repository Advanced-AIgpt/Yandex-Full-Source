

class MetricsCounter:
    def __init__(self):
        self.tp = 0.
        self.fp = 0.
        self.fn = 0.
        self.loss_sum = 0.
        self.batch_count = 0

    def add_batch(self, y_true, y_pred, loss = None, weights = 1.):
        assert y_true.shape == y_pred.shape
        self.tp += ((y_pred == 1) * (y_true == 1) * weights).sum()
        self.fp += ((y_pred == 1) * (y_true == 0) * weights).sum()
        self.fn += ((y_pred == 0) * (y_true == 1) * weights).sum()
        if loss is not None:
            self.loss_sum += loss
        self.batch_count += 1

    @property
    def precision(self):
        return self.tp / max(1, self.tp + self.fp)

    @property
    def recall(self):
        return self.tp / max(1, self.tp + self.fn)

    @property
    def f1(self):
        return 2 * self.precision * self.recall / max(1, self.precision + self.recall)

    @property
    def loss(self):
        return self.loss_sum / max(1, self.batch_count)

    @staticmethod
    def format_info_header(need_):
        return '[ Loss  Prec  Rec  F1  ]'

    def format_info_line(self):
        return '[ %.3f %3.f%% %3.f%% %3.f%% ]' % (
            self.loss,
            self.precision * 100,
            self.recall * 100,
            self.f1 * 100)
