# coding=utf-8

import numpy as np
import codecs, argparse, os
from collections import defaultdict, Counter
import regex as re
from preprocess_text import Preprocessor


def read_pool(filename):
    pool = []
    preprocessor = Preprocessor()
    with codecs.open(filename, 'r', 'utf-8') as f_i:
        for line in f_i:
            pool.append(preprocessor(line))
    return pool


class Features_maker(object):
    def __init__(self, ruslister_map_filename=None,
                 pattern_map_filename=None):
        who_regex = re.compile(u'\\b(я|ты|вы|он|она|они|кто|что)\\b', re.UNICODE)
        sam_regex = re.compile(u'\\bсам[аои]?\\b', re.UNICODE)
        danet_regex = re.compile(u'\\b(да|нет)\\b', re.UNICODE)
        self.regexes = [who_regex, sam_regex, danet_regex]

        if ruslister_map_filename and pattern_map_filename:
            self._read_maps(ruslister_map_filename, pattern_map_filename)
            self.num_ruslister_features = self._get_num_ruslister_features()
            #print 'num_features = %d' % (self.num_ruslister_features + len(self.pattern_map))


    def fit(self, ruslister_dct_filename, pool, ruslister_map_filename,
            pattern_map_filename):
        self.ruslister_map = self._build_ruslister_map(ruslister_dct_filename)
        self.pattern_map = self._build_pattern_map(pool)
        self._dump_maps(ruslister_map_filename, pattern_map_filename)
        self.num_ruslister_features = self._get_num_ruslister_features()
        #print 'num_features = %d' % (self.num_ruslister_features + len(self.pattern_map))


    def _read_maps(self, ruslister_map_filename,
                   pattern_map_filename):
        self.ruslister_map = {}
        with codecs.open(ruslister_map_filename, 'r', 'utf-8') as f:
            for line in f:
                key, value = line.rstrip('\n').split('\t')
                self.ruslister_map[key] = [int(x) for x in value.split(' ')]

        self.pattern_map = {}
        with codecs.open(pattern_map_filename, 'r', 'utf-8') as f:
            for line in f:
                key, value = line.rstrip('\n').split('\t')
                self.pattern_map[key] = int(value)


    def _dump_maps(self, ruslister_map_filename,
                   pattern_map_filename):
        with codecs.open(ruslister_map_filename, 'w', 'utf-8') as f:
            for word, ids in sorted(self.ruslister_map.iteritems()):
                f.write('%s\t%s\n' % (word, ' '.join([str(idx) for idx in ids])))

        with codecs.open(pattern_map_filename, 'w', 'utf-8') as f:
            for pattern, idx in sorted(self.pattern_map.iteritems(), key=lambda x: x[1]):
                f.write('%s\t%d\n' % (pattern, idx))


    def _build_pattern_map(self, pool):
        cntr = Counter()
        for line in pool:
            for regex in self.regexes:
                for x in re.finditer(regex, line):
                    cntr[x.group(0)] += 1
        vocab = {}
        for idx, (k, v) in enumerate(cntr.most_common()):
            vocab[k] = idx
        return vocab


    def _build_ruslister_map(self, ruslister_filename):
        #POSes = ['V', 'A', 'S', 'APRO', 'SPRO', 'NUM', 'ANUM']
        sexes = ['f', 'm', 'n']
        names = ['persn', 'famn', 'patrn']

        word2attribs = defaultdict(set)
        with codecs.open(ruslister_filename, 'r', 'utf-8') as f:
            possex_set = set()
            for line in f:
                parts = line.rstrip('\n').split('\t')
                if len(parts) == 2:
                    word_dirty = parts[0].lower()
                    word = ''
                    for char in word_dirty:
                        if char not in '[]':
                            word += char

                    attributes = parts[1].split(',')

                    POS = attributes[0]

                    sex_set = set(sexes) & set(attributes)
                    assert len(sex_set) < 2
                    sex = sex_set.pop() if len(sex_set) == 1 else ''

                    possex = POS if sex == '' else POS + '_' + sex
                    possex_set.add(possex)
                    word2attribs[word] |= {possex}

                    name_set = set(names) & set(attributes)
                    assert len(name_set) < 2
                    name = name_set.pop() if len(name_set) == 1 else ''
                    if name != '':
                        word2attribs[word] |= {name}

        attrib2idx = {}
        idx = 0
        for possex in sorted(possex_set):
            attrib2idx[possex] = idx
            idx += 1
        for name in names:
            attrib2idx[name] = idx
            idx += 1
        word2ids = {}
        for word, attribs in word2attribs.iteritems():
            word2ids[word] = sorted({attrib2idx[attrib] for attrib in attribs})
        return word2ids

    def _get_num_ruslister_features(self):
        return max(max(ids) for _, ids in self.ruslister_map.iteritems()) + 1

    def __call__(self, pool):
        pool_features = []
        for line in pool:
            line_features = []
            for turn in line.split('\t'):
                f_pattern = [0] * len(self.pattern_map)
                for regex in self.regexes:
                    for x in re.finditer(regex, turn):
                        f_pattern[self.pattern_map[x.group(0)]] += 1
                f_ruslister = [0] * self.num_ruslister_features
                for word in turn.split(' '):
                    if word in self.ruslister_map:
                        for idx in self.ruslister_map[word]:
                            f_ruslister[idx] += 1
                line_features.extend(f_ruslister + f_pattern)
            pool_features.append(line_features)
        return np.array(pool_features)


def main(args):
    #query_2,query_1,query_0,reply,context_2,context_1,context_0

    pool = read_pool(args.src)
    assert len(pool[0].split('\t')) == 7

    if args.build_maps:
        FM = Features_maker()
        FM.fit(args.ruslister_dct, pool,
               args.ruslister_map, args.pattern_map)
    else:
        FM = Features_maker(args.ruslister_map, args.pattern_map)

    np.savetxt(args.dst, FM(pool), fmt='%.1f', delimiter='\t')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--ruslister-map', required=True)
    parser.add_argument('--pattern-map', required=True)
    parser.add_argument('--ruslister-dct')
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst')
    parser.add_argument('--build-maps', action='store_true')
    args = parser.parse_args()
    main(args)
