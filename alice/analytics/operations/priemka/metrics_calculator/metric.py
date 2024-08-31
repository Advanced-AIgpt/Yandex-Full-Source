# coding: utf-8

import re

CAMEL_CASE_PATTERN = re.compile(r'(?<!^)(?=[A-Z])')


class Metric(object):
    def name(self):
        """
        :return str: Имя метрики, по умолчанию возвращается имя класса в snake_case
        """
        return CAMEL_CASE_PATTERN.sub('_', self.__class__.__name__).lower()

    def group(self):
        """
        :return str: Группа метрик, для показа-скрытия всех групп в metrics_viewer, по умолчанию None
        """
        return None

    def aggregate_type(self):
        """
        :return str: Возвращает тип аггрегации для метрики:
            ratio - среднее значение
            sum - сумма значений
            custom - позволяет описать свою функцию `compare` в классе метрики, рассчитывающее значение метрики и диффы
            None - не считать среднее и диффы (только стат.тест)
        """
        return 'ratio'

    def calculation_nulls_policy(self):
        """
        :return str: Возвращает способ обработки пропусков
            при расчёте значения метрики:
            * notnulls - пропуски выкидываются, вариант по умолчанию
            * all - пропуски заменяются на prior
        """
        return 'notnulls'

    def compare_nulls_policy(self):
        """
        :return string: Возвращает способ обработки пропусков
            при расчёте стат.значимости диффа сравнения с другой метрикой:
            * union - объединение, где хотя бы одна метрика непустая, пропуски заменяются на prior
            * intersection - метрика считается только там, где она определена
            * all - берём все значения и там и там, null'ы заменяются на prior'ы (которые тоже могут быть null'ами)
        """
        return 'intersection'

    def negative_queries_policy(self):
        """
        :return string: Возвращает способ обработки negative запросов из корзинки
            * default - выкидывать negative запросы, оставляем только positive
            * defined - остаются все кроме None
            * any - в корзинке остаются None, negative и positive запросы
            * only_negatives - оставляем только negative запросы
        """
        return 'default'

    def show_zeros(self):
        """
        :return bool: Возвращать ли результат метрики, если она равна нулю
            * True — метрика будет в выходном json'е, даже если значение 0
            * False — метрика будет в выходном json'е только при ненулевом значении
        """
        return True

    def statistical_test(self):
        """
        :return str: стат. тест, применяемый к данной метрике:
            * ttest_rel - единственный поддерживаемый пока что стат.тест
        """
        return 'ttest_rel'

    def prior(self):
        """
        :return float|None: Дефолтное значение при отсутствии значения метрики
        """
        return None

    def compare(self, prod, test):
        """Сравнивает значение метрики на объектах в проде и тесте
        Для одного объекта индекс одинаковый в проде и тесте
        :param list prod: массив значений метрики в проде
        :param list test: массив значений метрики в тесте
        :return double|None, double|None, double|None, double|None: Возращает 4 числа сравнения систем:
            prod_quality, test_quality, diff, diff_percent
        """
        return None, None, None, None

    def set_params(self, metric_params):
        """
        Задает параметры для метрики
        :param dict|None metric_params: словарь значений параметров
        """
        self.params = metric_params if metric_params is not None else {}


class BaseMetric(object):
    """
    Класс, при наследовании от которого, метрика класса БУДЕТ вычисляться,
    даже если у класса отсутствует метод value(), но он есть в цепочке родительских классов
    """
    calc_this_metric = True
