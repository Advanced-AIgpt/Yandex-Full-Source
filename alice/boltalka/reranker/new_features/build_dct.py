# coding=utf-8
import yt.wrapper as yt
import numpy as np
import regex as re
import codecs, argparse, sys, os, string
from collections import Counter
from preprocess_text import Preprocessor, tokenize_words, tokenize_trigrams, tokenize_lemmas

yt.config.set_proxy("hahn.yt.yandex.net")


class Docs_extractor(object):
    def __init__(self, from_pool=False):
        self.from_pool = from_pool

    def __call__(self, row):
        if not (self.from_pool and (len(row['context_1']) != 0 or len(row['context_2']) != 0)):
            yield {'doc': row['context_0']}
        yield {'doc': row['reply']}


class Tokens_extractor(object):
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer
        self.prpr = Preprocessor()

    def __call__(self, row):
        doc = self.prpr(row['doc'])
        cntr = Counter(self.tokenizer(doc))
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


def append_unk(num_docs):
    token = '_UNK_'
    freq = 0
    df = 0
    idf = np.log((num_docs + 1) / float(df + 1)) + 1.
    yield {'token': '_UNK_', 'freq': freq, 'idf': idf, 'inv_freq': -freq}


def main(args):
    yt.run_map(Docs_extractor(args.from_pool), args.src, args.dst)
    num_docs = yt.row_count(args.dst)
    if args.trigram:
        yt.run_map(Tokens_extractor(tokenize_trigrams), args.dst, args.dst)
    elif args.lemma:
        yt.run_map(Tokens_extractor(tokenize_lemmas), args.dst, args.dst)
    else:
        yt.run_map(Tokens_extractor(tokenize_words), args.dst, args.dst)
    yt.run_sort(args.dst, args.dst, sort_by=['token'])
    yt.run_reduce(Reducer(num_docs), args.dst, args.dst, reduce_by=['token'])
    yt.write_table('<append=%true>'+args.dst, append_unk(num_docs))
    yt.run_sort(args.dst, args.dst, sort_by=['inv_freq', 'token'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst')
    parser.add_argument('--from-pool', action='store_true')
    parser.add_argument('--trigram', action='store_true')
    parser.add_argument('--lemma', action='store_true')
    args = parser.parse_args()
    assert args.trigram + args.lemma < 2
    main(args)
