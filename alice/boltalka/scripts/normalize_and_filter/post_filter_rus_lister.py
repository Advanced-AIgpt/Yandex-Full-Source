#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import codecs
import argparse
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

def load_dict(dict_file):
    dct = set()
    with codecs.open(dict_file, 'r', 'utf-8') as inp:
        for line in inp:
            dct.add(line.strip())
    return dct

def main(args):
    bad_dict = load_dict(args.bad_dict)
    rus_lister = load_dict(args.rus_lister)
    total = len(rus_lister)
    res = rus_lister - bad_dict
    bad = rus_lister - res
    res = sorted(res)
    print >> sys.stderr, 'total:', total
    print >> sys.stderr, 'filtered:', total - len(res), 'words'
    for s in bad:
        print >> sys.stderr, s
    for s in res:
        print s

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--bad-dict', required=True)
    parser.add_argument('--rus-lister', required=True)
    args = parser.parse_args()

    main(args)
