#!/usr/bin/python
import sys
import argparse
import yt.wrapper as yt
from copy import deepcopy

def emit_edges(row):
    res = deepcopy(row)
    if not 'root_key' in res:
        res['root_key'] = '' if row['subkey'] else row['key']

    res['join'] = row['key']
    yield res

    if row['subkey']:
        res['join'] = row['subkey']
        yield res

def join_edges(key, rows):
    rows = list(rows)
    root = None
    for r in rows:
        if r['key'] == r['join']:
            assert root is None
            root = r
            if r['root_key']:
                res = deepcopy(r)
                del res['join']
                yield res

    if root is None:
        return

    for r in rows:
        if r['key'] == r['join']:
            continue
        res = {}
        # key is from deepest node in sequence so that other sequences could join it
        res['key'] = r['key']
        # subkey is from root to join this sequence to other sequences
        res['subkey'] = root['subkey']
        res['value'] = root['value'] + '\t' + r['value']
        res['root_key'] = root['root_key']
        yield res

def unique_reduce(key, rows):
    for r in rows:
        yield r
        break

def filter_roots(row):
    if row['root_key']:
        yield row

def filter_longest_chains(key, rows):
    key = key['root_key']
    prev = None
    for r in rows:
        if prev:
            assert prev['value'] <= r['value']
            if not r['value'].startswith(prev['value']):
                yield {'key': key, 'subkey': '', 'value': prev['value']}
        prev = deepcopy(r)
    if prev:
        yield {'key': key, 'subkey': '', 'value': prev['value']}

def find_leaves_and_roots(key, rows):
    rows = list(rows)
    root = None
    for r in rows:
        if r['key'] == r['join']:
            assert root is None
            root = r

    if root:
        yield {
            'key': root['key'],
            'subkey': root['subkey'],
            'value': root['value'],
            'is_leaf': int(len(rows) == 1),
            'is_root': int(not root['subkey'])
        }
    else:
        for r in rows:
            yield {
                'key': r['key'],
                'subkey': r['subkey'],
                'value': r['value'],
                'is_leaf': 0,
                'is_root': 1
            }

def process_leaves_and_roots(key, rows, no_len_one, more_roots):
    rows = list(rows)
    assert len(rows) == 1 or len(rows) == 2

    is_leaf = 0
    is_root = 0
    for r in rows:
        is_leaf |= r['is_leaf']
        is_root |= r['is_root']
        assert r['subkey'] == rows[0]['subkey']
        assert r['value'] == rows[0]['value']

    if no_len_one and is_leaf and is_root:
        return

    res = rows[0]
    del res['is_leaf']
    del res['is_root']

    if more_roots and is_root:
        res['subkey'] = ''

    yield res

def main(args):
    chains_dst = args.dst + '_chains'
    src = args.src

    if args.unique:
        print 'Uniqueing'
        yt.run_sort(src, sort_by=['key', 'subkey'])
        yt.run_reduce(unique_reduce, src, chains_dst, reduce_by='key')
        src = chains_dst

    if args.no_len_one or args.more_roots:
        print 'Preprocessing'
        yt.run_map_reduce(emit_edges, find_leaves_and_roots, src, chains_dst, reduce_by='join')
        yt.run_sort(chains_dst, sort_by='key')
        preprocess = lambda key, rows: process_leaves_and_roots(key, rows, args.no_len_one, args.more_roots)
        yt.run_reduce(preprocess, chains_dst, chains_dst, reduce_by='key')
        src = chains_dst

    for it in xrange(args.num_iters):
        print 'Iteration #%d/%d' % (it + 1, args.num_iters)
        yt.run_map_reduce(emit_edges, join_edges, src, chains_dst, reduce_by='join')
        src = chains_dst

    print 'Finalizing'
    yt.run_map_reduce(filter_roots, filter_longest_chains, chains_dst, args.dst, sort_by=['root_key', 'value'], reduce_by='root_key')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--num-iters', type=int, default=6)
    parser.add_argument('--unique', action='store_true')
    parser.add_argument('--no-len-one', action='store_true')
    parser.add_argument('--more-roots', action='store_true')
    args = parser.parse_args()

    main(args)
