# -*- coding: utf-8 -*-
"""
Проверка директории с конфигами алертов
"""
from typing import Dict, List
from logging import Logger, getLogger
from alice.tools.yasm.client.library.common import load_configs
from alice.tools.yasm.client.library.alerts import make_alerts


# -------------------------------------------------------------------------------------------------
def match_count_alerts_objects(data: Dict) -> Dict:
    """
    Считает совпадения при сопоставлении объектов и алертов

    :param data: Словарь с данными
    :type data: Dict
    :return: Словарь со счетчиками совпадений
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    result: Dict = dict()
    result["alerts"] = dict()
    result["objects"] = dict()
    for alert in data["alerts"]:
        regex_sections: List = ["apply_to"]
        if "apply_to" not in data["alerts"][alert]:
            log.error("Алерт %s не имеет секции apply_to", alert)
            continue
        if "dont_apply_to" in data["alerts"][alert]:
            regex_sections.append("dont_apply_to")
        if "signal" in data["alerts"][alert]:
            if data["alerts"][alert]["signal"].find(" ") != -1:
                log.warning("Сигнал алерта %s содержит пробелы", alert)
        else:
            log.error("Алерт %s не содержит сигнала", alert)
        if alert not in result["alerts"]:
            result["alerts"][alert] = dict()
            result["alerts"][alert]["total"] = 0
        for regex_section in regex_sections:
            result["alerts"][alert][regex_section] = dict()
            for regex in data["alerts"][alert][regex_section]:
                if regex not in result["alerts"][alert][regex_section]:
                    result["alerts"][alert][regex_section][regex] = 0
                for obj in data["objects"]:
                    if obj not in result["objects"]:
                        result["objects"][obj] = 0
                    if data["compiled_re"][regex].fullmatch(obj):
                        result["alerts"][alert][regex_section][regex] += 1
                        result["objects"][obj] += 1
                        result["alerts"][alert]["total"] += 1 if regex_section == "apply_to" else 0
    return result


# -------------------------------------------------------------------------------------------------
def match_count_filters(data: Dict, match_counts: Dict) -> None:
    """
    Считает совпадения при сопоставлении фильтров

    :param data: Словарь с данными
    :type data: Dict
    :param match_counts: Словарь со счетчиками совпадений
    :type match_counts: Dict
    """
    if "filters" not in match_counts:
        match_counts["filters"] = dict()
        match_counts["alert_defaults"] = dict()
    for alert in [data["made_alerts"][x]["name"] for x in data["made_alerts"]]:
        for matcher in [
            "alert_limits",
            "notifications",
            "flaps",
            "disaster",
            "abc",
            "namespace",
            "tags",
        ]:
            if data["filters"][matcher]:
                if matcher not in match_counts["filters"]:
                    match_counts["filters"][matcher] = dict()
                if "alert_default_limits" not in match_counts:
                    match_counts["alert_default_limits"] = list()
                match_count: bool = False
                for regex in [x["regex"] for x in data["filters"][matcher] if x]:
                    if regex not in match_counts["filters"][matcher]:
                        match_counts["filters"][matcher][regex] = 0
                    if data["compiled_re"][regex].fullmatch(alert):
                        match_counts["filters"][matcher][regex] += 1
                        match_count = matcher == "alert_limits"
                        break
                if match_count:
                    match_counts["alert_default_limits"].append(alert)


# -------------------------------------------------------------------------------------------------
def pprint_match_counts(match_counts: Dict) -> None:
    """
    Вывести в логи результаты проверки

    :param match_counts: Словарь со счетчиками срабатывания
    :type match_counts: Dict
    """
    log: Logger = getLogger("yasm-client")
    for obj in [x for x in match_counts["objects"] if match_counts["objects"][x] == 0]:
        log.error("Объект %s не имеет алертов", obj)
    for alert in match_counts["alerts"]:
        regex_sections: List = ["apply_to"]
        if "dont_apply_to" in match_counts["alerts"][alert]:
            regex_sections.append("dont_apply_to")
        if match_counts["alerts"][alert]["total"] == 0:
            log.error("Алерт %s не применен ни к одному объекту", alert)
        for regex_section in regex_sections:
            for regex in match_counts["alerts"][alert][regex_section]:
                if match_counts["alerts"][alert][regex_section][regex] == 0:
                    log.warning(
                        'Алерт %s, секция %s, регулярное выражение "%s" не имеет совпадений',
                        alert,
                        regex_section,
                        regex,
                    )

    for matcher in [
        "alert_limits",
        "notifications",
        "flaps",
        "disaster",
        "abc",
        "namespace",
        "tags",
    ]:
        if match_counts["filters"].get(matcher):
            if matcher not in match_counts["alert_defaults"]:
                match_counts["alert_defaults"][matcher] = list()
            for regex in match_counts["filters"][matcher]:
                if match_counts["filters"][matcher][regex] == 0:
                    log.warning('Лимит %s, регулярное выражение "%s" не имеет совпадений', matcher, regex)
    log.debug(
        "Алерты со значением %s по умолчанию:\n%s",
        matcher,
        "\n".join(sorted(match_counts["alert_default_limits"])),
    )


# -------------------------------------------------------------------------------------------------
def verify_main(data: Dict) -> bool:
    """
    Основная функция проверки

    :param data: Данные для проверки
    :type data: Dict
    :return: Успешно или нет
    :rtype: bool
    """
    log: Logger = getLogger("yasm-client")
    if not data["config_dir"]:
        log.critical("Не указана директория с конфигами")
        return False
    if not load_configs(data):
        log.critical("Не удалось загрузить конфиги из %s", data["config_dir"])
        return False
    make_alerts(data)
    match_counts: Dict = match_count_alerts_objects(data)
    match_count_filters(data, match_counts)
    pprint_match_counts(match_counts)
    return True
