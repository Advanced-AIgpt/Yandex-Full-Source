#!/usr/bin/env python
# encoding: utf-8
import json
import sys
from collections import OrderedDict
import re
from pprint import pprint

import yaml

# Разворачивание компактного yaml'а вида {Человекочитаемое-название: интент} в формат instruction.json

def _to_instruction(mapping):
    def _to_item((idx, (k, v))):
        name_parts = k.split('|', 1)
        name = name_parts[0].strip()
        if isinstance(v, dict):
            name += ' >'
        item = OrderedDict([
            ("name", name),
            ("instruction", name_parts[1].strip() if len(name_parts) == 2 else ""),
            ("key", "item_%s" % idx),
            ("intent", v),
            ("subitems", _to_instruction(v)),
        ])
        if not isinstance(v, basestring):
            del item['intent']
        return item

    if isinstance(mapping, basestring):
        return None

    return map(_to_item, enumerate(mapping.iteritems()))


def to_instruction(mapping):
    return {"name": "",
            "instruction": "",
            "key": "root",
            "subitems": _to_instruction(mapping)}


# === Хак для того чтобы не терять упорядоченность:
_mapping_tag = yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG

def dict_representer(dumper, data):
    return dumper.represent_dict(data.iteritems())

def dict_constructor(loader, node):
    return OrderedDict(loader.construct_pairs(node))

yaml.add_representer(OrderedDict, dict_representer)
yaml.add_constructor(_mapping_tag, dict_constructor)
# === End of hack


# Генерация json-инструкции для разметки из yaml-дерева
# Если к пункту нужен tooltip, он отделяется от названия вертикальной чертой "|"

# instr = to_instruction(yaml.load(open('navi/tbird_instr.yaml')))
# serialized = json.dumps(instr, indent=2).decode("raw_unicode_escape").encode('utf-8')
# open('navi/instruction.json', 'w').write(serialized)


def print_inst_html(mapping, intent=0):
    prefix = ' ' * intent
    print prefix + '<ul>'

    for key, val in mapping.iteritems():
        name_parts = key.split('|', 1)
        name = name_parts[0].strip()
        is_chapter = isinstance(val, dict)
        if is_chapter:
            print '%s  <li><b>%s:</b>' % (prefix, name)
            if len(name_parts) == 2:
                print '%s      <br/><small>%s</small>' % (prefix, name_parts[1])
            print_inst_html(val, intent + 4)
            print prefix + '  </li>'
        elif len(name_parts) == 2:
            print '%s  <li>%s<br/><small>%s</small></li>' % (prefix, name, name_parts[1])
        else:
            print '%s  <li>%s</li>' % (prefix, name)
    print prefix + '</ul>'


# Генерация html-дерева для вставки в инструкцию для толокеров
#print_inst_html(yaml.load(open('navi/tbird_instr.yaml')))


def get_intents(path):
    intents = set(l.split(':')[-1].strip() for l in open(path))
    intents.discard('')
    return intents


#print get_intents('navi/tbird_instr.yaml')

def validate(control_file, intents_file):
    intents = get_intents(intents_file)
    for l in open(control_file):
        i = l.split('\t')[-1].strip()
        if i and i not in intents:
            raise UserWarning('Unkonwn intent', l)


# Проверка, что в контрольной разметке нет интентов, которые отсутствуют в дереве

#validate('navi/hints.tsv', 'navi/tbird_instr.yaml')
#validate('navi/tbird_gs.tsv', 'navi/tbird_instr.yaml')

def to_honeypots(control_file, id_prefix='gs_'):
    hpots = []
    for l in open(control_file):
        l = l.strip()
        if not l:
            continue
        parts = l.rsplit('\t', 1)
        dialog = re.split(r'(?<!\\),',
                          unicode(parts[0], 'utf-8').strip().lower())
        hpots.append({
            "inputValues": {
                "reqid": "%s%s" % (id_prefix, abs(hash(l))),
                "dialog": dialog,
            },
            "knownSolutions": [{"outputValues": {"intent": parts[1].strip()}}]
        })
    return hpots


# Генерация ханипотов из контрольной разметки
# Валидационный пул делается точно так же (в таком же формате)

# hpots = to_honeypots('navi/tbird_gs.tsv')
# serialized = json.dumps(hpots, indent=2).decode("raw_unicode_escape").encode('utf-8')
# open('navi/gs.json', 'w').write(serialized)


# hpots = to_honeypots('navi/margin_hints.tsv', 'ref_')
# serialized = json.dumps(hpots, indent=2).decode("raw_unicode_escape").encode('utf-8')
# open('navi/validation.json', 'w').write(serialized)


HINTS_HEAD = 'INPUT:reqid\tINPUT:dialog\tINPUT:directive\tGOLDEN:intent\tHINT:text\n'

def _rev_hint(node, prefix, acc):
    intent = node.get('intent')
    if node['name']:
        path = prefix + [node['name']]
    else:
        path = prefix
    if intent and intent not in acc:
        acc[intent] = ' '.join(path)
    subitems = node['subitems']
    if subitems:
        for item in subitems:
            _rev_hint(item, path, acc)


def set_hints(hints_path, instr_path, out=sys.stdout):
    intent2hint = {}
    _rev_hint(json.load(open(instr_path)), prefix=[], acc=intent2hint)
    out.write(HINTS_HEAD)
    for n, line in enumerate(open(hints_path), start=1):
        dialog, intent = line.strip().split('\t')
        record = u'edu_{reqid}\t{dialog}\tunknown\t{intent}\t{text}\n'.format(
            reqid=n,
            dialog=unicode(dialog, 'utf-8'),
            intent=intent,
            text=intent2hint[intent],
        )
        out.write(record.encode('utf-8'))


# Генерация хинтов для загрузки в толоку.
# Подсказки выводятся из дерева. При необходимости - допилить напильником.

# with open('navi/toloka_edu.tsv', 'w') as out:
#    set_hints('navi/hints.tsv', 'navi/instruction.json', out)
