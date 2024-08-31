"""
Тест преобразования строки лога
"""
# coding: utf-8
from typing import Dict, Any
from time import localtime, strftime
from alice.tools.qloud_format.lib.func import log_line_structured


def test_log_structured():
    """
    Тестируем структурирование в JSON строки лога
    """
    in_message: str = 'Just some log message'
    in_tags: Dict[str, Any] = {
        "prj": "uniproxy",
        "itype": "uniproxy",
        "ctype": "prod",
        "metaprj": "alice"
    }
    in_row_id: int = 100500
    in_hostname: str = 'myhostname'
    in_host_timezone: str = '+0300'
    out_str: str = '{"pushclient_row_id": 100500, "level": 20000, "levelStr": "INFO", '+\
        '"loggerName": "stdout", "@version": 1, "threadName": "qloud-init", "@timestamp": "' +\
        strftime("%Y-%m-%dT%H:%M:%S", localtime()) + '+0300", "qloud_project": "alice",' +\
        ' "qloud_application": "uniproxy", "qloud_environment": "prod", "qloud_component":' +\
        ' "uniproxy", "qloud_instance": "-", "message": "Just some log message", ' +\
        ' "host": "myhostname"}'.strip()
    assert log_line_structured(
        in_message,
        in_tags,
        in_row_id,
        in_hostname,
        in_host_timezone
    ) == out_str
