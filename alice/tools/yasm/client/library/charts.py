# -*- coding: utf-8 -*-
"""
Работа с графиками панели Голована
"""
from typing import Dict, List


# -------------------------------------------------------------------------------------------------
def gen_chart_quantiles(data: Dict) -> List:
    """
    Генерировать графики квантилей

    :param Dict data: Данные из JSON
    :type data: Dict
    :return: Список графиков
    :rtype: List
    """
    signals: List = list()
    for quantile in data["quantiles"]:
        curve: Dict = dict()
        curve["active"] = True
        curve["name"] = data["signal"].format(quantile=quantile)
        curve["title"] = data["title"].format(quantile=quantile)
        curve["host"] = data["host"]
        curve["tag"] = data["tag"]
        if "normalizable" in data:
            curve["normalizable"] = data["normalizable"]
        signals.append(curve)
    if "colors" in data:
        for item, color in zip(signals, data["colors"]):
            item["color"] = color
    if "alertNames" in data:
        for item, alert_name in zip(signals, data["alertNames"]):
            item["alertName"] = alert_name
    return signals


# --------------------------------------------------------------------------------------------------
def gen_chart_per_location(data: Dict) -> List:
    """
    Генерировать полокационные графики

    :param data: Данные из JSON
    :type data: Dict
    :return: Список графиков
    :rtype: List
    """
    signals: List = list()
    for loc in data["locations"]:
        curve: Dict = dict()
        curve["active"] = True
        curve["name"] = data["signal"].format(loc=loc)
        curve["title"] = data["title"].format(loc=loc)
        curve["host"] = data["host"]
        curve["tag"] = data["tag"].format(loc=loc)
        if data.get("normalizable", None):
            curve["normalizable"] = data["normalizable"]
        signals.append(curve)
    if "colors" in data:
        for item, color in zip(signals, data["colors"]):
            item["color"] = color
    if "alertNames" in data:
        for item, alert_name in zip(signals, data["alertNames"]):
            item["alertName"] = alert_name
    elif "alertName" in data:
        for item, loc in zip(signals, data["locations"]):
            item["alertName"] = data["alertName"].format(loc=loc)
    return signals


# --------------------------------------------------------------------------------------------------
def gen_chart_signal_list(data: Dict) -> List:
    """
    Генерировать графики по списку сигналов

    :param data: Данные из JSON
    :type data: Dict
    :return: Список графиков
    :rtype: List
    """
    signals: List = list()
    for signal, title in zip(data["signals"], data["titles"]):
        curve: Dict = dict()
        curve["active"] = True
        curve["name"] = data["signal"].format(signal=signal)
        curve["title"] = title
        curve["host"] = data["host"]
        curve["tag"] = data["tag"]
        if data.get("normalizable", None):
            curve["normalizable"] = data["normalizable"]
        signals.append(curve)
    if "colors" in data:
        for item, color in zip(signals, data["colors"]):
            item["color"] = color
    if "alertNames" in data:
        for item, alert_name in zip(signals, data["alertNames"]):
            item["alertName"] = alert_name
    return signals


# --------------------------------------------------------------------------------------------------
def render_chart(chart_data: List[Dict]) -> List:
    """
    Генерируем графики в формате дашборда Голована

    :param chart_data: Список словарей с данными для генерации
    :type chart_data: Dict
    :return: Список с данными кривых графика в формате Голована
    :rtype: List[Dict]
    """
    signals: List = list()
    for chart in chart_data:
        chart_type: str = chart.get("type", "")
        if chart_type == "quantiles":
            signals += gen_chart_quantiles(chart)
        elif chart_type == "per_location":
            signals += gen_chart_per_location(chart)
        elif chart_type == "sig_list":
            signals += gen_chart_signal_list(chart)
    return signals
