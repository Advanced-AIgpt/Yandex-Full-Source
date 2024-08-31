#!/usr/bin/env python
# encoding: utf-8

# Схемы для таблиц с регулярной разметкой на естественность и продуктовые свойства ответа болталки
# На данный момент, устанавливаются операциями YT put и MR Sort в соответствующих кубиках
# https://nirvana.yandex-team.ru/operation/5c2ab4c4-16aa-4289-adfe-66c935021288/subprocess
# https://nirvana.yandex-team.ru/operation/b85be283-d2b8-4727-a320-7151e97a4a32/subprocess


REPLY_PROPERTIES = [
    {"name": "context_2", "type": "string", "sort_order": "ascending"},
    {"name": "context_1", "type": "string", "sort_order": "ascending"},
    {"name": "context_0", "type": "string", "sort_order": "ascending"},
    {"name": "reply", "type": "string"},
    {"name": "male", "type": "string"},
    {"name": "male_prob", "type": "double"},
    {"name": "rude", "type": "string"},
    {"name": "rude_prob", "type": "double"},
    {"name": "you", "type": "string"},
    {"name": "you_prob", "type": "double"},
    {"name": "source", "type": "string"},
    {"name": "key", "type": "string"},
    {"name": "submitTs", "type": "string"},
]


REPLY_NATURE = [
    {"name": "context_2", "type": "string", "sort_order": "ascending"},
    {"name": "context_1", "type": "string", "sort_order": "ascending"},
    {"name": "context_0", "type": "string", "sort_order": "ascending"},
    {"name": "reply", "type": "string"},
    {"name": "result", "type": "string"},
    {"name": "probability", "type": "double"},
    {"name": "source", "type": "string"},
    {"name": "key", "type": "string"},
    {"name": "submitTs", "type": "string"},
]


def to_qb2_fmt(schema):
    fmt = {'string': 'str', 'double': 'float'}
    for r in schema:
        print '"%s": %s,' % (r['name'], fmt[r['type']])


#to_qb2_fmt(REPLY_NATURE)
