#!/usr/bin/env python
# encoding: utf-8
import json
from hashlib import sha384
from random import shuffle


def to_hash(dct):
    """
    Вычисляет хэш от словаря (встроенная hash такое вообще запрещает)
    :param dict dct:
    :rtype: str
    """
    return sha384(json.dumps(dct, sort_keys=True)).hexdigest()


def split_tasks(records, fields, pool_id=None, group_matching=False):
    """
    Выделяет из json объекты с полями fields, остальные убирает в словарь, который потом можно будет приджойнить
    :param list[dict] records:
    :param list[basestring] fields: Названия имён, которые должны быть переданы в Толоку
    :param str|None pool_id: Если указан, будет проставлен в таски, чтобы можно было залить через кубик "upload tasks"
    :param bool group_matching: Нужно ли группировать совпадающие инпуты в список
    :return:
    """
    tasks = []
    additions = {}
    for rec in records:
        t = {}
        a = {}
        for key, val in rec.iteritems():
            if key in fields:
                t[key] = val
            else:
                a[key] = val

        hash_id = to_hash(t)
        if hash_id not in additions:
            tasks.append({'inputValues': t})
            if group_matching:
                additions[hash_id] = [a]
            else:
                additions[hash_id] = a
        elif group_matching:
            additions[hash_id].append(a)

    if pool_id:
        for t in tasks:
            t['poolId'] = pool_id

    return tasks, additions


def split_goldenset(gs, inp_fields, out_fields, pool_id=None):
    """
    Выделяет из готового голденсета подмножество для валидации, которое в пуле будет прикидываться обычными заданиями
    Так же, приводит плоскую структуру к формату, пригодному для загрузки в Толоку.
    :param list[dict] gs: Должны быть плоской структуры, если указаны inp_fields и out_fields либо готовыми ханипотами
    :param list[basestring]|None inp_fields:
    :param list[basestring]|None out_fields:
    :param str|None pool_id: Если указан, будет проставлен в ханипоты, чтобы можно было залить через кубик "upload tasks"
    :return:
    """
    if inp_fields is None and out_fields is None:
        def extract(rec):
            return rec['inputValues'], rec['knownSolutions'][0]['outputValues']
    else:
        def extract(rec):
            inp = {}
            out = {}
            for key, val in rec.iteritems():
                if key in inp_fields:
                    inp[key] = val
                elif key in out_fields:
                    out[key] = val
            return inp, out

    vld_size = min(100, (len(gs) / 10))  # Не больше 100 и не больше 10% тасков на валидацию
    shuffle(gs)

    validation_pool = []
    validation_map = {}
    for rec in gs[:vld_size]:
        inp, out = extract(rec)
        validation_map[to_hash(inp)] = out
        validation_pool.append({'inputValues': inp})

    if inp_fields is None and out_fields is None:
        honeypots = gs[vld_size:]
    else:
        honeypots = []
        for rec in gs[vld_size:]:
            inp, out = extract(rec)
            honeypots.append({
                'inputValues': inp,
                'knownSolutions': [{'outputValues': out}],
            })

    if pool_id:
        for h in honeypots:
            h['poolId'] = pool_id

        for t in validation_pool:
            t['poolId'] = pool_id

    return honeypots, validation_pool, validation_map


def join_tasks(tasks, additions, validation):
    """
    Возвращает в tasks поля, которые были из него выделены при split_tasks
    Заодно, делает структуру плоской, чтобы её можно было, например, записать в YT
    Так же, вычищает из ответов валидационный пул и считает по нему точность.
    :param list[dict] tasks:
    :param dict[str, dict|list] additions:
    :param dict[str, dict] validation:
    :return:
    """
    assigns = []
    counters = {'correct': 0, 'total': 0}

    for t in tasks:
        if t['knownSolutions']:
            continue  # Ханипоты из выхода выкидываем

        inp = t['inputValues']
        key = to_hash(inp)
        control = validation.get(key)
        if control:
            counters['total'] += 1
            if t['outputValues'] == control:
                # TODO: Отдельную точность по нескольким свойствам
                counters['correct'] += 1
            continue  # Валидацию из выходна тоже исключаем

        scores = {}
        prob = t.get('probability')
        if prob:
            for field, p in prob.iteritems():
                scores['%s_prob' % field] = p

        opinions = t.get('opinions')
        if opinions:
            for field, op in opinions.iteritems():
                scores['%s_opinions' % field] = op

        adds = additions[key]
        if not isinstance(adds, list):
            adds = [adds]

        for add in adds:
            assign = {}
            assign.update(inp)
            assign.update(add)
            assign.update(t['outputValues'])

            assign.update(scores)

            assigns.append(assign)

    if counters['total']:
        counters['accuracy'] = float(counters['correct']) / counters['total']

    return assigns, counters

