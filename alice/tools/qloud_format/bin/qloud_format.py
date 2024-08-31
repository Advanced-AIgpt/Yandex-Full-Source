"""
Pipe-фильтр логов для push-client
"""
# coding: utf-8
from os import uname
from sys import stdin, stdout
from typing import Dict
from alice.tools.qloud_format.lib.func import ATTRS_FILE, get_timezone, get_tags
from alice.tools.qloud_format.lib.func import log_line_structured


def main():
    """
    Основная функция
    """
    tags: Dict[str, str] = get_tags(ATTRS_FILE)
    host_timezone: str = get_timezone()
    hostname: str = uname()[1]
    while True:
        try:
            line: str = stdin.readline()
        except UnicodeDecodeError:
            continue
        if line == '':
            break
        try:
            secret, row_id, offset, message = line.split(";", 3)
        except ValueError:
            continue
        log_line: str = log_line_structured(
            message,
            tags,
            row_id,
            hostname,
            host_timezone
        )
        print(f'{secret};{row_id};{offset};{log_line}')
        stdout.flush()


if __name__ == '__main__':
    main()
