"""
Функции парсера pipe-фильтра для push-client
"""
# coding: utf-8
from json import dumps as j_dumps, load as j_load
from time import altzone, daylight, localtime, timezone, strftime
from typing import Dict, List, Tuple, Any

ATTRS_FILE: str = "dump.json"
SANE_TAGS: Dict[str, Tuple] = {
    'tags': ('itype', 'ctype', 'prj', 'metaprj'),
    'itype': ('uniproxy', 'unidelivery', 'asr', 'asrgpu', 'tts', 'ttsgpu'),
    'ctype': ('prod', 'beta', 'testing', 'yappy', 'hamster'),
    'metaprj': ('alice', 'voicetech')
}
DEFAULT_TAGS: Dict[str, str] = {
    "itype": "uniproxy",
    "ctype": "prod",
    "prj": "uniproxy",
    "metaprj": "alice"
}


def get_timezone() -> str:
    """
    Локальная зона в формате +0000
    :return: Зона в формате +0000
    :rtype: str
    """
    tz_raw: int = altzone if daylight and localtime().tm_isdst > 0 else timezone
    return '{}{:0>2}{:0>2}'.format(
        '-' if tz_raw > 0 else '+',
        abs(tz_raw) // 3600,
        abs(tz_raw // 60) % 60
    )


def parse_tags(tag_string: str) -> Dict[str, str]:
    """
    Разбор строки с тэгами из файла
    :param str tag_string: Строка с тэгами вида a_itype_uniproxy через пробле
    :return: Словарь с yasm тэгами
    :rtype: Dict[str, str]
    """
    properties_tags: List[str] = tag_string.split(" ")
    result: Dict[str, str] = DEFAULT_TAGS.copy()
    for raw_tag in properties_tags:
        tag: List = raw_tag.split('_')
        if tag[0] == "a":
            result[tag[1]] = tag[2]
    return result


def sanitize_tags(tag_dict: Dict[str, str]) -> Dict[str, str]:
    """
    Проверка тэгов на вхождение в допустимые диапазоны
    :param Dict[str, str] tag_dict: Словарь с yasm тэгами
    :return: Словарь с проверенными yasm тэгами
    :rtype: Dict[str, str]
    """
    for tag in list(tag_dict):
        if tag not in SANE_TAGS['tags']:
            tag_dict.pop(tag)
    for tag in ('itype', 'ctype', 'metaprj'):
        if tag_dict[tag] not in SANE_TAGS[tag]:
            tag_dict[tag] = DEFAULT_TAGS[tag]
    return tag_dict


def get_tags(filename: str) -> Dict[str, str]:
    """
    Чтение yasm тэгов из файла
    :param str filename: Имя файла с тэгами
    :return: Словарь с yasm тэгами
    :rtype: Dict[str, str]
    """
    tags: Dict[str, str] = dict()
    try:
        with open(filename) as dumpjson:
            dump: Dict = j_load(dumpjson)
        tags = parse_tags(dump['properties']['tags'])
    except IOError:
        tags = DEFAULT_TAGS
    except KeyError:
        tags = DEFAULT_TAGS
    return sanitize_tags(tags)


def log_line_structured(
        message: str,
        tags: Dict[str, str],
        row_id: int,
        hostname: str,
        host_timezone: str
) -> str:
    """
    Структурируем данные в сериализованный JSON
    :param str message: Сообщение из лога
    :param Dict[str, str] tags: Словарь yasm тэгов
    :param int row_id: Идентификатор строки из push-client
    :param str hostname: Имя хоста
    :param str host_timezone: Зона в формате +0000
    :return: Сериализованный JSON
    :rtype: str
    """
    result: Dict[str, Any] = {
        'pushclient_row_id': row_id,
        'level': 20000,
        'levelStr': 'INFO',
        'loggerName': 'stdout',
        '@version': 1,
        'threadName': 'qloud-init',
        '@timestamp': strftime("%Y-%m-%dT%H:%M:%S", localtime()) + host_timezone,
        'qloud_project': tags['metaprj'],
        'qloud_application': tags['itype'],
        'qloud_environment': tags['ctype'],
        'qloud_component': tags['prj'],
        'qloud_instance': '-',
        'message': message.strip('\n'),
        'host': hostname
    }
    return j_dumps(result).strip()
