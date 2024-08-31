#!/usr/bin/env python
# encoding: utf-8
from collections import defaultdict
from operator import itemgetter
from math import log10
import json

from utils.nirvana.op_caller import call_as_operation


def weighted_mv(assignments, resp_key=None, all_opinions=False):
    """
    Агрегация Majority Vote, усовершенствованная оценкой точности исполнителей
      Оценка считается по ханипотам на всём пуле.
      Так же, учитывается оценка точности на конкретных категориях ответов, чтобы исключить влияние "любимых пунктов" у исполнителей.
    :param list[dict] assignments: Ответы в формате кубика Get Assignments
    :param list[str]|str|None resp_key: Ключ, или ключи в которые записана разметка
         Если не задан, будет вытащен автоматически из самих ответов.
    :param bool all_opinions: Нужно ли кроме победителя выгружать полный список мнений с их вероятностями
    :return: В целом, аналогичен выдаче агрегатора David-Skene
    """
    if not assignments:
        return []

    if resp_key is None:
        resp_key = assignments[0]['outputValues'].keys()
        if len(resp_key) == 1:
            resp_key = resp_key[0]

    if isinstance(resp_key, basestring):
        weights = workers_accuracy(assignments, resp_key)
        aggr = mv_on_key(assignments, resp_key, weights, all_opinions)
        return aggr.values()

    aggrs = []
    for key in resp_key:
        weights = workers_accuracy(assignments, key)
        aggrs.append(mv_on_key(assignments, key, weights, all_opinions))

    first_aggr = aggrs[0]
    tail_aggrs = zip(resp_key[1:], aggrs[1:])
    for task_id, task in first_aggr.iteritems():
        for key, aggr in tail_aggrs:
            other_task = aggr[task_id]
            task['probability'].update(other_task['probability'])
            task['outputValues'].update(other_task['outputValues'])
            if all_opinions:
                task['opinions'].update(other_task['opinions'])

    return first_aggr.values()


def mv_on_key(assignments, resp_key, worker_weights=None, all_opinions=False):
    tasks = {}
    for assign in assignments:
        task_id = json.dumps(assign['inputValues'], sort_keys=True)  # На дублирующиеся таски даём один ответ
        task = tasks.get(task_id)
        if task is None:
            task = {
                'inputValues': assign['inputValues'],
                'knownSolutions': assign['knownSolutions'],
                'votes': defaultdict(list)
            }
            tasks[task_id] = task

        key = assign['outputValues'][resp_key]
        if worker_weights is None or assign['workerId'] not in worker_weights:
            task['votes'][key].append(1)
        else:
            skill = worker_weights[assign['workerId']]
            all = skill['all']
            cat = skill['categories'].get(key)
            if cat is None:
                task['votes'][key].append(all['accuracy'])
            else:
                incr = ((all['accuracy'] * all['significance']
                         + cat['accuracy'] * cat['significance']) /
                        (all['significance'] + cat['significance']))
                task['votes'][key].append(incr)

    def count_votes(task):
        votes = task.pop('votes')
        all_weights = sum(votes.itervalues(), [])  # Все веса в "плоском" виде
        total = sum(all_weights)
        # Вероятность того, что хоть какой-нибудь из выданных ответов правилен:
        some_right = 1 - reduce(lambda acc, v: acc * (1 - v), all_weights, 1.0)
        if total != 0:
            opinions = {op: some_right * (sum(weights) / total)
                        for op, weights in votes.iteritems()}
        else:
            opinions = {op: 0.0
                        for op, weights in votes.iteritems()}
        # Наиболее правильный ответ:
        major_solution, major_weight = max(opinions.iteritems(), key=lambda x: x[1])

        task['outputValues'] = {resp_key: major_solution}
        task['probability'] = {resp_key: major_weight}
        if all_opinions:
            task['opinions'] = {resp_key: opinions}
        return task

    return {task_id: count_votes(task) for task_id, task in tasks.iteritems()}


def workers_accuracy(assignments, resp_key):
    workers = defaultdict(lambda: {
        'all': {'correct': 0.0, 'total': 0.0},
        'categories': defaultdict(lambda: {'correct': 0.0, 'total': 0.0})
    })
    for assign in assignments:
        known = assign['knownSolutions']
        if not known:
            continue
        result = assign['outputValues'][resp_key]
        skill = workers[assign['workerId']]
        all = skill['all']
        cat = skill['categories'][result]
        all['total'] += 1
        cat['total'] += 1
        if result in (k['outputValues'][resp_key] for k in known):
            all['correct'] += 1
            cat['correct'] += 1

    def to_weighted(counters):
        return {'accuracy': counters['correct'] / counters['total'],
                'significance': log10(counters['total'] + 1)}

    return {w_id: {'all': to_weighted(w_skill['all']),
                   'categories': {cat: to_weighted(cnt)
                                  for cat, cnt in w_skill['categories'].iteritems()}}
            for w_id, w_skill in workers.iteritems()}


if __name__ == '__main__':
    call_as_operation(weighted_mv, input_spec={
        "assignments": {"parser": "json"},
    })
