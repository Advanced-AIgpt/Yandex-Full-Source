#!/usr/bin/env python
# encoding: utf-8
import os
import json


class Getter(object):
    """
    Абстрактный геттер.
    Не предназначен для использования напрямую в пользовательском коде
    """
    def __init__(self, name):
        """
        :param str name: Уникальное название, в пределах эксперимента
           Для разных экспериментов может (и должно) совпадать, если геттеры вычисляют одни и те же данные
           В случае, если для геттера используется файловый кэш, это имя должно быть допустимым названием для файла
        """
        self.name = name

    def read(self, path):
        return json.load(open(path))

    def write(self, path, data):
        dir = os.path.dirname(path)
        if not os.path.exists(dir):
            os.makedirs(dir)

        with open(path, 'w') as out:
            json.dump(data, out)

    def process(self, store):
        return None

    def get_value(self, store):
        path = os.path.join(store.cache_path, self.name)
        if os.path.exists(path):
            return self.read(path)
        data = self.process(store)
        self.write(path, data)
        return data


class Const(Getter):
    """Всегда отдаёт указанную константу. Файловый кэш для него не используется"""
    def __init__(self, name, const):
        """
        :param str name: Название геттера
        :param Any const: Константу, которую нужно вернуть
        """
        super(Const, self).__init__(name)
        self.const = const

    def get_value(self, store):
        return self.const  # Skip caches


class HeavyFunction(Getter):
    """
    Функция, повторное выполнение которой нежелательно.
    Пишет результат в файловый кэш и в следующие запуски берёт из него.
    Стоит иметь в виду, что кэш привязан к имени геттера, а не к его содержимому.
       То есть, при изменении функции, результаты всё равно будут сначала браться из кэша и только если он удалён, пересчитываться заново.
    """
    def __init__(self, name, func):
        """
        :param str name:
        :param Callable(utils.comparative.experiments._Store) func:
        """
        super(HeavyFunction, self).__init__(name)
        self.func = func

    def process(self,store):
        # TODO: Можем ли мы учитывать исчезновение кэшей зависимостей?
        # Можно писать последние даты изменения файлов. Но тогда кэш нужно писать у всех, даже у констант.
        # Можно подсчитывать контрольную сумму от использованных аргументов и записывать рядом с данными
        # Сам список зависимостей легко подсчитывать в самом itemgetter'е
        # Можно во время выполнения дампить конфиг Компаратива, а в следующих запусках смотреть на дифф
        return self.func(store)


class LightFunction(HeavyFunction):
    """Функция без кэша. Пересчитывается при каждом запуске"""
    def get_value(self, store):
        return self.process(store)


class FromComparative(Getter):
    """
    Получение результатов из другого набора экспериментов
    """
    def __init__(self, name, cmpr, exp_name, key):
        super(FromComparative, self).__init__(name)
        self.cmpr = cmpr
        self.exp_name = exp_name
        self.key = key

    def get_value(self, store):
        # О кэшах уже должен был позаботиться вложенный Comparative
        return self.cmpr.stores[self.exp_name][self.key]

