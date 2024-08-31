import yt.wrapper as yt
import argparse, os, sys, hashlib
from copy import copy
from collections import defaultdict

yt.config.set_proxy("hahn.yt.yandex.net")


class Generate_reply_negatives(object):
    def __init__(self, num_negatives, negative_type):
        self.num_negatives = num_negatives
        self.negative_type = negative_type

    def __call__(self, key, rows):
        rows = list(rows)
        context2replies_dct = defaultdict(set)
        utterances_ids = set()

        for row in rows:
            if row['reply'] is not None:
                assert row['height'] != -1
                context2replies_dct[row['context_0']].add(row['reply'])
                utterances_ids.add((row['reply'], row['id']))

        for row in rows:
            negative_replies_in_tree = [(u, idx) for (u, idx) in utterances_ids if u not in context2replies_dct[row['context_0']]]
            negative_replies_in_tree = sorted(negative_replies_in_tree, key=lambda x: x[1])
            if self.num_negatives:
                negative_replies_in_tree = negative_replies_in_tree[:self.num_negatives]

            positive_row = copy(row)
            negative_row = copy(row)
            positive_row['label'] = 1
            negative_row['label'] = 0
            positive_row['negative_type'] = self.negative_type
            negative_row['negative_type'] = positive_row['negative_type']
            positive_row['num_negatives'] = len(negative_replies_in_tree)
            negative_row['num_negatives'] = positive_row['num_negatives']

            for negative_reply, negative_id in negative_replies_in_tree:
                negative_row['reply'] = negative_reply
                negative_row['id'] = negative_id
                group_id = hashlib.md5('%s\t%s\t%s' % (self.negative_type, positive_row['id'], negative_row['id'])).hexdigest()
                positive_row['group_id'] = group_id
                negative_row['group_id'] = group_id
                yield positive_row
                yield negative_row


class Generate_context_negatives(object):
    def __init__(self, num_negatives, negative_type):
        self.num_negatives = num_negatives
        self.negative_type = negative_type

    def __call__(self, key, rows):
        rows = list(rows)
        reply2contexts_dct = defaultdict(set)
        utterances_ids = set()

        for row in rows:
            reply2contexts_dct[row['reply']].add(row['context_0'])
            utterances_ids.add((row['reply'], row['id']))

        for row in rows:
            negative_contexts_in_tree = set()
            negative_contexts1_in_tree = [(u, idx) for (u, idx) in utterances_ids if u not in reply2contexts_dct[row['reply']]]
            for context1, negative_id in negative_contexts1_in_tree:
                for context2 in reply2contexts_dct[context1]:
                    negative_contexts_in_tree.add(((context2, context1), negative_id))
            negative_contexts_in_tree = sorted(negative_contexts_in_tree, key=lambda x: x[1])
            if self.num_negatives:
                negative_contexts_in_tree = negative_contexts_in_tree[:self.num_negatives]

            positive_row = copy(row)
            negative_row = copy(row)
            positive_row['label'] = 1
            negative_row['label'] = 0
            positive_row['negative_type'] = self.negative_type
            negative_row['negative_type'] = positive_row['negative_type']
            positive_row['num_negatives'] = len(negative_contexts_in_tree)
            negative_row['num_negatives'] = positive_row['num_negatives']

            for negative_context, negative_id in negative_contexts_in_tree:
                negative_row['context_2'], negative_row['context_1'] = negative_context
                negative_row['id'] = negative_id
                group_id = hashlib.md5('%s\t%s\t%s' % (self.negative_type, positive_row['id'], negative_row['id'])).hexdigest()
                positive_row['group_id'] = group_id
                negative_row['group_id'] = group_id
                yield positive_row
                yield negative_row


def main(args):
    if args.negative_source == 'tree':
        reduce_cols = ['key']
    elif args.negative_source == 'user':
        reduce_cols = ['user']
    elif args.negative_source == 'tree-user':
        reduce_cols = ['key', 'user']
    yt.run_sort(args.src, args.dst, sort_by=reduce_cols+['id'])

    if args.negative_type == 'reply':
        negatives_generator = Generate_reply_negatives(args.num_negatives, args.negative_type)
    elif args.negative_type == 'context21':
        negatives_generator = Generate_context_negatives(args.num_negatives, args.negative_type)

    yt.run_reduce(negatives_generator, args.dst, args.dst, reduce_by=reduce_cols, sort_by=reduce_cols+['id'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--num-negatives', type=int, default=None)
    parser.add_argument('--negative-source', choices=['tree', 'user', 'tree-user'], default='tree')
    parser.add_argument('--negative-type', choices=['reply', 'context21'], default='reply')
    args = parser.parse_args()
    main(args)
