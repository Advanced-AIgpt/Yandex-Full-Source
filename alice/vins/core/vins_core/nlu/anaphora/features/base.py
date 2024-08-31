# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from collections import OrderedDict
import logging

logger = logging.getLogger(__name__)


class BaseFeatureExtractor(object):
    def add_features(self, context, features):
        raise NotImplementedError


def create_empty_anaphora_features(context):
    features = []
    for phrase_mentions in context.antecedents:
        features.append([OrderedDict() for _ in phrase_mentions])
    return features
