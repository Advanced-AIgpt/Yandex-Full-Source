#!/usr/bin/env python
# encoding: utf-8
from os.path import join
from itertools import islice, chain
from utils.clickhouse.requesting import insert_req, post_req


def iter_chunks(iterable, chunk_size):
    # Группирует поток iterable в чанки по chunk_size штук
    # Нормально работает только если вызывающий код проходится по всему чанку перед тем как запросить следующий
    slice_size = chunk_size - 1
    while True:
        yield chain([iterable.next()], islice(iterable, slice_size))


def copy_table(iter_lines, src_root, date, trg_table, slice=None, chunk_size=30000, replace=False):
    """
    Копирование данных из YT-таблицы в Clickhouse таблицу.
    :param iter_lines: Генератор строк таблицы src_root
    :param str src_root: YT-директория с табличками
    :param str date: Дата, за которую сделать копирование.
        Используется и как имя исходной таблицы и как имя партиции в Кликхаус
    :param str trg_table: Название таблицы в CH
    :param str|None slice: Спецификация строк, которые вычитывать из YT-таблицы
        Формат '[#<номер_первой_строки>:#<номер_последней_строки_не_включительно>]'
    :param int chunk_size: Количество строк, которые пишем в CH за один запрос.
    :param bool replace: Перезаписывать ли в данные за эту дату, если они уже были в CH.
    :return: None
    """
    if replace:
        post_req("ALTER TABLE %s DROP PARTITION '%s'" % (trg_table, date))

    if slice is None:
        src_path = join(src_root, date)
    else:
        src_path = join(src_root, date + slice)

    lines = iter_lines(src_path)
    for chunk in iter_chunks(lines, chunk_size):
        insert_req(trg_table, ''.join(chain.from_iterable(chunk)))


# Очистка ранее созданных партиций
GET_PARTITIONS_QUERY = """
SELECT DISTINCT partition FROM system.parts
WHERE table = '{table_name}' AND 
    (max_date < (toDate('{end_date}') - {expire_days})
     OR 
     (min_date >= toDate('{start_date}') AND max_date <= toDate('{end_date}')) 
    )
"""


def clean_partitions(table_name, start_date, end_date, expire_days=15):
    # Удаление партиций, которые сейчас будут (пере)записаны, а также утративших актуальность.
    query = GET_PARTITIONS_QUERY.format(**locals())
    partitions = [line.strip("\\'")
                  for line in post_req(query).strip().split('\n')
                  if line]
    for p in partitions:
        post_req("ALTER TABLE {tbl} DROP PARTITION '{part}'".format(tbl=table_name, part=p))
    return partitions


# Финализация партиций
FINALIZE_QUERY = """OPTIMIZE TABLE {table} PARTITION '{date}'"""
