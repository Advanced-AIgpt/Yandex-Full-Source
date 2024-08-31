#!/usr/bin/env python
# encoding: utf-8
from itertools import izip
from urllib2 import urlopen
from urllib import quote_plus
import json

from utils.yt.reader import iter_table, to_utf8
from utils.yt.writer import write_table


#https://wiki.yandex-team.ru/misspell/api/#3.jsonwebservisixml
URL = 'http://misc-spell-dev.yandex.net:19036/misspell.json/check'
OPTIONS = 'srv=voice&ie=utf8&lang=ru&options=0'


def iter_amends(src_table, batch_size=50):
    s = iter_table(src_table, columns=['cnt', 'txt'])
    rows = []
    for i, row in enumerate(s):
        if i % 1000 == 0:
            print '%sk' % (i / 1000)
        row['txt'] = to_utf8(row['txt'])
        rows.append(row)
        if i % batch_size == (batch_size - 1):
            update_with_amends(rows)
            for r in rows:
                yield r

            rows = []

    if rows:
        update_with_amends(rows)
        for r in rows:
            yield r


def update_with_amends(rows, retries=5, min_reliability=8000):
    args = OPTIONS + ''.join('&text=%s' % quote_plus(r['txt']) for r in rows)

    for attempt in xrange(retries):
        try:
            responses = json.load(urlopen(URL, data=args))
            break
        except Exception:
            if attempt == (retries - 1):
                raise

    for row, resp in izip(rows, responses):
        if resp['r'] >= min_reliability:
            row['corrected'] = resp['text']
        else:
            row['corrected'] = row['txt']


def set_misspells(src_table, trg_table):
    write_table(trg_table,
                iter_amends(src_table=src_table),
                schema=[
                   {"name": "txt", "type": "string", "required": True},
                   {"name": "corrected", "type": "string", "required": True},
                   {"name": "cnt", "type": "uint32", "required": True},
               ],
                replace=True)


if __name__ == '__main__':
    set_misspells(src_table='//home/voice/yoschi/top_suggest/filtered_18k',
                  trg_table='//home/voice/yoschi/top_suggest/corrected_18k')
