#!/usr/bin/env python
# encoding: utf-8
from random import shuffle

from utils.nirvana.op_caller import call_as_operation

from utils.toloka.std_prj import StdProject
from utils.toloka.split import split_goldenset, split_tasks


# Используется в "Run Toloka Std Pool"
# https://nirvana.yandex-team.ru/operation/e4ba3f74-69f2-469e-be60-99bfc80335af/subprocess

def mix_split(records, gs_records, is_sandbox, prj_id, pool_id=None, **other_settings):
    prj = StdProject(is_sandbox=is_sandbox, prj_id=prj_id)
    inp_fields, out_fields = prj.get_field_names()
    tasks, additions = split_tasks(records, inp_fields, pool_id)
    honeypots, validation_pool, validation_map = split_goldenset(gs_records, inp_fields, out_fields, pool_id)
    tasks.extend(validation_pool)
    shuffle(tasks)
    return {
        'tasks': tasks,
        'additions': additions,
        'honeypots': honeypots,
        'validation': validation_map,
    }


if __name__ == '__main__':
    call_as_operation(mix_split, {
        'records': {'link_name': 'records', 'parser': 'json'},
        'gs_records': {'link_name': 'gs_records', 'parser': 'json'},
    })
