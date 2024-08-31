#!/bin/env python

import sys
import json
import codecs

fixlist_in = sys.argv[1]
fixlist_out = sys.argv[2] if len(sys.argv) > 2 else 'navigation_fixlist.json'


def make_node(queries, parts):
    gplay = parts[0]
    itunes = parts[1]
    node = {
        'queries': queries,
        'data': {
            'nav': {
                'text': parts[4],
                'app': {
                    'gplay': gplay,
                    'itunes': itunes
                },
                'url': {
                    'desktop': parts[3],
                    '_': parts[2]
                }
            },
            'suggests': {
                'serp': {
                    'text': parts[5] if len(parts) > 5 else parts[4]
                }
            }
        }
    }
    if gplay == '':
        node['data']['nav']['app'].pop('gplay')
    if itunes == '':
        node['data']['nav']['app'].pop('itunes')
    if gplay == '' and itunes == '':
        node['data']['nav'].pop('app')
    return node


with codecs.open(fixlist_in, 'r', encoding='utf-8') as fin,\
        codecs.open(fixlist_out, 'w', encoding='utf-8') as fout:
    fout.write('[\n')
    i = 0
    queries = []
    common_parts = []
    for line in fin:
        i = i + 1
        if i == 1:
            continue
        parts = line.strip('\r\n').split('\t')
        query = parts[0].strip()
        if common_parts == parts[1:]:
            queries.append(query)
        else:
            if len(queries) != 0:
                node = make_node(queries, common_parts)
                fout.write(json.dumps(node, ensure_ascii=False, indent=2))
                fout.write(',\n')
            queries = [query]
            common_parts = parts[1:]
    if len(queries) != 0:
        node = make_node(queries, common_parts)
        fout.write(json.dumps(node, ensure_ascii=False, indent=2))
    fout.write('\n]')
    print i
