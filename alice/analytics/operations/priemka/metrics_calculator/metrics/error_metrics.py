# coding: utf-8

import alice.analytics.utils.yt.basket_common as common
from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.operations.priemka.alice_parser.utils.errors import (
    Ue2eResultError,
    ALL_PREDIFINED_RESULTS,
    ILL_URL_RESULT,
)


class ErrorMetric(Metric):
    """Базовый класс метрик ошибок, не содержит метода value, поэтому не вычисляется"""
    def group(self):
        return 'download_error_percent'

    def statistical_test(self):
        return None

    def show_zeros(self):
        # эти метрики ошибок показываем только при непустых значениях
        return False

    def calc_general_error_count_by_result(self, item, result):
        """
        Считает количество определённых `result`'ов для поверхности general
        :param dict item:
        :param str result:
        :return Optional[int]:
        """
        if 'app' in item and item['app'] not in common.GENERAL_APPS:
            return None

        if item.get('result') == result:
            return 1

        if item.get('result') in ALL_PREDIFINED_RESULTS:
            return None

        return 0


class RenderError(ErrorMetric):
    """
    Ошибка отрисовки скриншота. Считается, если у запроса с ПП отсутствует скриншот (поле state0.url)
    Считается только по последним запросам (у контекстов допускается отсутствие скриншотов в Толоке)
    Считается только по запросам, у которых есть скриншоты (general)
    Считается как доля ошибок рендера относительно успешно прокачанных запросов
    """
    def value(self, item):
        return self.calc_general_error_count_by_result(item, Ue2eResultError.RENDER_ERROR)


class TolokaBadUrlError(ErrorMetric):
    """Доля 'битых' скриншотов
    т.е. доля запросов, которым толокеры поставили оценку ILL_URL в толочном шаблоне со скриншотами"""
    def value(self, item):
        return self.calc_general_error_count_by_result(item, ILL_URL_RESULT)


class TolokaBadUrlErrorAbsolute(ErrorMetric):
    """Число 'битых' скриншотов
    т.е. число запросов, которым толокеры поставили оценку ILL_URL в толочном шаблоне со скриншотами"""
    def group(self):
        # чтобы не создавать отдельную группу ошибок; это не download_error, но абсолютное число ошибок ill_url нужно
        return 'download_error_absolute'

    def aggregate_type(self):
        return 'sum'

    def value(self, item):
        return self.calc_general_error_count_by_result(item, ILL_URL_RESULT)
