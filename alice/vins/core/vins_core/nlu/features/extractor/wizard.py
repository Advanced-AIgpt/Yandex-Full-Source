# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from collections import defaultdict

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, SparseSeqFeatures, SparseFeatureValue

logger = logging.getLogger(__name__)


class WizardFeatureExtractor(BaseFeatureExtractor):

    DEFAULT_NAME = 'wizard'
    DEFAULT_RULES = ('GeoAddr', 'Date', 'Fio', 'IsNav', 'EntityFinder', 'DirtyLang')

    def __init__(self, rules=None, use_onto=False, use_freebase=False, **kwargs):
        super(WizardFeatureExtractor, self).__init__()

        self._rules = rules or self.DEFAULT_RULES
        self._use_onto = use_onto
        self._use_freebase = use_freebase

    def _call(self, sample, **kwargs):
        if 'wizard' not in sample.annotations:
            logger.warning('Wizard annotation not found in sample = "%s". Can\'t extract wizard features.', sample.text)
            return self.get_default_features(sample)

        wizard_entities = []

        rule_to_call = {
            'GeoAddr': self._rule_geoaddr,
            'Date': self._rule_date,
            'Fio': self._rule_fio,
            'IsNav': self._rule_isnav,
            'Wares': self._rule_wares,
            'EntityFinder': self._rule_entity_finder,
            'DirtyLang': self._rule_dirty_lang,
        }
        for rule in self._rules:
            wizard_entities.extend(rule_to_call[rule](sample, sample.annotations['wizard']))

        return self.make_features(sample=sample, entities=wizard_entities)

    @staticmethod
    def make_features(sample, entities):
        result = [[] for _ in xrange(len(sample))]

        for st, en, name in entities:
            prefix_bio = 'B-'
            for t in xrange(st, en):
                result[t].append(SparseFeatureValue(prefix_bio + name))
                prefix_bio = 'I-'

        return [SparseSeqFeatures(result)]

    @classmethod
    def _rule_geoaddr(cls, sample, annotation):
        markup = annotation.markup
        out = []
        if 'GeoAddr' in markup:
            for item in markup['GeoAddr']:
                if 'Fields' not in item:
                    cls._add_out(out, item, annotation.token_alignment, 'GeoAddr')
                    continue

                for fields in item['Fields']:
                    cls._add_out(out, fields, annotation.token_alignment, 'GeoAddr_%s' % (fields['Type']))
        return out

    @classmethod
    def _rule_date(cls, sample, annotation):
        markup = annotation.markup
        out = []
        if 'Date' in markup:
            for item in markup['Date']:
                tag = 'Date'
                if 'RelativeDay' in item and item['RelativeDay']:
                    tag = 'Date_RelativeDay'
                cls._add_out(out, item, annotation.token_alignment, tag)
        return out

    @classmethod
    def _rule_fio(cls, sample, annotation):
        markup = annotation.markup
        out = []
        if 'Fio' in markup:
            for item in markup['Fio']:
                tag = 'Fio_%s' % (item['Type'])
                cls._add_out(out, item, annotation.token_alignment, tag)
        return out

    @staticmethod
    def _add_out(out, item, token_alignment, tag):
        st = token_alignment[item['Tokens']['Begin']]
        en = token_alignment[item['Tokens']['End'] - 1] + 1
        if st >= 0 and en > 0:
            out.append((st, en, tag))
        else:
            logger.warning('Wizard markup error: unable to align token %r', item)
            logger.warning('Token alignment: %r')

    @classmethod
    def _rule_isnav(cls, sample, annotation):
        rules = annotation.rules
        out = []
        if 'IsNav' in rules:
            if int(rules['IsNav'].get('RuleResult', 0)) > 0:
                out.append((0, len(sample.tokens), 'IsNav'))
        return out

    def _rule_entity_finder(self, sample, annotation):
        rules = annotation.rules
        out = []
        if rules.get('EntityFinder', {}).get('Winner') is None:
            return out

        # List of strings "<token>\t<begin>\t<end>\t<onto_id>\t<weight>\t<category>\t<fb_categories>\t8"
        # <fb_categories> has a format like: "fb:m.05zppz|fb:government.politician"
        winners = rules['EntityFinder']['Winner']
        if isinstance(winners, basestring):
            winners = [winners]
        winners = [winner.split('\t')[1:7] for winner in winners]

        token_to_entity = defaultdict(list)

        for (begin, end, ontoid, weight, category, fbstr) in winners:
            token_to_entity[(int(begin), int(end))].append((weight, ontoid, category, fbstr.split('|')))

        # For each token choose winner with maximum weight.
        for key in token_to_entity:
            (_, ontoid, category, fb_categories) = max(token_to_entity[key], key=lambda x: x[0])

            item = {'Tokens': {'Begin': key[0], 'End': key[1]}}
            # Add OntoID category.
            if self._use_onto:
                self._add_out(out, item, annotation.token_alignment, 'EntityFinder_' + category)

            # Add FreeBase categories.
            if self._use_freebase:
                fb_categories = filter(lambda fb: len(fb) > 0, fb_categories)
                [self._add_out(out, item, annotation.token_alignment, 'EntityFinder_' + fb) for fb in fb_categories]

        return out

    @classmethod
    def _rule_wares(cls, sample, annotation):
        raise NotImplementedError()

    def get_default_features(self, sample=None):
        if sample is None:
            raise ValueError(
                'Unable to get default feature size: %s requires "sample" arg in get_default_features() method',
                self.__class__.__name__
            )
        return [SparseSeqFeatures([[] for _ in xrange(len(sample))])]

    @classmethod
    def _rule_dirty_lang(cls, sample, annotation):
        rules = annotation.rules
        out = []
        if 'DirtyLang' in rules:
            if int(rules['DirtyLang'].get('RuleResult', 0)) > 0:
                out.append((0, len(sample.tokens), 'DirtyLang'))
        return out

    @property
    def _features_cls(self):
        return SparseSeqFeatures
