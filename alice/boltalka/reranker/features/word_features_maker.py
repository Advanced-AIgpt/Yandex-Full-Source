# coding=utf-8

import numpy as np
import regex as re
import codecs, argparse, string, scipy, sys
from sklearn.preprocessing import normalize
from collections import Counter
from preprocess_text import Preprocessor


def read_dct(filename, size=None):
    dct = {}
    idfs = []
    with codecs.open(filename, 'r', 'utf-8') as f:
        for line_idx, line in enumerate(f):
            if not size is None and line_idx == size:
                break
            word, idf_str, freq_str = line.split('\t')
            dct[word] = line_idx
            idfs.append(float(idf_str))
    return dct, np.array(idfs)


def build_dct(pool, output_file):
    cntr = Counter()
    preprocessor = Preprocessor()

    with codecs.open(pool, 'r', 'utf-8') as f:
        for line in f:
            line = preprocessor(line)
            for word in preprocessor.tokenize(line):
                cntr[word] += 1
    dct = {}
    for word_idx, (word, freq) in enumerate(cntr.most_common()):
        dct[word] = word_idx

    tfidf = Tfidf(dct)
    idf = tfidf.calc_idf(args.pool)

    with codecs.open(output_file, 'w', 'utf-8') as f:
        for word, pos in sorted(dct.items(), key=lambda x: x[1]):
            f.write('%s\t%.5f\t%d\n' % (word, idf[pos], cntr[word]))

    return dct, idf


def get_base_features(filename, cols=None, delimiter='\t'):
    features = []
    punct = '?!.,()'
    preprocessor = Preprocessor()

    with codecs.open(filename, 'r', 'utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if cols is not None:
                parts = line.split(delimiter)
                line = delimiter.join([parts[col] for col in cols])

            punct_cntr = Counter()
            for x in line:
                if x in punct:
                    punct_cntr[x] += 1
            punct_features = [punct_cntr[x] for x in punct]

            line = preprocessor(line)

            word_lens = [len(word) for word in preprocessor.tokenize(line)]
            if len(word_lens) == 1 and word_lens[0] == 0:
                num_words = 0
            else:
                num_words = len(word_lens)
            sum_lens = sum(word_lens)

            len_stats = [num_words, sum_lens, sum_lens / float(num_words) if num_words != 0 else 0,
                         min(word_lens), max(word_lens)]

            features.append(punct_features + len_stats)

    return np.array(features)


class Tfidf(object):
    def __init__(self, dct):
        self.dct = dct
        self.preprocessor = Preprocessor()

    def calc_idf(self, data_file):
        df = np.zeros(len(self.dct))
        num_docs = 0
        with codecs.open(data_file, 'r', 'utf-8') as f:
            for line in f:
                line = self.preprocessor(line)
                for token in set(self.preprocessor.tokenize(line)):
                    token_idx = self.dct[token]
                    df[token_idx] += 1
                num_docs += 1
        return np.log((num_docs + 1) / float(df + 1)) + 1.

    def calc_tf(self, data_file, cols=None, delimiter='\t'):
        row_ids = []
        col_ids = []
        data = []
        with codecs.open(data_file, 'r', 'utf-8') as f:
            for line_idx, line in enumerate(f):
                line = line.rstrip('\n')
                if cols is not None:
                    parts = line.split(delimiter)
                    line = delimiter.join([parts[col] for col in cols])
                line = self.preprocessor(line)
                num_tokens = 0
                for token in self.preprocessor.tokenize(line):
                    if token in self.dct:
                        row_ids.append(line_idx)
                        col_ids.append(self.dct[token])
                        num_tokens += 1
                if num_tokens != 0:
                    data.extend([1 / float(num_tokens)] * num_tokens)
        return scipy.sparse.csr_matrix((data, (row_ids, col_ids)), shape=(line_idx+1, len(self.dct)))



def main(args):
    if args.build_dct:
        word_dct, idf = build_dct(args.pool, args.dct)
    else:
        word_dct, idf = read_dct(args.dct)

    tfidf = Tfidf(word_dct)

    tf_reprs = []
    stats = []

    #query_2,query_1,query_0,reply,context_2,context_1,context_0
    with codecs.open(args.src) as f:
        num_cols = len(f.readline().rstrip('\n').split('\t'))

    groups = [[0,1,2], [2], [1], [0], [3]]
    if num_cols == 4:
        pass
    elif num_cols == 7:
        groups.extend([[4,5,6], [6], [5], [4]])
    else:
        print >> sys.stderr, 'Invalid number of columns: %d' % num_cols
        sys.exit(1)

    for group in groups:
        tf_reprs.append(tfidf.calc_tf(args.src, cols=group))
        stats.append(get_base_features(args.src, cols=group))

    dists = []
    for i in range(len(groups)):
        for j in range(i + 1, len(groups)):
            M = tf_reprs[i].multiply(tf_reprs[j]).dot((idf**2)[:,None])
            dists.append(M)

    np.savetxt(args.dst, np.hstack(dists + stats), fmt='%.5f', delimiter='\t')



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dct', required=True)
    parser.add_argument('--build-dct', action='store_true')
    parser.add_argument('--pool')
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)

