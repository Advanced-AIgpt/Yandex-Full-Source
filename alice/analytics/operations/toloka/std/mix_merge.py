#!/usr/bin/env python
# encoding: utf-8
from utils.nirvana.op_caller import call_as_operation

from utils.toloka.split import join_tasks


# Используется в "Run Toloka Std Pool"
# https://nirvana.yandex-team.ru/operation/3d255a9c-dc17-45b0-9097-02dd700439a8/subprocess

def mix_merge(tasks, splitted=None, additions=None, validation=None):
    """

    :param tasks: Агрегированные ответы на задания
    :param dict[str, dict|list] splitted: выход скрипта "mix_split".
      Если не задан, должны быть заданы уже разделённые additions и validation
    :return:
    """
    if splitted is not None:
        additions = splitted['additions']
        validation = splitted['validation']
    assigns, counters = join_tasks(tasks, additions, validation)
    return {
        'records': assigns,
        'stat': counters,
    }


if __name__ == '__main__':
    call_as_operation(mix_merge, {
        'tasks': {'link_name': 'tasks', 'parser': 'json'},
        'splitted': {'link_name': 'splitted', 'parser': 'json', "required": False},
        'additions': {'link_name': 'additions', 'parser': 'json', "required": False},
        'validation': {'link_name': 'validation', 'parser': 'json', "required": False},
    })

