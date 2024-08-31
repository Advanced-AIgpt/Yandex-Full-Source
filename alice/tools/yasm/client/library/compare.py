# -*- coding: utf-8 -*-
"""
Функции сортировки элементов
"""
from typing import List, Dict, Set, Tuple
from copy import deepcopy
from deepdiff import DeepDiff


# --------------------------------------------------------------------------------------------------
def comparator_common(item: str, compare_list: List) -> int:
    """
    Общая часть функции-компаратора

    :param item: Элемент сравнения
    :type item: str
    :param compare_list: Список сравнения
    :type compare_list: List
    :return: Индекс элемента
    :rtype: int
    """
    index: int
    try:
        index = compare_list.index(item.lower())
    except ValueError:
        index = (list([str(i) for i in range(10)]) + list([chr(i) for i in range(ord("a"), ord("z") + 1)])).index(
            item.lower()[0]
        ) + len(compare_list)
    return index


# --------------------------------------------------------------------------------------------------
def comparator_dc(item: str) -> int:
    """
    Кастомная сортировка ДЦ

    :param item: объект сортировки
    :type item: str
    :return: Индекс элемента
    :rtype: int
    """
    dc_order: List = ["man", "sas", "vla", "iva", "myt", "nodc"]
    return comparator_common(item, dc_order)


# --------------------------------------------------------------------------------------------------
def comparator_alert(item: str) -> int:
    """
    Кастомная сортировка алертов

    :param item: объект сортировки
    :type item: str
    :return: Индекс элемента
    :rtype: int
    """
    alert_order: List = [
        "online_instances",
        "cpu_usage",
        "cpu_throttle",
        "mem_usage",
        "major_page_faults",
        "oom",
    ]
    return comparator_common(item, alert_order)


# --------------------------------------------------------------------------------------------------
def comparator_itype(item: Dict) -> int:
    """
    Кастомная сорировка приложений

    :param item: объект сортировки
    :type item: Dict
    :return: Индекс элемента
    :rtype: int
    """
    itype_order: List = ["uniproxy", "asrgpu", "asr", "ttsgpu", "tts", "yabio", "memcache"]
    return comparator_common(item[1]["itype"], itype_order)


# --------------------------------------------------------------------------------------------------
def compare_lists(list_left: List, list_right: List) -> Tuple[List, List, List]:
    """
    Сравнение списов без сохранения порядка

    :param list_left: Первый список (А)
    :type list_left: List
    :param list_right: Второй список (Б)
    :type list_right: List
    :return: Три списка - А-Б, Б-А, А&Б
    :rtype: Tuple[List, List, List]
    """
    list_left_set: Set = set(list_left)
    list_right_set: Set = set(list_right)
    return (
        list(list_left_set - list_right_set),
        list(list_right_set - list_left_set),
        list(list_left_set & list_right_set),
    )


# --------------------------------------------------------------------------------------------------
def compare_alerts_left(dict_left: Dict, dict_right: Dict) -> DeepDiff:
    """
    Сравнение значений левого словаря с соответсвующими полями правого

    :param dict_left: Первый словарь
    :type dict_left: Dict
    :param dict_right: Второй словарь
    :type dict_right: Dict
    :return: Результат работы DeepDiff
    :rtype: DeepDiff
    """
    dict_right_clean: Dict = deepcopy(dict_right)
    if dict_right_clean.get("juggler_check", None):
        for key in ["children", "description", "mark", "meta", "namespace", "pronounce"]:
            if key not in dict_left["juggler_check"]:
                dict_right_clean["juggler_check"].pop(key, None)
        if "tags" in dict_right_clean["juggler_check"]:
            new_tags = list()
            for tag in dict_right_clean["juggler_check"]["tags"]:
                if not tag.startswith(("a_yasm_prefix_", "a_itype_", "a_ctype_", "a_geo_", "a_prj_", "a_mark_")):
                    new_tags.append(tag)
            dict_right_clean["juggler_check"].pop("tags", None)
            if new_tags:
                dict_right_clean["juggler_check"]["tags"] = new_tags
    if "updated" in dict_right_clean:
        dict_right_clean.pop("updated")
    return DeepDiff(dict_right_clean, dict_left, ignore_order=False)
