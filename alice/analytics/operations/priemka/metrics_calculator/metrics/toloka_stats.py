# coding: utf-8

from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.operations.priemka.alice_parser.utils.errors import ALL_PREDIFINED_RESULTS


class ReusedMarksRatio(Metric):
    """Доля запросов в корзинке, имеющих толочную разметку
    В основном, используется в 'быстром контуре', чтобы получить 'кешхит'
    В полном контуре, после дооценки Толокой, метрика должна равняться 1
    Корректно учитывает неответы"""
    def group(self):
        return 'toloka_stats'

    def statistical_test(self):
        return None

    def value(self, item):
        if item.get('result') in ALL_PREDIFINED_RESULTS:
            return None

        return 1 if item.get('result') else 0
