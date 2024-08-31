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
    dct = set()
    if not os.path.exists(dict_file):
        return dct
    with codecs.open(dict_file, 'r', 'utf-8') as inp:
        for line in inp:
            dct.add(line.strip())
    return dct

_BAD_RE = re.compile(ur'қ|ң|ү|ө|ѝ|і|ї|ў|ў|[а-я]i|i[а-я]| i ')

def is_good(raw_line, rus_dict, bad_eng_dict, dict_ratio, keep_empty_tokens, inverse):
    line = re.sub(ur'[ёë]', u'е', raw_line.rstrip().lower())

    if _BAD_RE.search(line):
        return False

    eng = re.sub(ur'[^a-z]', ' ', line).split()
    for tok in eng:
        if tok in bad_eng_dict:
            return False

    s = re.sub(ur'[^а-я-]', ' ', line)
    if s.count(' ') == len(s):
        return False

    if keep_empty_tokens:
        s = s.split(' ')
    else:
        s = s.split()
    rus_cnt = 0
    for tok in s:
        if tok in rus_dict:
            rus_cnt += 1

    ratio = rus_cnt / float(len(s))
    #print ratio, raw_line.rstrip()
    return inverse ^ (ratio >= dict_ratio)

def local_main(args):
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    print >> sys.stderr, "Loading dict..."
    rus_dict = load_dict(args.dict_file)
    bad_eng_dict = load_dict(args.bad_eng_dict_file)

    print >> sys.stderr, "Filtering..."
    for it, raw_line in enumerate(sys.stdin):
        it += 1
        if it % 100000 == 0:
            print >> sys.stderr, it, "lines processed\r",

        if is_good(raw_line, rus_dict, bad_eng_dict, args.dict_ratio, args.keep_empty_tokens, args.inverse):
            print raw_line.rstrip()

    print >> sys.stderr

class IsGoodMapper(object):
    def __init__(self, args):
        self.dict_file = os.path.basename(args.dict_file)
        self.bad_eng_dict_file = os.path.basename(args.bad_eng_dict_file)
        self.dict_ratio = args.dict_ratio
        self.keep_empty_tokens = args.keep_empty_tokens
        self.inverse = args.inverse

    def start(self):
        self.rus_dict = load_dict(self.dict_file)
        self.bad_eng_dict = load_dict(self.bad_eng_dict_file)

    def __call__(self, row):
        line = unicode(row['reply'], 'utf-8')
        if is_good(line, self.rus_dict, self.bad_eng_dict, self.dict_ratio, self.keep_empty_tokens, self.inverse):
            yield row

def yt_main(args):
    assert args.src and args.dst
    yt.run_map(IsGoodMapper(args), args.src, args.dst, local_files=[args.dict_file, args.bad_eng_dict_file])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--dict-file', required=True)
    parser.add_argument('--bad-eng-dict-file', required=True)
    parser.add_argument('--dict-ratio', type=float, default=1.0)
    parser.add_argument('--keep-empty-tokens', action='store_true')
    parser.add_argument('--inverse', action='store_true')
    parser.add_argument('--local', action='store_true')
    parser.add_argument('--src', default='')
    parser.add_argument('--dst', default='')
    args = parser.parse_args()

    if args.local:
        local_main(args)
    else:
        yt_main(args)
