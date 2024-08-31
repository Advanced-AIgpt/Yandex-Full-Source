#!/usr/bin/env python
# encoding: utf-8
"""
Конвертация из питона в tsv формат, годный для записи в clickhouse
https://clickhouse.yandex/docs/ru/formats/tabseparated/
"""
import json
from utils.yt.reader import to_utf8


def to_str_array(values):
    if not values:
        return '[]'
    else:
        # Экранируем строчки
        return "['%s']" % "','".join('\\N' if v is None else escape(v)
                                     for v in values)


def to_num_array(values):
    return "[%s]" % ",".join(values)


def to_str_item_array(values, item):
    """
    Для вложенных структур
    Их нужно сохранять как отдельные массивы свойств
    :param list[dict] values:
    :param str item:
    :return:
    """
    if not values:
        return '[]'

    return "['%s']" % "','".join(escape(v.get(item))
                                 for v in values)


def to_item_subarrays(values, item):
    # Списки списков
    # экранируем строки массива внутри функции to_str_array,
    # потому что встречаются символы одинарных кавычек среди элементов массива actions внутри cards
    if not values:
        return '[]'
    return '[%s]' % ','.join(to_str_array(v.get(item))
                             for v in values)


def escape(value):
    "Минимальная экранизация символов, которые могут нарушить tsv разметку"
    if not value:
        if isinstance(value, basestring):
            return ''
        else:  # Отчего-то иногда встречаются не только None, но и {}
            return '\\N'

    value = to_utf8(value)  # Подразумевается, что мы конвертим из YT-формата
    return value.replace('\\', '\\\\').replace('\t', '\\t').replace('\r\n', '\\n').replace('\r', '\\n').replace('\n', '\\n').replace("'", "\\'")


def nullable_bool(value):
    if value is None:
        return '\\N'
    elif value:
        return '1'
    else:
        return '0'


def to_json(value):
    if value is None:
        return '\\N'
    else:
        return json.dumps(
            value, separators=(',', ':')
        ).decode("raw_unicode_escape").encode(
            'utf-8'
        ).replace('\\', '\\\\').replace("'", "\\'")
