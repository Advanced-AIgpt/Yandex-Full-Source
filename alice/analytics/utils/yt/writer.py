#!/usr/bin/env python
# encoding: utf-8
import yt.wrapper
from yt.wrapper.ypath import TablePath

from cli import make_simple_client
from creator import make_table


def write_table(path, iterator, schema=None, replace=True):
    """
    Пишет в таблицу данные из потока
    :param str path: Путь к YT-таблице
    :param Iterable[dict] iterator: Итератор по словарям, представляющим собой строки для таблицы
    :param list[dict]|None schema: Схема в обычном YT-формате
    :param bool replace: True, если содержимое существующей таблицы нужно переписать заново.
        False, если строки нужно дописывать (append mode).
    :return:
    """
    if schema:
        make_table(path, schema=schema, replace=replace)

    if not replace:
        path =TablePath(path, append=True)

    cli = make_simple_client()
    cli.write_table(path, iterator,
                    format=yt.wrapper.JsonFormat(attributes={"encode_utf8": False}),
                    raw=False,
                    force_create=replace)

