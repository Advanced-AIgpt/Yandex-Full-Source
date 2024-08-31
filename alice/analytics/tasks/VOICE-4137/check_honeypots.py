#!/usr/bin/env python
# encoding: utf-8
import json
# Проверка на корректность ханипотов, с учётом задания
# По сути, обратный процесс их выбору


HONEYPOTS_PATH = 'tmp/results/honeypots.json'
ASSIGNMENTS_PATH = 'tmp/assignments_2.json'

HONEY_MAP = {h['inputValues']['url']: h['outputValues']
             for h in json.load(open(HONEYPOTS_PATH), encoding='utf-8')}


for assignment in json.load(open(ASSIGNMENTS_PATH), encoding='utf-8'):
    url = assignment['inputValues']['url']
    try:
        ref = HONEY_MAP[url]
    except KeyError:
        continue
    else:
        values = assignment['outputValues']
        for key, ref_v in ref.iteritems():
            assert values[key].lower().replace(u'ё', u'е') == ref_v.lower().replace(u'ё', u'е'), '{} != {} on key {}'.format(values, ref, key)
