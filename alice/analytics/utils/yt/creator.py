#!/usr/bin/env python
# encoding: utf-8
from cli import make_simple_client


def make_directory(path, client=None):
    """
    Создаёт YT-директорию, если она ещё не создана
    :param path: str
    :param client: yt.wrapper.YtClient
    :return: uuid созданной или найденной директории
    """
    if client is None:
        client = make_simple_client()

    path = path.rstrip('/')

    if client.exists(path=path):
        return client.get(path='%s/@id' % path)
    else:
        return client.create(path=path, type='map_node')


def make_table(path, schema, replace=False):
    """
    Создаёт таблицу по указанному пути с указанной схемой.
    :param str path: Путь к таблице
    :param list[dict] schema: Схема в обычном ытёвом формате
    :param bool replace: Заменять ли существующую таблицу.
        Если False и таблица уже существует, она останется нетронутой.
        Схема в этом случае тоже не изменится.
    :rtype: str|None
    :return: uuid созданной таблицы, или None, если таблица уже существовала
    """
    cli = make_simple_client()
    if cli.exists(path):
        if replace:
            cli.remove(path)
        else:
            return
    return cli.create('table', path, attributes={'schema': schema})
