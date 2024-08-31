# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.nlu.classifier import Classifier
from collections import defaultdict


logger = logging.getLogger(__name__)


class ExactMatchClassifier(Classifier):
    """
    Intent classifier based on raw nlu data. Returns 1.0, if intent contains precise phrase and 0 otherwise.
    """
    def __init__(self, nlu_sources_data, default_score=0.0, matching_score=1.0, exact_matched_intents=None,
                 samples_extractor=None, **kwargs):
        """
        :param nlu_sourses: list of NLUSourceItems objects.
        """
        super(ExactMatchClassifier, self).__init__(**kwargs)
        self._samples_extractor = samples_extractor
        self._classes = {}
        self._loaded_intents = set()
        self._exact_matched_intents = exact_matched_intents or set()
        self._nlu_sources_data = nlu_sources_data
        self._data = defaultdict(set)
        self._default_score = default_score
        self._matching_score = matching_score

        logger.info("ExactMatchClassifier: exact matched intents: %s", ",".join(self._exact_matched_intents))
        for intent_name in self._exact_matched_intents:
            self._load_intent(intent_name)

    def _load_intent(self, intent_name):
        if intent_name in self._loaded_intents:
            return

        logger.info("ExactMatchClassifier: loading intent %s", intent_name)
        items = self._nlu_sources_data[intent_name]
        class_index = len(self._classes)
        self._classes[class_index] = intent_name
        self._loaded_intents.add(intent_name)
        for item in items:
            self._data[self._normalize(item.text)].add(class_index)

    def _process(self, feature, **kwargs):
        check_for_loaded_intents, exact_matched_intents = self._get_exact_matched_intents()

        if check_for_loaded_intents:
            for intent_name in exact_matched_intents:
                self._load_intent(intent_name)

        return {self._classes[class_index]: self._matching_score for class_index in self._data[feature.sample.text] if
                self._classes[class_index] in exact_matched_intents}

    def _normalize(self, text):
        return self._samples_extractor([text], is_inference=False, normalize_only=True)[0].text

    def _get_exact_matched_intents(self):
        return False, self._exact_matched_intents

    @property
    def classes(self):
        return self._classes

    @property
    def default_score(self):
        return self._default_score

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
