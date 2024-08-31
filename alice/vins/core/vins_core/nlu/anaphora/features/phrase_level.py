# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.anaphora.features.base import BaseFeatureExtractor

logger = logging.getLogger(__name__)


class PhraseLevelFeaturesExtractor(BaseFeatureExtractor):
    def add_features(self, context, features):
        for phrase_id, (phrase_features, phrase_sender) in enumerate(reversed(zip(features, context.senders))):
            for mention_features in phrase_features:
                if context.same_request_mentions:
                    phrase_id = 1
                mention_features['distance_in_phrases'] = phrase_id
                mention_features['user_phrase'] = int(phrase_sender == 'user')
