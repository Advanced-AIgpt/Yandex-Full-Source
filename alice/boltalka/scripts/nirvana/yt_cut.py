#!/usr/bin/python
import os
import sys
import math
import re
import time
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

class Cut(object):
    def __init__(self, columns, start_expr, preprocess_expr):
        self.columns = columns
        self.start_expr = compile(start_expr, '<string>', 'exec')
        self.preprocess_expr = compile(preprocess_expr, '<string>', 'exec')

    def start(self):
        exec(self.start_expr)

    def __call__(self, row):
        exec self.preprocess_expr in globals(), locals()
        yield { k: row[k] for k in self.columns }

def main(args):
    mapper = Cut(args.columns, args.start_expr, args.preprocess_expr)
    yt.run_map(mapper, args.src, args.dst)#, spec={'data_size_per_job': 20 * 2**20})

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--columns', type=lambda x: x.split(','), required=True)
    parser.add_argument('--start-expr', default='')
    parser.add_argument('--preprocess-expr', default='')
    args = parser.parse_args()
    main(args)
