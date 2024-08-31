# coding=utf-8
import yt.wrapper as yt
import re
import codecs, argparse, sys, os, string
from collections import Counter
from preprocess_text import Preprocessor, tokenize_words, tokenize_trigrams

yt.config.set_proxy("hahn.yt.yandex.net")


def read_idf_dct(table, size=None):
    idf_dct = {}
    for row_idx, row in enumerate(yt.read_table(table)):
        if size is not None and row_idx == size:
            break
        idf_dct[row['token']] = row['idf']
    return idf_dct


class Mapper(object):
    def __init__(self, word_idf_dct, trigram_idf_dct, first_factor_idx):
        self.word_idf_dct = word_idf_dct
        self.trigram_idf_dct = trigram_idf_dct
        self.first_factor_idx = first_factor_idx
        self.prpr = Preprocessor()

    def __call__(self, row):
        factor_idx = self.first_factor_idx
        reply = self.prpr(row['reply'])

        for context_col in ['query_0', 'context_0']:
            context = self.prpr(row[context_col])
            for (idf_dct, tokenize) in zip([self.word_idf_dct, self.trigram_idf_dct],
                                           [tokenize_words, tokenize_trigrams]):
                cntr = 0
                for context_token in tokenize(context):
                    for reply_token in tokenize(reply):
                        token = context_token + '\t' + reply_token
                        if token in idf_dct:
                            cntr += 1
                row['factor_%d' % factor_idx] = cntr
                factor_idx += 1
        yield row


def main(args):
    word_idf_dct = read_idf_dct(args.word_dct, args.word_dct_size)
    trigram_idf_dct = read_idf_dct(args.trigram_dct, args.trigram_dct_size)
    yt.run_map(Mapper(word_idf_dct, trigram_idf_dct, args.first_factor_idx), args.src, args.dst,
               memory_limit=10000000000)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--word-dct', required=True)
    parser.add_argument('--trigram-dct', required=True)
    parser.add_argument('--first-factor-idx', type=int, default=537)
    parser.add_argument('--word-dct-size', type=int, default=None)
    parser.add_argument('--trigram-dct-size', type=int, default=None)
    args = parser.parse_args()
    main(args)

