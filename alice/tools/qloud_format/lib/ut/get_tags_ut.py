"""
Тест чтения тэгов из dump.json
"""
# coding: utf-8
from typing import Dict
from alice.tools.qloud_format.lib.func import get_tags


def test_get_tags():
    """
    Тестируем получение тэгов из dump.json
    """
    tags: Dict[str, str] = {
        "itype": "uniproxy",
        "ctype": "prod",
        "prj": "uniproxy",
        "metaprj": "alice"
    }
    assert get_tags('dump.json') == tags
