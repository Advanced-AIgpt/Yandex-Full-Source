"""
Тест функции генерации таймзоны в формате +0000
"""
# coding: utf-8
from time import altzone, daylight, timezone, localtime
from alice.tools.qloud_format.lib.func import get_timezone

TZ_RAW: int = altzone if daylight and localtime().tm_isdst > 0 else timezone
TZ: str = '{}{:0>2}{:0>2}'.format(
    '-' if TZ_RAW > 0 else '+',
    abs(TZ_RAW) // 3600,
    abs(TZ_RAW // 60) % 60
)


def test_timezone():
    """
    Тестируем генератор таймзоны в формате +0000
    """
    assert get_timezone() == TZ
