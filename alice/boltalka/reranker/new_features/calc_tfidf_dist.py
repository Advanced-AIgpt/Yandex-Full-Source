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
        self.cols = ['query_2', 'query_1', 'query_0', 'reply', 'context_2', 'context_1', 'context_0']
        self.groups = [[0,1,2], [2], [1], [0], [3], [4,5,6], [6], [5], [4]]
        self.first_factor_idx = first_factor_idx
        self.prpr = Preprocessor()

    def __call__(self, row):
        turns = [self.prpr(row[col]) for col in self.cols]
        factor_idx = self.first_factor_idx

        for (idf_dct, tokenize) in zip([self.word_idf_dct, self.trigram_idf_dct],
                                        [tokenize_words, tokenize_trigrams]):
            tf = []
            for group in self.groups:
                text = ' '.join([turns[col] for col in group])
                cntr = Counter(tokenize(text))
                num_tokens = float(sum(cntr.values()))
                tf.append({k : (v / num_tokens) for k, v in cntr.iteritems()})

            for i in range(len(self.groups)):
                for j in range(i + 1, len(self.groups)):
                    dot_prod = 0.
                    for token in set(tf[i]) & set(tf[j]):
                        idf = idf_dct.get(token, idf_dct['_UNK_'])
                        dot_prod += tf[i][token] * tf[j][token] * (idf ** 2)
                    row['factor_%d' % factor_idx] = dot_prod
                    factor_idx += 1
        yield row



def main(args):
    word_idf_dct = read_idf_dct(args.word_dct)
    trigram_idf_dct = read_idf_dct(args.trigram_dct)
    yt.run_map(Mapper(word_idf_dct, trigram_idf_dct, args.first_factor_idx), args.src, args.dst,
               memory_limit=10000000000)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--word-dct', required=True)
    parser.add_argument('--trigram-dct', required=True)
    parser.add_argument('--first-factor-idx', type=int, default=537)
    args = parser.parse_args()
    main(args)

