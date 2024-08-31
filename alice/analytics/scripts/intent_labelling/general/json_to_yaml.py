#!/usr/bin/env python
# encoding: utf-8
import json

# Одноразовый скрипт для перегона json-инструкции в yaml формат

def print_as_yaml(tree):
    _print_sub_yaml(tree['subitems'], indent=0)


def _print_sub_yaml(items, indent):
    prefix = ' ' * indent
    for item in items:
        name = item['name']
        tooltip = item['instruction']
        if tooltip:
            name += (' | %s' % tooltip)

        if ':' in name:
            name = '"%s"' % name.replace('"', '\\"')
        if 'intent' in item:
            print '%s%s: %s' % (prefix, name, item['intent'])
            assert item['subitems'] is None
        else:
            print '%s%s:' % (prefix, name)
            _print_sub_yaml(item['subitems'], indent+2)

if __name__ == '__main__':
    print_as_yaml(json.load(open('instruction.json')))

