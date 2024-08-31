# coding: utf-8

import sys
from copy import deepcopy
from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.tasks.va_571.handcrafted_responses import (
    HANDCRAFTED_INTENTS,
    REASK_INTENTS,
    HANDCRAFTED_RESPONSES,
)
from alice.analytics.operations.priemka.alice_parser.utils.errors import (
    Ue2eResultError,
    ALL_PREDIFINED_RESULTS,
    FULL_UNASWERS,
)

FRAUD_PENALTY = -0.75
SIDESPEECH_PENALTY = -0.1
IGNORE_PENALTY = -0.1
LEGTH_PENALTY = -0.25
HANDCRAFTED_BONUS = 0.1
MIN_ANSWER_LENGTH_TO_PENALTY = 125
MAX_ANSWER_LENGTH_TO_PENALTY = 2


def get_str_length(obj):
    if sys.version_info[0] >= 3:
        return len(obj)

    if isinstance(obj, str):
        return len(obj.decode('utf-8'))
    elif isinstance(obj, unicode):  # noqa
        return len(obj)

    assert False, 'На вход get_str_length ожидается строка'


def length_penalty(answer):
    return LEGTH_PENALTY * min(MAX_ANSWER_LENGTH_TO_PENALTY, (get_str_length(answer) // MIN_ANSWER_LENGTH_TO_PENALTY))


def metric_with_fraud_and_length_penalty(rec):
    """
    В новой ue2e есть штрафы за обман, сайд-спич и дополнительно за длинный неуместный ответ
    :param dict rec:
    :return Optional[float]:
    """
    if rec['result'] in FULL_UNASWERS:
        return None

    # SideSpeech
    is_empty_answer = rec['result'] in [Ue2eResultError.EMPTY_VINS_RESPONSE, Ue2eResultError.EMPTY_SIDESPEECH_RESPONSE]
    if rec['text'] == '':
        if is_empty_answer:
            return 1.
        else:
            # NONEMPTY_SIDESPEECH_RESPONSE
            return SIDESPEECH_PENALTY
    elif is_empty_answer:
        return IGNORE_PENALTY

    if rec['fraud'] is True or rec['result'] == 'fraud':
        return FRAUD_PENALTY

    if rec['result'] == 'good':
        return 1.
    if rec['result'] == 'bad':
        return length_penalty(rec.get('answer') or '')
    if rec.get('intent') in HANDCRAFTED_INTENTS.union(REASK_INTENTS) or rec.get('answer') in HANDCRAFTED_RESPONSES:
        return HANDCRAFTED_BONUS
    return 0.


class QualityMetric(Metric):
    def group(self):
        return 'quality'

    def compare_nulls_policy(self):
        """
        Для расчёта p-value считаются только те пары, у которых есть значения и в проде и в тесте, null выкидываются
        VA-1775
        """
        return 'intersection'


class QualityInformationMetric(Metric):
    def group(self):
        return 'quality_info'

    def compare_nulls_policy(self):
        """
        Для расчёта p-value считаются только те пары, у которых есть значения и в проде и в тесте, null выкидываются
        VA-1775
        """
        return 'intersection'


# Important quality metrics

RESULT_MARKS_MAP = {
    'good': 1.0,
    'part': 0.5,
    'bad': 0.0,
    'fraud': -0.75,
    None: -1.0,
}


def update_max_eosp_result(item):
    """
    Обновляет поля result и fraud в случае, если текст запроса содержит <EOSp> тег
        и имеет оценки как обычной записи, так и EOSP
    :param dict item:
    :return dict: тот же объект в случае если ничего не поменялось:
        * если объект не содержит EOSP результатов
        * или EOSP результаты не лучше полных результатов
    """
    if RESULT_MARKS_MAP.get(item.get('result_eosp'), -1.0) > RESULT_MARKS_MAP.get(item.get('result'), -1.0) \
            or (item.get('fraud_eosp') is not None and item.get('fraud_eosp') < (item.get('fraud') or False)):
        # в случае, когда в результате первичного реюза приходит одна строка с заполненными result и result_eosp
        # или fraud и fraud_eosp —> то выбираем наибольшее значение
        new_item = deepcopy(item)
        new_item['result'] = item['result_eosp']
        new_item['fraud'] = item['fraud_eosp']
        return new_item

    return item


class Integral(QualityMetric):
    """Основная метрика качества Алисы.
    Учитывает штрафы за Обманы, sidespeech, длину нерел ответа; бонусы за Заглушки
    Если метрика прокрасилась в коричневый цвет — качество в тестовой выборке < 0.7, недостаточно для выкатки шортката/сценария"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item))


class Commands(QualityMetric):
    """Основная метрика integral, посчитанная на срезе Команд"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) if item.get('is_command') else None


class Music(QualityMetric):
    """Основная метрика integral, посчитанная на срезе Музыки"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'music' else None


class Video(QualityMetric):
    """Основная метрика integral, посчитанная на срезе Видео"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'video' else None


# Less important quality information metrics


class NoCommands(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе НЕ_Команд"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('is_command') is not None and item.get('is_command') is False else None


class Radio(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Радио"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'radio' else None


class Search(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Поиска"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'search' else None


class Timetables(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Расписания"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'timetables' else None


class TolokaGc(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Болталки"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'toloka_gc' else None


class Translate(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Переводчика"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'translate' else None


class Weather(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Погоды"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'weather' else None


class Geo(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Гео"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'geo' else None


class AlarmsTimers(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Будильников"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'alarms_timers' else None


class News(QualityInformationMetric):
    """Основная метрика integral, посчитанная на срезе Новостей"""
    def value(self, item):
        return metric_with_fraud_and_length_penalty(update_max_eosp_result(item)) \
            if item.get('general_toloka_intent') == 'news' else None


# интегрально по корзине
class Fraud(QualityInformationMetric):
    """Доля обманов во всей корзинке"""
    def aggregate_type(self):
        return 'ratio'

    def value(self, item):
        # don't count sidespeech and all kinds of mistakes as denominator
        if item.get('result') in ALL_PREDIFINED_RESULTS:
            return None

        updated_item = update_max_eosp_result(item)
        return 1 if updated_item.get('fraud') or updated_item.get('result') == 'fraud' else 0


# только по срезу болталки
class GeneralConversationFraud(QualityInformationMetric):
    """Доля обманов на срезе болталки"""
    def aggregate_type(self):
        return 'ratio'

    def value(self, item):
        if item.get('result') in ALL_PREDIFINED_RESULTS or item.get('generic_scenario') != "general_conversation":
            return None

        updated_item = update_max_eosp_result(item)
        return 1 if updated_item.get('fraud') or updated_item.get('result') == 'fraud' else 0
