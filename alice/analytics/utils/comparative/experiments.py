#!/usr/bin/env python
# encoding: utf-8
import os
from collections import OrderedDict

# Фреймворк для проведения сравнительных экспериментов


class Comparative(object):
    """
    Набор однородных экспериментов
    """
    def __init__(self, experiments=(), relations=(), cache_path='tmp/cache'):
        """
        :param Collection[Experiment] experiments:
           Набор экспериментов с уникальными настройками для каждого
        :param Collection[utils.comparative.getters.Getter] relations:
           Набор геттеров, вычисляющихся одинаково для всех экспериментов
        :param str cache_path: Директория, в которой хранить закэшированные результаты выполнения геттеров
        """
        self.cache_path = cache_path

        self.experiments = OrderedDict((exp.name, exp) for exp in experiments)
        if len(experiments) != len(self.experiments):
            raise UserWarning('Duplicated experiment names')

        self.relations = {rel.name: rel for rel in relations}  # TODO: OrderedDict?
        if len(relations) != len(self.relations):
            raise UserWarning('Duplicated relation names')

        self.stores = OrderedDict((exp.name, _Store(self, exp))
                                  for exp in experiments)

        self.names = [exp.name for exp in experiments]  # для запоминания порядка

    def list_data(self, data_names):
        """
        Получение результатов в структурированном виде
        :param list[str] data_names: Список названий интересующих геттеров
        :rtype: dict[str, dict[str, Any]]
        :return: {'название_эксперимента': {'название_геттера': значение}}
        """
        if isinstance(data_names, basestring):
            data_names = [data_names]

        data = {}
        for name, store in self.stores.iteritems():
            data[name] = {dname: store[dname]
                          for dname in data_names}
        return data

    def raw_print(self, data_names, data_fmt=None):
        """
        Упрощённая печать результатов в консоль (для более сложной следует пользовать Reporter'ы)
        :param list[str] data_names: Список названий интересующих геттеров
        :param str|None data_fmt: Форматирующая строка для данных.
            Количество подстановок должно совпадать с длинной data_names
        """
        if isinstance(data_names, basestring):
            data_names = [data_names]


        max_name_len = max(map(len, self.names))
        fmt = '{:%ss}' % (max_name_len + 1)

        if data_fmt is None:
            data_fmt = ' {}' * len(data_names)
        fmt += data_fmt

        for name in self.names:
            store = self.stores[name]
            print fmt.format(name, *(store[dn]for dn in data_names))


class Experiment(object):
    """
    Один эксперимент в серии. Хранит в себе настройки, отличающиеся от других экспериментов
    """
    def __init__(self, name, getters):
        """
        :param str name: Уникальное в пределах Comparative название
            Должно быть допустимым названием для директории (в ней будут храниться файловые кэши)
        :param Collection[Getter] getters: Набор геттеров, отличных от других экспериментов
        """
        self.name = name
        self.getters = {g.name: g for g in getters}

    def getter_by_name(self, cmpr, name):
        try:
            return self.getters[name]
        except KeyError:
            return cmpr.relations[name]

    def get_data(self, cmpr, name, store):
        return self.getter_by_name(cmpr, name).get_value(store)

    def get_store(self, cmpr):
        return _Store(cmpr, self)


class _Store(dict):
    def __init__(self, cmpr, expr):
        """
        Хранит данные, посчитанные геттерами для отдельно взятого эксперимента
        :param Comparative cmpr:
        :param Experiment expr:
        """
        super(_Store, self).__init__()
        self.cmpr = cmpr
        self.expr = expr
        self.cache_path = os.path.join(cmpr.cache_path, expr.name)
        if not os.path.exists(self.cache_path):
            os.makedirs(self.cache_path)

    def __missing__(self, key):
        self[key] = self.expr.get_data(self.cmpr, key, self)
        return self[key]

    def list_all(self, key):
        """
        Получение данных сразу из всего набора экспериментов
        :param str key: Ключ геттера
        :rtype: list[Any]
        """
        return [self.cmpr.stores[name][key]
                for name in self.cmpr.names]

    def list_others(self, key):
        """
        Получение данных из всего набора экспериментов, исключая текущий
        :param str key: Ключ геттера
        :rtype: list[Any]
        """
        return [self.cmpr.stores[name][key]
                for name in self.cmpr.names
                if name != self.expr.name]
