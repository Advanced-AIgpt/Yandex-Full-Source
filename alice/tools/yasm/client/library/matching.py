# -*- coding: utf-8 -*-
"""
Функции поиска вхождений в CSV
"""
from typing import List, Dict, Tuple, Optional
from logging import Logger, getLogger
from re import Pattern
from copy import deepcopy
from alice.tools.yasm.client.library.types import FlapSettings

DEFAULT_CRIT: List = [15.0, None]
DEFAULT_WARN: List = [5.0, 15.0]
DEFAULT_FLAP: FlapSettings = FlapSettings(critical=120, stable=60, boost=0)


# --------------------------------------------------------------------------------------------------
def match_at_least_one(source: str, re_keys: Optional[List[str]], re_comp: Dict[str, Pattern]) -> bool:
    """
    Тестирует строку против списка регулярных выражений.
    Возвращает истину если подошло хотя бы под одно регулярное выражение

    :param source: Строка для проверки
    :type source: str
    :param re_keys: Список регулярных выражений текстом
    :type re_keys: Options[List[str]]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Подошло ли хотя бы одно регулярное выражение
    :rtype: bool
    """
    result: bool = False
    if re_keys:
        for regex in re_keys:
            if re_comp[regex].fullmatch(source):
                result = True
                break
    return result


# --------------------------------------------------------------------------------------------------
def match_notifications(
    alert: str,
    notifications: Dict,
    match_list: List[Dict],
    re_comp: Dict[str, Pattern],
    skip_default: bool = False,
) -> List[Dict]:
    """
    Список уведомлений для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param notifications: Словарь с уведомлениями
    :type notifications: Dict
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :param skip_default: Флаг пропуска уведомления "по умолчания"
    :type skip_default: bool
    :return: Список уведомлений заданного алерта
    :rtype: List[Dict]
    """
    log: Logger = getLogger("yasm-client")
    notification_added: List = list()
    result: List = list()
    if not skip_default:
        notification_added.append("default")
        result.append(notifications["default"])
    for notif_match in match_list:
        if re_comp[notif_match["regex"]].fullmatch(alert):
            for notif_name in notif_match["notifications"].split(";"):
                if notif_name not in notifications:
                    log.error(
                        "Невозможно добавить в алерт %s уведомление %s, его нет в словаре уведомлений",
                        alert,
                        notif_name,
                    )
                elif notif_name not in notification_added:
                    notification_added.append(notif_name)
                    result.append(notifications[notif_name])
    return result


# --------------------------------------------------------------------------------------------------
def match_alert_limits(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern]) -> Tuple[List, List]:
    """
    Пороги для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Кортеж списков с порогами CRIT и WARN для данного алерта
    :rtype: Tuple[List, List]
    """
    crit: List = DEFAULT_CRIT
    warn: List = DEFAULT_WARN
    for limit in match_list:
        if re_comp[limit["regex"]].fullmatch(alert):
            warn = list()
            warn.append(float(limit["warn_low"]) if limit["warn_low"] != "" else None)
            warn.append(float(limit["warn_high"]) if limit["warn_high"] != "" else None)
            crit = list()
            crit.append(float(limit["crit_low"]) if limit["crit_low"] != "" else None)
            crit.append(float(limit["crit_high"]) if limit["crit_high"] != "" else None)
            break
    return warn, crit


# --------------------------------------------------------------------------------------------------
def match_flaps(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern]) -> FlapSettings:
    """
    Параметры флаподава для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Словарь, содержащий параметры флаподава
    :rtype: FlapSettings
    """
    result: FlapSettings = deepcopy(DEFAULT_FLAP)
    for flap in match_list:
        if re_comp[flap["regex"]].fullmatch(alert):
            result["critical"] = int(flap.get("critical", DEFAULT_FLAP["critical"]))
            result["stable"] = int(flap.get("stable", DEFAULT_FLAP["stable"]))
            result["boost"] = int(flap.get("boost", DEFAULT_FLAP["boost"]))
            break
    return result


# --------------------------------------------------------------------------------------------------
def match_abc(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern]) -> str:
    """
    ABC сервис для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Имя ABC сервиса, ответсвенного за алерт
    :rtype: str
    """
    abc: str = ""
    for abc_owner in match_list:
        if re_comp[abc_owner["regex"]].fullmatch(alert):
            abc = abc_owner["abc"].strip('"')
            break
    return abc


# --------------------------------------------------------------------------------------------------
def match_namespace(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern]) -> str:
    """
    Yasm namespace для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Название yasm namespace для juggler проверок алерта
    :rtype: str
    """
    namespace: str = "yasm.ambry.simple"
    for namespace_name in match_list:
        if re_comp[namespace_name["regex"]].fullmatch(alert):
            namespace = namespace_name["namespace"].strip('"')
            break
    return namespace


# --------------------------------------------------------------------------------------------------
def match_disaster(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern]) -> bool:
    """
    Истина если алерт квалифицируется как критический

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :return: Истина либо ложь
    :rtype: bool
    """
    disaster: bool = False
    for disaster_regex in match_list:
        if re_comp[disaster_regex["regex"]].fullmatch(alert):
            disaster = True
            break
    return disaster


# --------------------------------------------------------------------------------------------------
def match_tags(alert: str, match_list: List[Dict], re_comp: Dict[str, Pattern], is_disaster: bool) -> List:
    """
    Тэги для переданного алерта

    :param alert: Название алерта
    :type alert: str
    :param match_list: Список сопоставлений
    :type match_list: List[Dict]
    :param re_comp: Словарь скомпилированных регулярных выражений
    :type re_comp: Dict[str, Pattern]
    :param is_disaster: Флаг disaster
    :type is_disaster: bool
    :return: Список тэгов алерта
    :rtype: List
    """
    tags: List = list()
    for tag_list in match_list:
        if re_comp[tag_list["regex"]].fullmatch(alert):
            tags += list([x.strip() for x in tag_list["tags"].split(";")])
    if is_disaster:
        tags += ["is_disaster"]
    return tags
