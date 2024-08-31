#!/usr/bin/python
import sys
import argparse
import yt.wrapper as yt
import simplejson as json
import regex as re
from copy import deepcopy

yt.config.set_proxy("hahn.yt.yandex.net")

def filter_len2(row):
    if not row['value'].count('\t'):
        return
    prev = None
    for reply in row['value'].split('\t'):
        if prev and prev == reply:
            return
        prev = reply
    yield row

def main(args):
    yt.run_map(filter_len2, args.src, args.dst)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    main(args)
