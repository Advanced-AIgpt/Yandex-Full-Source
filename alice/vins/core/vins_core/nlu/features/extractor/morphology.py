# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.utils.lemmer import Lemmer
from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, SparseSeqFeatures, SparseFeatureValue


rumorph = Lemmer(['ru', 'en'])


class LemmaFeatureExtractor(BaseFeatureExtractor):

    def _call(self, sample, **kwargs):
        return [SparseSeqFeatures(map(lambda token: [SparseFeatureValue(self._lemmatize(token))], sample.tokens))]

    @classmethod
    def _lemmatize(cls, word):
        return rumorph.parse(word)[0].normal_form

    @property
    def _features_cls(self):
        return SparseSeqFeatures


class PosTagFeatureExtractor(BaseFeatureExtractor):

    _UNKNOWN = 'UNKN'

    def _call(self, sample, **kwargs):
        return [SparseSeqFeatures(map(lambda token: [SparseFeatureValue(self._pos_tag(token))], sample.tokens))]

    @classmethod
    def _pos_tag(cls, word):
        pos_tag = rumorph.parse(word)[0].tag.POS
        if not pos_tag:
            return cls._UNKNOWN
        else:
            return unicode(pos_tag)

    @property
    def _features_cls(self):
        return SparseSeqFeatures


class CaseFeatureExtractor(BaseFeatureExtractor):

    _UNKNOWN = 'UNKN'

    def _call(self, sample, **kwargs):
        return [SparseSeqFeatures(map(lambda token: [SparseFeatureValue(self._case(token))], sample.tokens))]

    @classmethod
    def _case(cls, word):
        if rumorph.parse(word)[0].tag.POS in ('NOUN', 'ADJF', 'PRTF', 'NUMR'):
            return rumorph.parse(word)[0].tag.case or cls._UNKNOWN
        else:
            return cls._UNKNOWN

    @property
    def _features_cls(self):
        return SparseSeqFeatures
