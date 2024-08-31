# coding: utf-8

from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.operations.priemka.alice_parser.utils.errors import FULL_UNASWERS
from .quality_metrics import Integral
from .utils import calc_diff_percent


class DiffByHashsum(Metric):
    """Число различающихся ответов Алисы в проде и тесте.
    В качестве значений в prod и test — указано число запросов с ответами Алисы (т.е. без неответов)
    (тоже самое, что и в 'queries_count')"""
    def group(self):
        return 'diff'

    def value(self, item):
        return item.get('hashsum')

    def aggregate_type(self):
        return 'custom'

    def compare(self, prod, test):
        length = len(prod)
        diff = sum([1 for i in range(length) if prod[i] != test[i]])
        diff_percent = 100.0 * diff / float(length) if length > 0 else 0
        return len([x for x in prod if x is not None]), len([x for x in test if x is not None]), diff, diff_percent

    def statistical_test(self):
        return None

    def compare_nulls_policy(self):
        return 'union'


class QueriesCount(Metric):
    """Количество хороших (is_negative_query == 0|None) запросов в проде и тесте, где посчитана метрика (т.е. без неответов)"""
    def group(self):
        return 'diff'

    def aggregate_type(self):
        return 'sum'

    def value(self, item):
        return 1 if item['result'] not in FULL_UNASWERS else None

    def statistical_test(self):
        return None


class NegativeQueriesCount(Metric):
    """Количество плохих запросов в проде и тесте, где посчитана метрика (т.е. без неответов)"""
    def group(self):
        return 'diff'

    def aggregate_type(self):
        return 'sum'

    def negative_queries_policy(self):
        return 'only_negatives'

    def value(self, item):
        return 1 if item['result'] not in FULL_UNASWERS else None

    def statistical_test(self):
        return None


class QueriesCountMetricPlus(Integral):
    """Число 'плюсиков', т.е. число запросов, где метрика integral на корзинке в TEST'е больше чем метрика в PROD'е"""
    def group(self):
        return 'diff'

    def value(self, item):
        return super().value(item)

    def aggregate_type(self):
        return 'custom'

    def compare(self, prod, test):
        length = len(prod)
        diff = sum([1 for i in range(length) if test[i] > prod[i]])
        diff_percent = 100.0 * diff / float(length) if length > 0 else 0
        return None, None, diff, diff_percent

    def statistical_test(self):
        return None


class QueriesCountMetricMinus(Integral):
    """Число 'минусов', т.е. число запросов, где метрика integral на корзинке в TEST'е меньше чем метрика в PROD'е"""
    def group(self):
        return 'diff'

    def value(self, item):
        return super().value(item)

    def aggregate_type(self):
        return 'custom'

    def compare(self, prod, test):
        length = len(prod)
        diff = sum([1 for i in range(length) if test[i] < prod[i]])
        diff_percent = 100.0 * diff / float(length) if length > 0 else 0
        return None, None, diff, diff_percent

    def statistical_test(self):
        return None


# Расчёт queries_count на срезах:


class QueriesCountDiffBaseMetric(Metric):
    def group(self):
        return 'diff'

    def aggregate_type(self):
        return 'custom'

    def statistical_test(self):
        return None

    def compare_function(self, value1, value2):
        return value1 != value2

    def compare(self, prod, test):
        length = len(prod)
        diff = sum([1 for i in range(length) if prod[i] is not None and test[i] is not None
                    and self.compare_function(prod[i], test[i])])
        diff_percent = 100.0 * diff / float(length) if length > 0 else 0
        return None, None, diff, diff_percent


class QueriesCountOnAsrChanged(QueriesCountDiffBaseMetric):
    """Число запросов, где разный asr_text"""
    def value(self, item):
        return item.get('asr_text')


class QueriesCountOnScenarioChanged(QueriesCountDiffBaseMetric):
    """Число запросов, где одинаковый asr_text, но разный generic_scenario"""
    def value(self, item):
        if item.get('generic_scenario') is None:
            return None
        if item.get('asr_text') is None:
            return None
        return item.get('asr_text'), item.get('generic_scenario')

    def compare_function(self, value1, value2):
        if value1[0] != value2[0]:
            return False
        return value1[1] != value2[1]


# Расчёт metric_integral на срезах:


class MetricIntegralOnDiffBaseMetric(Integral):
    def group(self):
        return 'diff'

    def aggregate_type(self):
        return 'custom'

    def compare(self, prod_values, test_values):
        prod_value = (1.0 * sum(prod_values) / len(prod_values)) if len(prod_values) else 0.0
        test_value = (1.0 * sum(test_values) / len(test_values)) if len(test_values) else 0.0
        return prod_value, test_value, test_value - prod_value, calc_diff_percent(prod_value, test_value)

    def compare_function(self, value1, value2):
        return value1 != value2

    def compare_nulls_policy(self):
        def get_values_on_different_texts(reqids, prod_values_dict, test_values_dict):
            prod_values_list, test_values_list = [], []
            for key in reqids:
                prod_value = prod_values_dict.get(key)
                test_value = test_values_dict.get(key)
                if prod_value is None or test_value is None:
                    continue
                if self.compare_function(prod_value[0], test_value[0]):
                    prod_values_list.append(prod_value[1])
                    test_values_list.append(test_value[1])
            return prod_values_list, test_values_list
        return get_values_on_different_texts


class IntegralOnAsrChanged(MetricIntegralOnDiffBaseMetric):
    """Значение метрики integral на срезе, где отличается asr_text
    Внимание: в качестве diff и diff% указан вклад (==contrib) в метрику integral на всей корзинке
    Т.е. это не дифф между тестом и продом на срезе, а дифф, нормированный на долю целевого среза относительно размера корзинки"""
    def value(self, item):
        if item.get('asr_text') is None:
            return None
        metric_value = super().value(item)
        if metric_value is None:
            return None
        return item.get('asr_text'), super().value(item)


class IntegralOnScenarioChanged(MetricIntegralOnDiffBaseMetric):
    """Значение метрики integral на срезе, где одинаковый asr_text но различается generic_scenario
    Внимание: в качестве diff и diff% указан вклад (==contrib) в метрику integral на всей корзинке
    Т.е. это не дифф между тестом и продом на срезе, а дифф, нормированный на долю целевого среза относительно размера корзинки"""

    def value(self, item):
        if item.get('generic_scenario') is None:
            return None
        if item.get('asr_text') is None:
            return None
        metric_value = super().value(item)
        if metric_value is None:
            return None
        return (item.get('asr_text'), item.get('generic_scenario')), metric_value

    def compare_function(self, value1, value2):
        if value1[0] != value2[0]:
            return False
        return value1[1] != value2[1]
