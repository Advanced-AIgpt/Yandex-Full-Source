#!/usr/bin/python
# coding=utf-8
import argparse
import re
import sys
import codecs
import random
import time
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

bad = [
    u'сладенький ответ',
    u'знаешь чей ответ',
    u'сам знаешь кого ответ',
    u'сама знаешь кого ответ',
    u'гомофоба ответ',
    u'голубя ответ',
    u'алкаша ответ',
    u'ты знаешь чей это ответ',
    u'ты знаешь чей ответ',
    u'дурочки ответ',
    u'прокурора ответ',
    u'еврейский ответ',
    u'еврея ответ',
    u'кассира макдональдса ответ',
    u'ну ты знаешь чей ответ',
    u'знаешь кого ответ',
    u'гуманитария ответ',
    u'дурака ответ',
    u'либерала ответ',
    u'кого ответ',
    u'чей ответ',
    u'жадины ответ',
    u'медика ответ',
    u'школьника ответ',
    u'знаешь кого ответ',
    u'рэпера ответ',
    u'репера ответ',
    u'историка ответ',
    u'женщины ответ',
    u'шоколадки ответ',
    u'юли ответ',
    u'кати ответ',
    u'светы ответ',
    u'дат',
    u'скотины ответ',
    u'ответ от бога',
    u'эрика ответ'
]
#bad = set(bad)
bad = [' ' + s.strip() + ' ' for s in bad]

replace = [
    u'красноречивый ответ',
    u'содержательный ответ',
    u'лаконичный ответ',
    u'очень развернутый ответ',
    u'какой содержательный ответ',
    u'я предсказала ваш ответ',
    u'котика ответ',
    u'информативный ответ',
    u'отрицательный ответ',
    u'это ваш окончательный ответ ?',
    u'исчерпывающий ответ',
    u'не могу придумать смешной ответ',
    u'на нет и суда нет',
    u'аргументированный ответ',
    u'сухой ответ'
]

def get_random(list_):
    return list_[random.randint(0, len(list_) - 1)]

def substitute_reply(bad, replace, reply):
    original_reply = reply
    reply = ' '.join(re.sub(ur'[^а-я\s]', ' ', reply.lower().strip()).split(' ')).strip()
    reply = ' ' + reply + ' '
    ok = True
    for b in bad:
        if b in reply:
            return get_random(replace)
    return original_reply

class SubsMapper(object):
    def start(self):
        random.seed(int(time.time() * 1e6))

    def __call__(self, row):
        key = 'rewritten_reply' if 'rewritten_reply' in row else 'reply'
        reply = unicode(row[key], 'utf-8')
        row['rewritten_reply'] = substitute_reply(bad, replace, reply)
        if row['rewritten_reply']:
            yield row

def yt_main(args):
    assert args.src and args.dst
    row_count = yt.get(args.src + '/@row_count')
    rows_per_job = 10000
    job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
    yt.run_map(SubsMapper(), args.src, args.dst, spec={'job_count': job_count})

def local_main(args):
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    r = 0
    for line in sys.stdin:
        reply = ' '.join(re.sub(ur'[^а-я\s]', ' ', line.strip()).split(' ')).strip()
        reply = ' ' + reply + ' '
        ok = True
        for b in bad:
            if b in reply:
                ok = False
                break
        if not ok:
            print >> sys.stderr, 'replacing "' + line.strip() + '" with "' + replace[r] + '"'
            print replace[r]
            r += 1
            r %= len(replace)
        else:
            print line.strip()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='')
    parser.add_argument('--dst', default='')
    parser.add_argument('--local', action='store_true')
    args = parser.parse_args()

    if args.local:
        local_main(args)
    else:
        yt_main(args)

