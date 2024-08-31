#!/usr/bin/python
# coding=utf-8
import os
import argparse
import re
import sys
import codecs
import yt.wrapper as yt
from collections import OrderedDict
yt.config.set_proxy("hahn.yt.yandex.net")

def normalize(s):
    return ' '.join(re.sub(ur'[^а-я\s0-9?!]', ' ', s.strip()).split()).strip()

def load_replace(filename):
    dct = OrderedDict()
    with codecs.open(filename, 'r', 'utf-8') as inp:
        for line in inp:
            if not line.strip() or line.startswith('#'):
                continue
            reply, replace = line.rstrip('\r\n').split('\t')
            reply = normalize(reply)
            dct[reply] = replace
    return dct

def substitute_reply(replace, reply):
    key = normalize(reply)
    return replace[key] if key in replace else reply

class SubsMapper(object):
    def __init__(self, args):
        self.replace_file = os.path.basename(args.replace_file)

    def start(self):
        self.replace = load_replace(self.replace_file)

    def __call__(self, row):
        key = 'rewritten_reply' if 'rewritten_reply' in row else 'reply'
        reply = unicode(row[key], 'utf-8')
        row['rewritten_reply'] = substitute_reply(self.replace, reply)
        if row['rewritten_reply']:
            yield row

def yt_main(args):
    assert args.src and args.dst
    row_count = yt.get(args.src + '/@row_count')
    rows_per_job = 10000
    job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
    yt.run_map(SubsMapper(args), args.src, args.dst, local_files=[args.replace_file], spec={'job_count': job_count})

def local_main(args):
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    replace = load_replace(args.replace_file)

    for line in sys.stdin:
        reply = substitute_reply(replace, line.strip())
        if line.strip() != reply:
            #print >> sys.stderr, 'replacing "' + line.strip() + '" with "' + reply + '"'
            pass
        print reply

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='')
    parser.add_argument('--dst', default='')
    parser.add_argument('--replace-file', required=True)
    parser.add_argument('--local', action='store_true')
    args = parser.parse_args()

    if args.local:
        local_main(args)
    else:
        yt_main(args)

