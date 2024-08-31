#!/usr/bin/python
# coding=utf-8
import os
import sys
import codecs
import argparse
import yt.wrapper as yt
from copy import copy
import hashlib
yt.config.set_proxy("hahn.yt.yandex.net")


class Dialogue_builder(object):
    def __init__(self, context_length):
        self.context_length = context_length
        self.columns = ['context_' + str(context_length - i - 1) for i in xrange(context_length)] + ['reply']

    def __call__(self, row):
        res = {}
        res['session_id'] = row['session_id']

        queue = [None] * (self.context_length + 2)

        for turn_idx, turn in enumerate(row['session']):
            queue = queue[2:] + [turn['_query'], turn['_reply']]
            if turn['callback'].get('suggest_type') == 'gc_suggest':
                res['suggests'] = row['session'][turn_idx - 1]['suggests']
                res['req_id'] = turn['req_id']
                for column, utterance in zip(self.columns, queue[:-1]):
                    res[column] = utterance
                yield res


def make_pairs(row):
    rank = 0
    suggests_above = []
    positive_row = copy(row)
    del positive_row['suggests']
    negative_row = copy(positive_row)
    positive_row['label'] = 1
    negative_row['label'] = 0

    for utterance in row['suggests']:
        if utterance.startswith('🔍'):
            continue
        if utterance == row['reply']:
            positive_row['rank'] = rank
            for negative_rank, negative_reply in enumerate(suggests_above):
                negative_row['reply'] = negative_reply
                negative_row['rank'] = negative_rank
                key = positive_row['req_id'] + '\t' + str(negative_rank)
                group_id = hashlib.md5(key).hexdigest()
                positive_row['group_id'] = group_id
                negative_row['group_id'] = group_id
                yield negative_row
                yield positive_row
            return
        suggests_above.append(utterance)
        rank += 1


def main(args):
    srcs = []
    lower_bound = os.path.join(args.src, args.from_date)
    upper_bound = os.path.join(args.src, args.to_date)
    for table in yt.list(args.src, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)
    yt.run_map(Dialogue_builder(args.context_length), srcs, args.dst)
    yt.run_map(make_pairs, args.dst, args.dst)
    yt.run_sort(args.dst, sort_by=['group_id'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', default='//home/voice/dialog/sessions')
    parser.add_argument('--from-date', required=True)
    parser.add_argument('--to-date', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    args = parser.parse_args()
    main(args)
