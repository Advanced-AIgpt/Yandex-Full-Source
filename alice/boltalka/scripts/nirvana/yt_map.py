#!/usr/bin/python
import os
import sys
import math
import re
import time
import hashlib
import string
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

class Map(object):
    def __init__(self, map_expr, start_expr):
        self.map_expr = compile(map_expr, '<string>', 'exec')
        self.start_expr = compile(start_expr, '<string>', 'exec')

    def start(self):
        exec(self.start_expr)

    def __call__(self, row):
        exec self.map_expr in globals(), locals()
        yield row

def main(args):
    mapper = Map(args.map_expr, args.start_expr)
    yt.run_map(mapper, args.src, args.dst)#, spec={'data_size_per_job': 20 * 2**20})

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--map-expr', required=True)
    parser.add_argument('--start-expr', default='')
    args = parser.parse_args()
    main(args)
