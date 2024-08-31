import yt.wrapper as yt
import argparse, hashlib, os
import copy
from collections import defaultdict
import numpy as np

np.random.seed(1)

yt.config.set_proxy("hahn.yt.yandex.net")


class Reducer(object):
    def __init__(self, negative_column, context_length, num_negatives, repeat_negatives, root_as_negative):
        self.negative_column = negative_column
        self.num_negatives = num_negatives
        self.repeat_negatives = repeat_negatives
        self.root_as_negative = root_as_negative
        self.text_columns = ['context_' + str(context_length - 1 - i) for i in range(context_length)] + ['reply']
        self.user_columns = ['user_' + str(context_length - 1 - i) for i in range(context_length)] + ['user']
        assert self.negative_column in self.text_columns
        self.negative_column_idx = self.text_columns.index(self.negative_column)
        self.negative_parent_column = self.text_columns[self.negative_column_idx - 1] if self.negative_column_idx >= 1 else None
        self.transition_dct = defaultdict(set)

    def __call__(self, key, rows):
        rows = list(rows)
        utterances = []
        root = None
        for row in rows:
            self.transition_dct[row['context_0']].add(row['reply'])
            if row['reply']:
                reply = {'key': row['key'], 'user': row['user'], 'text': row['reply']}
                utterances.append(reply)
            if all(row[k] is None for k in self.text_columns[:-2]):
                root = {'key': row['key'], 'user': row['user_0'], 'text': row['context_0']}
                utterances.append(root)

        for row in rows:
            bad_negatives = set(row[k] for k in self.text_columns)
            if self.negative_parent_column is not None:
                bad_negatives |= self.transition_dct[row[self.negative_parent_column]]
            if not self.root_as_negative and root is not None:
                bad_negatives.add(root['text'])

            negatives = [u for u in utterances if u['text'] not in bad_negatives]
            if len(negatives) == 0:
                continue
            num_negatives = self.num_negatives if self.repeat_negatives else min(self.num_negatives, len(negatives))
            np.random.shuffle(negatives)

            for negative_idx in xrange(num_negatives):
                pos = negative_idx % len(negatives)
                negative_row = copy.deepcopy(row)
                negative_row[self.negative_column] = negatives[pos]['text']
                negative_row[self.user_columns[self.negative_column_idx]] = negatives[pos]['user']
                negative_row['key'] = negatives[pos]['key']
                negative_row['label'] = 0
                negative_row['negative_column'] = self.negative_column
                negative_row['negative_idx'] = negative_idx
                row['label'] = 1
                row['negative_idx'] = negative_idx
                yield row
                yield negative_row


def main(args):
    reduce_by = args.reduce_by.split(',')
    yt.run_sort(args.src, args.dst, sort_by=reduce_by+['id'])
    yt.run_reduce(Reducer(args.negative_column, args.context_length, args.num_negatives, args.repeat_negatives, args.root_as_negative),
                  args.dst, args.dst, reduce_by=reduce_by, sort_by=reduce_by+['id'], memory_limit=2**31)
    yt.run_sort(args.dst, sort_by=['negative_idx', 'id', 'label'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    parser.add_argument('--reduce-by', default='key')
    parser.add_argument('--negative-column', default='reply')
    parser.add_argument('--num-negatives', type=int, required=True)
    parser.add_argument('--repeat-negatives', action='store_true')
    parser.add_argument('--root-as-negative', action='store_true')
    args = parser.parse_args()
    main(args)
