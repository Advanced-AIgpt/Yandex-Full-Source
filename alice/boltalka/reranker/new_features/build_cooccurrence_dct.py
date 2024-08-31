# coding=utf-8
import yt.wrapper as yt
import numpy as np
import regex as re
import codecs, argparse, sys, os, string
from collections import Counter
from preprocess_text import Preprocessor, tokenize_words, tokenize_trigrams

yt.config.set_proxy("hahn.yt.yandex.net")


class Cooc_extractor(object):
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer
        self.prpr = Preprocessor()

    def __call__(self, row):
        cntr = Counter()
        context = self.prpr(row['context_0'])
        reply = self.prpr(row['reply'])
        for context_token in self.tokenizer(context):
            for reply_token in self.tokenizer(reply):
                token = context_token + '\t' + reply_token
                cntr[token] += 1
        for token, freq in cntr.iteritems():
            yield {'token': token, 'freq': freq}


class Reducer(object):
    def __init__(self, num_docs):
        self.num_docs = num_docs

    def __call__(self, key, rows):
        freq = 0
        df = 0
        for row in rows:
            freq += row['freq']
            df += 1
        idf = np.log((self.num_docs + 1) / float(df + 1)) + 1.
        yield {'token': key['token'], 'freq': freq, 'idf': idf, 'inv_freq': -freq}


def main(args):
    num_docs = yt.row_count(args.src)
    if args.trigrams:
        yt.run_map(Cooc_extractor(tokenize_trigrams), args.src, args.dst)
    else:
        yt.run_map(Cooc_extractor(tokenize_words), args.src, args.dst)
    yt.run_sort(args.dst, args.dst, sort_by=['token'])
    yt.run_reduce(Reducer(num_docs), args.dst, args.dst, reduce_by=['token'])
    yt.run_sort(args.dst, args.dst, sort_by=['inv_freq', 'token'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--trigrams', action='store_true')
    args = parser.parse_args()
    main(args)
