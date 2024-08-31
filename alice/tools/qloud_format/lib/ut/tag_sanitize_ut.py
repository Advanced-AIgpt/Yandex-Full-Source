"""
Тест проверки тэгов
"""
# coding: utf-8
from typing import Dict
from alice.tools.qloud_format.lib.func import sanitize_tags


def test_remove_unwanted_tags():
    """
    Тестируем удаление лишнего тэга
    """
    in_tags: Dict[str, str] = {
        "dc": "man",
        "itype": "uniproxy",
        "ctype": "prod",
        "prj": "uniproxy",
        "metaprj": "alice"
    }
    out_tags: Dict[str, str] = {
        "itype": "uniproxy",
        "ctype": "prod",
        "prj": "uniproxy",
        "metaprj": "alice"
    }
    assert sanitize_tags(in_tags) == out_tags


def test_change_to_defaults():
    """
    Тестируем сброс тэгов в дефолтные значения
    """
    in_tags: Dict[str, str] = {
        "itype": "some_itype",
        "ctype": "ttttttesting",
        "prj": "uniproxy",
        "metaprj": "unknown"
    }
    out_tags: Dict[str, str] = {
        "itype": "uniproxy",
        "ctype": "prod",
        "prj": "uniproxy",
        "metaprj": "alice"
    }
    assert sanitize_tags(in_tags) == out_tags
