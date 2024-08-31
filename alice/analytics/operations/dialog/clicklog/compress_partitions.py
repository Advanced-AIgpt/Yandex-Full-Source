#!/usr/bin/env python
# encoding: utf-8
import time

from utils.nirvana.op_caller import call_as_operation
from utils.clickhouse.requesting import post_req


QUERY_FIND_LARGEST = """
SELECT
    argMax(partition, cnt) as largest_part,
    max(cnt) as largest_size,
    sum(cnt) / count() as avg_size,
    sum(cnt) as total,
    max(mod_time) as last_mod_time,
    (now() - MAX(mod_time)) / 60 as last_mod_age -- in minutes
FROM
(
SELECT partition, COUNT() as cnt, MAX(modification_time) as mod_time
FROM system.parts
WHERE table = '{table}' AND active == 1
GROUP BY partition, active
ORDER BY partition
)
"""


def find_largest(table='dialogs'):
    return post_req(QUERY_FIND_LARGEST.format(table=table), as_json=True)['data'][0]



QUERY_OPTIMIZE_PARTITION = """
OPTIMIZE TABLE {table} PARTITION '{partition}' FINAL
"""


def optimize_partition(partition, table='dialogs'):
    """
    :param str|unicode partition:
    :param str|unicode table:
    :return:
    """
    query = QUERY_OPTIMIZE_PARTITION.format(table=table, partition=partition.strip("'"))
    return post_req(query)


def optimize_largest(table='dialogs', standby_minutes=30):
    """
    Запустить оптимизацию (сжатие) самой большой партиции в таблице `table`
    param str|unicode table:
    :param float|int standby_minutes: Время ожидания с момента последнего изменения в партициях
         Устанавливается на случай, если прямо сейчас уже идут оптимизации или вставка данных
         Если установить в ноль, то оптимизация будет запущена в любом случае. Но это может не дать эффекта.
    :return:
    """
    info = find_largest(table=table)
    print '{}: partitions info {}'.format(time.strftime('%H:%M:%S'), info)
    if info['last_mod_age'] < standby_minutes:
        print 'standby'
        return {'status': 'standby', 'info': info}
    elif int(info['largest_size']) < 3:
        print 'optimized enough'
        return {'status': 'pass', 'info': info}
    else:
        part = info['largest_part']
        print '{}: optimize {}...'.format(time.strftime('%H:%M:%S'), part)
        optimize_partition(part, table=table)
        print '{}: finished'.format(time.strftime('%H:%M:%S'))
        return {'status': 'optimized', 'info': info}


if __name__ == '__main__':
    call_as_operation(optimize_largest)
