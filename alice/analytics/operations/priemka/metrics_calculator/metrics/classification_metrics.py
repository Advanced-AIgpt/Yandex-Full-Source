# coding: utf-8

from alice.analytics.operations.priemka.metrics_calculator.metric import Metric, BaseMetric


def is_positively_classified(item, selector):
    """
    Проверка является ли объект (item) позитивом для заданных ограничений на значения полей (selector)
    Если значение - строка, то проверка по вхождению, иначе по равенству
    :param item dict: строка таблицы
    :param selector dict: key - название столбца, value - список положительных значений
    """
    for column_name, positive_values in selector.items():
        item_value = item.get(column_name)
        if item_value is None:
            continue

        for value in positive_values:
            if (isinstance(value, str) and value in item_value) or value == item_value:
                return True
    return False


def can_be_classified(item, selector):
    """
    Проверка, можно ли отнести объект к какому-либо классу
    Если хотя бы в одной ячейке не None, то можем классифицировать
    """
    for column_name in selector:
        if item.get(column_name) is not None:
            return True
    return False


class ClassificationMetric(Metric):
    calc_this_metric = False

    def set_params(self, *args, **kwargs):
        super().set_params(*args, **kwargs)
        self.__doc__ = 'Метрика бинарной классификации {}. Считается по срабатыванию {}'.format(
            self.name(), self._get_selector()
        )

    def numerator(self):
        return []

    def denominator(self):
        return []

    def group(self):
        return 'classification'

    def statistical_test(self):
        return 'none'

    def compare_nulls_policy(self):
        return 'union'

    def negative_queries_policy(self):
        return 'any'

    def _get_selector(self):
        return self.params.get('classification_metrics_selector', {})

    def value(self, item):
        selector = self._get_selector()
        if not can_be_classified(item, selector):
            return None

        res = 'P' if is_positively_classified(item, selector) else 'N'
        res = ('T' if (res == 'P') ^ bool(item.get('is_negative_query', False)) else 'F') + res

        if res in self.numerator():
            return 1
        if res in self.denominator():
            return 0
        return None


class Accuracy(BaseMetric, ClassificationMetric):
    def numerator(self):
        return ['TP', 'TN']

    def denominator(self):
        return ['TP', 'TN', 'FP', 'FN']


class Precision(BaseMetric, ClassificationMetric):
    def numerator(self):
        return ['TP']

    def denominator(self):
        return ['TP', 'FP']


class Recall(BaseMetric, ClassificationMetric):
    def numerator(self):
        return ['TP']

    def denominator(self):
        return ['TP', 'FN']


class Fpr(BaseMetric, ClassificationMetric):
    def numerator(self):
        return ['FP']

    def denominator(self):
        return ['FP', 'TN']
