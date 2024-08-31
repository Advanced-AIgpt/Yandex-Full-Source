#!/usr/bin/env python2

from __future__ import print_function

import argparse
import numpy as np
import pandas as pd
import subprocess


DEFAULT_QUERIES='queries.csv'
DEFAULT_DEMO='./demo/demo'
DEFAULT_REGIONS='regions.bin'


def run_demo(path, regions, queries):
    """Runs demo binary, feeds list of queries to it and returns a list of result ids"""

    args = [path, '--only-top-id', 'yes', '--position', 'yandex']
    process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    result = process.communicate(input='\n'.join(queries))[0]
    return list(map(int, result.splitlines()))


def go(args):
    data = pd.read_csv(args.queries, na_values=['MUL', 'NAN']).dropna()

    queries = data['Query'].tolist()

    expected_ids = np.array(map(int, data['GeoId'].tolist()))
    actual_ids = np.array(run_demo(args.demo, args.regions, queries))
    assert len(expected_ids) == len(actual_ids)

    if args.show:
        nogeo = np.array(queries)[expected_ids != actual_ids]
        print('\n'.join(nogeo))

    print('Total number of samples: ', len(expected_ids))
    print('Accuracy: {:3f}'.format(float(np.sum(expected_ids == actual_ids)) / len(expected_ids)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--queries', type=str, default=DEFAULT_QUERIES, help='path to csv with queries')
    parser.add_argument('--demo', type=str, default=DEFAULT_DEMO, help='path to a binary')
    parser.add_argument('--regions', type=str, default=DEFAULT_REGIONS, help='path to a regions binary')
    parser.add_argument('--show', action='store_true', default=False, help='show non-found queries')
    args = parser.parse_args()
    go(args)
