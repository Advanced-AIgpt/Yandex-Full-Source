#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import codecs
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

from remove_emoticons import EmoticonsRemover

_NON_ALPH_RE = re.compile(ur'[^а-яa-z\s]')
_emo_remover = EmoticonsRemover(strip_nonstandard_chars=True)

def get_join_key(s):
    s = unicode(s, 'utf-8')
    s = _emo_remover(s, iterative=True)
    s = s.lower()
    return ' '.join(_NON_ALPH_RE.sub(' ', s).split()).strip()

def yield_whitelist_key(row):
    row['src'] = 0
    row['join'] = get_join_key(row['reply'])
    if row['join']:
        yield row

def yield_src_key(row):
    row['src'] = 1
    row['join'] = get_join_key(row['reply'])
    if row['join']:
        yield row

def join_with_whitelist(key, rows):
    good = False
    for row in rows:
        if row['src'] == 0:
            good = True
        else:
            if not good:
                return
            del row['src']
            del row['join']
            yield row

def main(args):
    yt.run_map(yield_whitelist_key, args.whitelist, args.dst, spec={'data_size_per_job': 20 * 2**20})
    yt.run_map(yield_src_key, args.src, yt.TablePath(args.dst, append=True), spec={'data_size_per_job': 20 * 2**20})
    yt.run_sort(args.dst, args.dst, sort_by=['join', 'src'])
    yt.run_reduce(join_with_whitelist, args.dst, args.dst, reduce_by='join')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--whitelist', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    main(args)
