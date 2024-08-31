#!/usr/bin/env python
# encoding: utf-8
"""
Удаляет оценки из входного набора, чтобы получилось новое перекрытие.
Может использоваться для расчётов, насколько хорошо себя поведёт оценка, если ей уменьшить перекрытие.

Стоит иметь в виду, что если в оценки уже подмешаны ханипоты, то они так же будут урезаны до размера overlap, независимо от того, сколько раз их задавали исполнителям
"""
import json
from collections import defaultdict
from itertools import chain
from random import sample

from nirvana.job_context import context


def reduce_overlap(tasks, overlap):
    grouped = defaultdict(list)
    for t in tasks:
        grouped[t['taskId']].append(t)

    return list(chain.from_iterable(sample(g, overlap) if len(g) > overlap else g
                                    for g in grouped.itervalues()))


def main():
    ctx = context()
    inputs = ctx.get_inputs()
    outputs = ctx.get_outputs()
    params = ctx.get_parameters()

    input_tasks = json.load(open(inputs.get('inp_tasks')))

    out_tasks = reduce_overlap(input_tasks, params['overlap'])

    with open(outputs.get('reduced_tasks'), 'w') as out:
        json.dump(out_tasks, out, indent=2)


if __name__ == '__main__':
    main()

