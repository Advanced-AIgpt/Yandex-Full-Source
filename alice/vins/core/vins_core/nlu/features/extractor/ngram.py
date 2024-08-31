# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from itertools import chain
from vins_core.nlu.features.extractor.base import (
    BaseFeatureExtractor, SparseSeqFeatures, EmptyFeatures, SparseFeatures, SparseFeatureValue
)


def _pad_left(sequence, n, symbol=None):
    return chain((symbol,) * (n - 1), iter(sequence))


def _ngrams(sequence, n):
    history = []
    while n > 1:
        history.append(next(sequence))
        n -= 1
    for item in sequence:
        history.append(item)
        yield tuple(history)
        del history[0]


class NGramFeatureExtractor(BaseFeatureExtractor):

    BOS_TAG = '<s>'
    EOS_TAG = '</s>'

    def __init__(self, n=1):
        self._n = n
        super(NGramFeatureExtractor, self).__init__()

    def _split_ngrams(self, iterable, sep):
        ngrams = list(_ngrams(_pad_left(iterable, self._n, symbol=self.BOS_TAG), self._n))

        if len(ngrams) == 0:
            return [EmptyFeatures()]

        out = []
        for i, ngram in enumerate(ngrams):
            value = sep.join(ngram)
            if self._n > 1 and i == len(ngrams) - 1:
                # For last n-gram, end-of-string token is appended.
                value += sep + self.EOS_TAG
            out.append([SparseFeatureValue(value)])
        return [SparseSeqFeatures(out)]

    def _call(self, sample, **kwargs):
        return self._split_ngrams(sample.tokens, ' ')

    @property
    def _features_cls(self):
        return SparseSeqFeatures


class BagOfCharFeatureExtractor(NGramFeatureExtractor):

    def __init__(self, n=3):
        super(BagOfCharFeatureExtractor, self).__init__(n=n)

    def _call(self, sample, **kwargs):
        char_seq = self._split_ngrams(sample.text, '')
        features = char_seq[0]
        if isinstance(features, EmptyFeatures):
            return char_seq
        bag_of_char = list(set(sum(features.data, [])))
        return [SparseFeatures(bag_of_char)]

    @property
    def _features_cls(self):
        return SparseFeatures
