# coding=utf-8

import numpy as np
import codecs, argparse, string
from collections import Counter
import regex as re


def read_pool(filename):
    pool = []
    with codecs.open(filename, 'r', 'utf-8') as f_i:
        for line in f_i:
            line = line.rstrip('\n')
            pool.append(line.split('\t'))
    return np.array(pool, dtype=object)


def merge_turns(pool):
    text = []
    for parts in pool:
        text.append(' '.join(parts))
    return text


class Male_features_maker(object):
    def __init__(self, pool):
        past_verb_regex = re.compile(u'[иеаяоуы]л[ао]?(с[яь])?\\b', re.UNICODE)
        adjective_regex = re.compile(u'([иыо]й|[ая]я|[ое]е|[ыи]е)\\b', re.UNICODE)
        muzh_regex = re.compile(u'\\b(муж|жен|дев)', re.UNICODE)
        sam_regex = re.compile(u'\\bсама?\\b', re.UNICODE)
        who_regex = re.compile(u'\\b(я|ты|вы|он|она|они|кто|что)\\b', re.UNICODE)

        self.regexes = [past_verb_regex, adjective_regex,
                        muzh_regex, sam_regex, who_regex]

        self.vocab = self.get_pattern_vocab(pool)
        """
        with codecs.open('vocab.txt', 'w', 'utf-8') as f:
            for k, v in self.vocab.iteritems():
                f.write('%s\n' % k)
        """

    def get_pattern_vocab(self, pool):
        cntr = Counter()
        for line in pool:
            for regex in self.regexes:
                for x in re.finditer(regex, line):
                    cntr[x.group(0)] += 1
        vocab = {}
        for idx, (k, v) in enumerate(cntr.most_common()):
            vocab[k] = idx
        return vocab

    def __call__(self, pool):
        pool_f = []
        for line in pool:
            f = [0]*len(self.vocab)
            for regex in self.regexes:
                for x in re.finditer(regex, line):
                    f[self.vocab[x.group(0)]] += 1
            pool_f.append(f)
        return np.array(pool_f)


def main(args):
    #query_2,query_1,query_0,reply,context_2,context_1,context_0
    cols = [2, 1, 0, 3, 6, 5, 4]

    data_train = read_pool(args.input_train)
    data_val = read_pool(args.input_val)
    assert data_train.shape[1] == data_val.shape[1] == 7

    MFM = Male_features_maker(merge_turns(data_train[:,cols]))

    X_train = []
    X_val = []
    for col in cols:
        X_train.append(MFM(data_train[:,col]))
        X_val.append(MFM(data_val[:,col]))

    np.savetxt(args.output_train, np.hstack(X_train), fmt='%.1f', delimiter='\t')
    np.savetxt(args.output_val, np.hstack(X_val), fmt='%.1f', delimiter='\t')



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--input-train', required=True)
    parser.add_argument('--input-val', required=True)
    parser.add_argument('--output-train', required=True)
    parser.add_argument('--output-val', required=True)
    args = parser.parse_args()
    main(args)

