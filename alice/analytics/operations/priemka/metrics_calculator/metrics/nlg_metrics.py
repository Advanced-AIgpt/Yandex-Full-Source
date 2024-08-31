# coding: utf-8

from alice.analytics.operations.priemka.metrics_calculator.metric import Metric, BaseMetric
from alice.analytics.operations.priemka.alice_parser.utils.errors import ALL_PREDIFINED_RESULTS
from alice.analytics.tasks.va_571.handcrafted_responses import HANDCRAFTED_RESPONSES
from cofe.projects.alice.mappings import (
    NLG_ERRORS, NLG_NOT_FOUND, NLG_SORRY, NLG_CANT
)


class NlgMetric(Metric):
    calc_this_metric = False
    patterns = None

    def group(self):
        return 'nlg'

    def value(self, item):
        return self.match(item, self.patterns)

    def match(self, item, patterns):
        """Проверяет, что хоть какой-то элемент из `patterns` присутствует в ответе Алисы `item.answer`"""
        if item['result'] in ALL_PREDIFINED_RESULTS:
            return None

        reply = (item.get('answer', '') or '').lower()
        if not reply:
            return None

        return max([1 if pattern.lower() in reply else 0
                    for pattern in patterns])


class NlgEmpty(NlgMetric):
    """Доля запросов с пустым ответом Алисы (ответ None или пустая строка)
    Как в АБ метрика 'Доля пустых запросов'"""
    def value(self, item):
        if item['result'] in ALL_PREDIFINED_RESULTS:
            return None
        reply = (item.get('answer', '') or '').lower()
        return int(not reply)


class NlgAllNotFound(BaseMetric, NlgMetric):
    """Доля запросов с ответами-заглушками типа 'я ничего не нашла, извините'
    Ответы из интентов hancrafted и reask а также включает в себя топ популярных ответов из других сценариев"""
    patterns = HANDCRAFTED_RESPONSES


class NlgNotFound(BaseMetric, NlgMetric):
    """Доля запросов с ответами Алисы содержащими подстроки типа 'не нашла', 'не удалось найти' и т.п.
    Как в АБ метрика 'Доля "Я ничего не нашла"'"""
    patterns = NLG_NOT_FOUND


class NlgErrors(BaseMetric, NlgMetric):
    """Доля запросов с ответами Алисы типа 'Прошу прощения, что-то сломалось.'
    Список ответов из common_nlg phrase error
    Как в АБ метрика 'Доля "Что-то пошло не так"'"""
    patterns = NLG_ERRORS


class NlgSorry(BaseMetric, NlgMetric):
    """Доля запросов с ответами Алисы содержащими подстроки типа 'извините', 'простите' и т.п.
    Как в АБ метрика 'Доля "Извините"'"""
    patterns = NLG_SORRY


class NlgCant(BaseMetric, NlgMetric):
    """Доля запросов с ответами Алисы содержащими подстроки типа 'не могу', 'не знаю', 'не удалось' и т.п.
    Как в АБ метрика 'Доля "Не смогла"'"""
    patterns = NLG_CANT
