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
    bad_dict = set()
    with codecs.open(dict_file, 'r', 'utf-8') as inp:
        for line in inp:
            bad_dict.add(line.strip())
    return bad_dict

def main(args):
    bad_dict = set()
    for bad in args.bad_dict:
        dct = load_dict(bad)
        bad_dict = bad_dict | dct

    with codecs.open(args.rus_lister, 'r', 'utf-8') as inp:
        word = []
        for line in inp:
            line = line.strip()
            if line:
                word.append(line)
                continue
            if not word:
                continue
            base = ''
            for s in word:
                if re.match(ur'^[а-я]+$', s):
                    base = s
                    break
            if not base:
                print >> sys.stderr, 'not found:' + '\t' + ' _EOL_ '.join(word)
                word = []
                continue
            if base in bad_dict:
                print >> sys.stderr, 'filtered:' + '\t' + base
                word = []
                continue
            for s in word:
                print s
            print
            word = []

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--bad-dict', type=lambda x:x.split(','), required=True)
    parser.add_argument('--rus-lister', required=True)
    args = parser.parse_args()

    main(args)
