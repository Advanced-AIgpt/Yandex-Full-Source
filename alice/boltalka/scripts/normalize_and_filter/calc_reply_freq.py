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

def yield_replies(row):
    for reply in row['value'].split('\t'):
        user_id, text = reply.split(' ', 1)
        text = get_join_key(text)
        user_id = int(user_id)
        if not text:
            continue
        yield {
            'reply': text,
            'user_id': user_id,
        }

def calc_usage(key, rows):
    users = set()
    for row in rows:
        users.add(row['user_id'])
    if len(users) == 1:
        return
    res = dict(key)
    res.update({
        'user_cnt': len(users),
        'inv_user_cnt': 2**64 - len(users),
    })
    yield res

def main(args):
    yt.run_map_reduce(yield_replies, calc_usage, args.src, args.dst, reduce_by=['reply'], spec={'data_size_per_map_job': 20 * 2**20})
    yt.run_sort(args.dst, args.dst, sort_by=['inv_user_cnt', 'user_cnt', 'reply'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    main(args)
