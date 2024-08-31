# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.anaphora.features.base import create_empty_anaphora_features

logger = logging.getLogger(__name__)


class BaseAnaphoraMatcher(object):
    def __init__(self, **kwargs):
        self._feature_extractors = []

    def _extract_features(self, anaphoric_context):
        features = create_empty_anaphora_features(anaphoric_context)
        for feature_extractor in self._feature_extractors:
            feature_extractor.add_features(anaphoric_context, features)
        return features

    def match(self, anaphoric_context):
        raise NotImplementedError
