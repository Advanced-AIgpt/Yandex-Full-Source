# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.nlu.classifier import Classifier


logger = logging.getLogger(__name__)


class DirtyLangClassifier(Classifier):
    """
    Intent classifier based on the Behemoth's DirtyLang rule. Returns 1.0, if wizard features content dirty lang mark
    """
    def __init__(self, fallback_intent, **kwargs):
        super(DirtyLangClassifier, self).__init__(**kwargs)
        self._fallback_intent = fallback_intent
        self._classes = {self._fallback_intent: 0}

    def _process(self, feature, **kwargs):
        prob = 0.0
        if 'wizard' in feature.sparse_seq:
            if any(feature.value == 'B-DirtyLang' for features in feature.sparse_seq['wizard'] for feature in features):
                prob = 1.0

        return {self._fallback_intent: prob}

    @property
    def classes(self):
        return self._classes

    @property
    def default_score(self):
        return 1.0

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
