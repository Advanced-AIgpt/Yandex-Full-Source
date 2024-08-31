#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

_NON_ALPH_RE = re.compile(ur'[^а-яa-z\s]')

def alpha(s):
    s = unicode(s, 'utf-8')
    return ' '.join(_NON_ALPH_RE.sub(' ', s).split()).strip()

def identity(s):
    return unicode(s, 'utf-8')

def yield_key(join_key, join_key_normalizer, row):
    row['join'] = join_key_normalizer(row[join_key])
    row['join_freq'] = 1
    yield row

def calc_frequencies(output_column, key, rows):
    rows = list(rows)
    freq = 0
    for row in rows:
        freq += row['join_freq']
    for row in rows:
        row[output_column] = freq
        del row['join']
        del row['join_freq']
        yield row

def get_output_column(args):
    return args.key + ('' if args.normalization == 'identity' else '_' + args.normalization) + '_freq'

def main(args):
    mapper = lambda row: yield_key(args.key, globals()[args.normalization], row)
    reducer = lambda key, rows: calc_frequencies(get_output_column(args), key, rows)
    yt.run_map_reduce(mapper, reducer, args.src, args.dst, reduce_by=['join'], spec={'data_size_per_map_job': 20 * 2**20})

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--key', required=True)
    parser.add_argument('--normalization', default='identity')
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    main(args)
