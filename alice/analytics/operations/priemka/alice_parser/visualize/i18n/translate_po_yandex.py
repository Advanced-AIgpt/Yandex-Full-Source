#!/usr/bin/python

import os
import sys

import urllib
import requests


def translate(source, lang):
    url = 'http://translate.yandex.net/api/v1/tr.json/translate?srv=alice-analytics&lang=' + lang + '&text=' + urllib.parse.quote_plus(source)
    r = requests.get(url)
    return r.json()['text'][0]


print('run python translate file: {}'.format(sys.argv[1]))
LANGUAGE = sys.argv[1].split('/')[-3]

fout = open(sys.argv[1] + '.translated', 'w')
with open(sys.argv[1]) as f:
    text_ru = None
    for line in f:
        if 'msgid' in line and 'msgid ""' not in line:
            text_ru = line.strip()[7:-1]
        elif 'msgstr ""' in line and text_ru is not None:
            text_ar = translate(text_ru, 'ru-{LANGUAGE}'.format(LANGUAGE=LANGUAGE))
            fout.write('msgstr "{}"\n'.format(text_ar))
            continue
        fout.write(line)
        continue

fout.close()
os.rename(sys.argv[1] + '.translated', sys.argv[1])
