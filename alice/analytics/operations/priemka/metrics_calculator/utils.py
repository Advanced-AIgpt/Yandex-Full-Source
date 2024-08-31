# coding: utf-8

import random
from alice.analytics.operations.priemka.metrics_calculator.metric import Metric
from alice.analytics.operations.priemka.metrics_calculator.metrics import *  # noqa


def get_all_subclasses(cls):
    """
    Рекурсивно получает все дочерние подклассы, отнаследуемые от указанного класса
    :param cls:
    :return set:
    """
    all_subclasses = set()

    for subclass in cls.__subclasses__():
        all_subclasses.add(subclass)
        all_subclasses.update(get_all_subclasses(subclass))

    return all_subclasses


def get_metrics():
    """
    Возвращает массив из доступных классов для расчёта метрик
    Создаёт(инициализирует) инстансы-синглтоны для каждой метрики
    Метрика должна быть проимпортирована в общее пространство имён
    :return list:
    """
    return [MetricClass() for MetricClass in get_all_subclasses(Metric)]


def random_hash(hash_length=16):
    """
    Возвращает случайную последовательность из `hash_length` символов в 16-ричной системе счисления
    :param int hash_length:
    :return str:
    """
    return ''.join(map(lambda x: random.choice('0123456789abcdef'), range(hash_length)))


def get_metric_name(metric_name):
    """
    Возвращает "полное" название метрики, включающее префикс "metric_"
    :param str metric_name:
    :return str:
    """
    if not metric_name.startswith('metric_'):
        return 'metric_' + metric_name
    return metric_name
