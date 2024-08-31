#!/usr/bin/env python
# encoding: utf-8
import yt.wrapper
from yt.wrapper.ypath import TablePath

from cli import make_simple_client


def iter_table(tbl_path, columns=None, proxy='hahn'):
    """
    Читает таблицу из YT в потоке. То есть, не зачитывая её в память целиком
    :param str tbl_path: Путь к таблице на YT
    :param list[str]|None columns: Список колонок, которые нужно вычитать. Если не указан - читаются все.
    :param str proxy: Кластер YT
    :return: итератор из словарей, представляющих собой строки в таблице tbl_path
    """
    cli = make_simple_client()
    if columns:
        tbl_path = TablePath(tbl_path, columns=columns)
    return cli.read_table(tbl_path, raw=False, format='json')


def to_utf8(yt_string):
    """
    Преобразование строки с кириллицей из странного YT-формата
    в обычную питоно-строку закодированную в utf-8
    :param yt_string: str|unicode
    :rtype: str
    """
    if isinstance(yt_string, str):
        return yt_string
    else:
        return yt_string.encode('raw_unicode_escape')


def count_rows(table_name):
    """
    Количество строк в таблице. Если таблица не найдена, возвращает None
    :param str table_name:
    :rtype: int|None
    """
    cli = make_simple_client()
    if cli.exists(table_name):
        return cli.row_count(table_name)
    else:
        return None


def list_dir(path):
    """
    Получает список детей узла path
    :param path: Путь
    :return: список строк | None
    """
    cli = make_simple_client()
    path = path.rstrip('/')

    if cli.exists(path=path):
        return cli.list(path)
    else:
        return None
