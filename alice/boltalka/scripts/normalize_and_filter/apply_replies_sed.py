#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import codecs
import argparse
import random
from collections import OrderedDict

import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

random.seed(123)

def load_regexes(filename):
    dct = OrderedDict()
    with codecs.open(filename, 'r', 'utf-8') as inp:
        for line in inp:
            if not line.strip() or line.startswith('#'):
                continue
            type_, regex, sub = line.rstrip('\r\n').split('\t')
            if type_ == 'exact':
                regex = '^' + regex + '$'
            elif type_ == 'substring':
                regex = '^.*' + regex + '.*$'
            elif type_ == 'phrase':
                regex = '(^| )' + regex + '($| )'
                sub = r'\1' + sub + r'\2'
            else:
                print >> sys.stderr, 'unsupported match type: ' + type_
                sys.exit(1)
            dct[regex] = dct.get(regex, []) + [sub]
    return [(re.compile(regex), sub) for regex, sub in dct.iteritems()]

def get_random(list_):
    return list_[random.randint(0, len(list_) - 1)]

def substitute_reply(subs, reply):
    for regex, sub in subs:
        reply = regex.sub(get_random(sub), reply).strip()
        if not reply:
            break
    return reply

class SubsMapper(object):
    def __init__(self, args):
        self.regexes_file = os.path.basename(args.regexes_file)

    def start(self):
        self.subs = load_regexes(self.regexes_file)

    def __call__(self, row):
        key = 'rewritten_reply' if 'rewritten_reply' in row else 'reply'
        reply = unicode(row[key], 'utf-8')
        row['rewritten_reply'] = substitute_reply(self.subs, reply)
        if row['rewritten_reply']:
            yield row

def yt_main(args):
    assert args.src and args.dst
    row_count = yt.get(args.src + '/@row_count')
    rows_per_job = 10000
    job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
    yt.run_map(SubsMapper(args), args.src, args.dst, local_files=[args.regexes_file], spec={'job_count': job_count})

def local_main(args):
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    subs = load_regexes(args.regexes_file)
    for line in sys.stdin:
        reply = line.strip()
        reply = substitute_reply(subs, reply)
        if not reply:
            print 'DELETED'
            continue
        print reply

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='')
    parser.add_argument('--dst', default='')
    parser.add_argument('--regexes-file', required=True)
    parser.add_argument('--local', action='store_true')
    args = parser.parse_args()

    if args.local:
        local_main(args)
    else:
        yt_main(args)

