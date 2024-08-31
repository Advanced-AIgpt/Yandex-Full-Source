# coding=utf-8
import yt.wrapper as yt
import numpy as np
import regex as re
import codecs, argparse, sys, os, string
from collections import Counter

yt.config.set_proxy("hahn.yt.yandex.net")


class Docs_extractor(object):
    def __init__(self, from_pool=False):
        self.from_pool = from_pool

    def __call__(self, row):
        if not (self.from_pool and (len(row['context_1']) != 0 or len(row['context_2']) != 0)):
            yield {'doc': row['context_0']}
        yield {'doc': row['reply']}


class Preprocessor(object):
    def __init__(self):
        self.tokenize_regex = re.compile(u'([' + string.punctuation + ur'\\])')
        self.yo_regex = re.compile(u'[ёë]', re.UNICODE)

    def __call__(self, line):
        line = line.decode('utf-8')
        line = self.tokenize_regex.sub(ur' \1 ', line)
        line = '\t'.join([re.sub('\s+', ' ', turn, flags=re.UNICODE).strip()
                          for turn in line.split('\t')])
        line = line.lower()
        line = self.yo_regex.sub(u'е', line)
        return line

    def tokenize(self, line):
        line = re.sub('\s+', ' ', line, flags=re.UNICODE).strip()
        return line.split(' ')


class Mapper(object):
    def __init__(self):
        self.preprocessor = Preprocessor()

    def __call__(self, row):
        doc = self.preprocessor(row['doc'])
        cntr = Counter(self.preprocessor.tokenize(doc))
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
        idf = np.log((self.num_docs + 1) / float(df + 1)) + 1
        yield {'token': key['token'], 'freq': freq, 'idf': idf, 'inv_freq': -freq}


def main(args):
    with yt.TempTable(os.path.dirname(args.src)) as temp:
        yt.run_map(Docs_extractor(args.from_pool), args.src, temp)
        num_docs = yt.row_count(temp)
        yt.run_map(Mapper(), temp, temp)
        yt.run_sort(temp, temp, sort_by=['token'])
        yt.run_reduce(Reducer(num_docs), temp, args.dst, reduce_by=['token'])
    yt.run_sort(args.dst, args.dst, sort_by=['inv_freq', 'token'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst')
    parser.add_argument('--from-pool', action='store_true')
    args = parser.parse_args()
    main(args)

