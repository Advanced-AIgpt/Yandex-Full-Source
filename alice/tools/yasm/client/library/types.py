# -*- coding: utf-8 -*-
"""
Типы данных
"""
from typing import TypedDict, Dict, List


# --------------------------------------------------------------------------------------------------
class YasmWidget(TypedDict, total=False):
    """
    Виджет Голована
    """

    id: str
    width: int
    height: int
    col: int
    row: int
    type: str
    title: str
    name: str
    links: str
    text: str
    host: str
    service: str
    signals: List
    stacked: bool
    normalize: bool


# --------------------------------------------------------------------------------------------------
class FlapSettings(TypedDict):
    """
    Настройки флаподава
    """

    critical: int
    stable: int
    boost: int


# --------------------------------------------------------------------------------------------------
class ReqStatus(TypedDict, total=False):
    """
    Словарь с ответом сервера
    """

    success: bool
    obj_name: str
    url: str
    code: int
    error: str
    data: Dict
