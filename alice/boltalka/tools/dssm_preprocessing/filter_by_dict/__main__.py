#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import codecs
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

def load_dict(dict_file):
    if dict_file is None:
        return None

    dct = set()
    with codecs.open(dict_file, 'r', 'utf-8') as inp:
        for line in inp:
            line = line.strip().lower()
            word = line
            sp = line.split('\t')
            if len(sp) == 2 and sp[1] in ['starts', 'ends', 'inside', 'exact']:
                word = sp[0]
                if sp[1] == 'starts':
                    word = ' ' + word
                elif sp[1] == 'ends':
                    word = word + ' '
                elif sp[1] == 'inside':
                    pass
                elif sp[1] == 'exact':
                    word = ' ' + word + ' '
            else:
                word = ' ' + word + ' '
            dct.add(word)
    return dct

def is_good(raw_line, good_dict, bad_dict, dict_ratio, inverse):
    line = raw_line.rstrip().lower()

    words = line.split()
    joined_words = ' ' + ' '.join(words) + ' '
    if bad_dict is not None:
        for bad_word in bad_dict:
            if bad_word in joined_words:
                return (False, bad_word)

    if good_dict is None:
        return (True, None)

    good_cnt = 0
    for tok in words:
        if tok in good_dict:
            good_cnt += 1

    ratio = good_cnt / float(len(words))
    #print ratio, raw_line.rstrip()
    return (inverse ^ (ratio >= dict_ratio), '__DICT_RATIO__')

def local_main(args):
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    print >> sys.stderr, "Loading dict..."
    good_dict = load_dict(args.good_dict_file)
    bad_dict = load_dict(args.bad_dict_file)

    print >> sys.stderr, "Filtering..."
    for it, raw_line in enumerate(sys.stdin):
        it += 1
        if it % 100000 == 0:
            print >> sys.stderr, it, "lines processed\r",

        success, reason = is_good(raw_line, good_dict, bad_dict, args.dict_ratio, args.inverse)
        if success:
            print raw_line.rstrip()

    print >> sys.stderr

class IsGoodMapper(object):
    def __init__(self, args):
        self.good_dict_file = os.path.basename(args.good_dict_file) if args.good_dict_file else None
        self.bad_dict_file = os.path.basename(args.bad_dict_file) if args.bad_dict_file else None
        self.dict_ratio = args.dict_ratio
        self.inverse = args.inverse

    def start(self):
        self.good_dict = load_dict(self.good_dict_file)
        self.bad_dict = load_dict(self.bad_dict_file)

    def __call__(self, row):
        line = unicode(row['reply'], 'utf-8')
        success, reason = is_good(line, self.good_dict, self.bad_dict, self.dict_ratio, self.inverse)
        if success:
            yield yt.create_table_switch(0)
        else:
            yield yt.create_table_switch(1)
            row['reason'] = reason
        yield row

def yt_main(args):
    assert args.src and args.dst
    local_files = []
    if args.good_dict_file:
        local_files.append(args.good_dict_file)
    if args.bad_dict_file:
        local_files.append(args.bad_dict_file)
    yt.run_map(IsGoodMapper(args), args.src, [args.dst, args.err], local_files=local_files)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--good-dict-file')
    parser.add_argument('--bad-dict-file')
    parser.add_argument('--dict-ratio', type=float, default=1.0)
    parser.add_argument('--inverse', action='store_true')
    parser.add_argument('--local', action='store_true')
    parser.add_argument('--src', default='')
    parser.add_argument('--dst', default='')
    parser.add_argument('--err', default='')
    args = parser.parse_args()

    if args.local:
        local_main(args)
    else:
        yt_main(args)

