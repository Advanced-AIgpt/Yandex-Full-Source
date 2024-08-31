#!/usr/bin/env python
# encoding: utf-8
import contextlib
from uuid import uuid4

import yt.wrapper

from cli import make_simple_client


@contextlib.contextmanager
def buffer_table(trg_table, schema=None, prefix='//home/voice/tmp'):
    """
    Генерит имя для временной таблицы и после выхода из контекста перемещает её в trg_table.
    Используется в случае, если нельзя сразу писать в trg_table.
    Например, когда в trg_table лежат исходные данные, которые нельзя очищать перед стартом джобов
    Вместо:
    ```some_actions(output='//home/username/write_here')```
    пишем:
    ```
    with buffer_table('//home/username/write_here') as tmp_table:
        some_actions(output=tmp_table)
    ```
    :param str trg_table: Таблица, в которую планируется выгружать результат
    :param list[dict] schema: Схема в формате yt (не qb2!).
        Имеет смысл указывать только если её не перезаписывают другие джобы.
    :param str prefix: YT-директория для временной таблицы
    """
    tmp_table = '%s/%s' % (prefix, uuid4().hex)
    cli = make_simple_client()
    if schema:
        cli.create('table', tmp_table, attributes={'schema': schema})

    yield tmp_table

    # При неотловленных исключениях, выполнение досюда не доходит. Обычно это то, что и требуется.
    if yt.wrapper.exists(tmp_table, client=cli):
        yt.wrapper.move(tmp_table, trg_table, force=True, client=cli)
