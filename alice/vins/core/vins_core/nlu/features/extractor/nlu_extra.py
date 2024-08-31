# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, SparseSeqFeatures
from vins_core.nlu.features.extractor.wizard import WizardFeatureExtractor

logger = logging.getLogger(__name__)


class NluExtraFeaturesExtractor(BaseFeatureExtractor):
    def _call(self, sample, **kwargs):
        if 'extra' not in sample.annotations:
            return self.get_default_features(sample)

        entities = []
        extra_annotation = sample.annotations.get('extra')

        entities += self._collect_entities(sample.text, entities=extra_annotation.rooms, entities_type='room')
        entities += self._collect_entities(sample.text, entities=extra_annotation.groups, entities_type='group')
        entities += self._collect_entities(sample.text, entities=extra_annotation.devices, entities_type='device')
        entities += self._collect_entities(sample.text, entities=extra_annotation.multiroom_all_devices,
                                           entities_type='multiroom_all_devices')

        return WizardFeatureExtractor.make_features(sample=sample, entities=entities)

    @staticmethod
    def _collect_entities(sample_text, entities, entities_type):
        result = []
        for entity_tokens in entities:
            start = sample_text[:sample_text.find(entity_tokens)].count(' ')
            end = start + entity_tokens.count(' ') + 1
            if start != -1:
                result.append((start, end, 'Location_{}'.format(entities_type)))
        return result

    @property
    def _features_cls(self):
        return SparseSeqFeatures

    def get_default_features(self, sample=None):
        return WizardFeatureExtractor.make_features(sample=sample, entities=[])
