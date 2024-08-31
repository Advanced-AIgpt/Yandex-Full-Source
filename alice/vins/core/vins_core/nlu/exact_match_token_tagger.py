# coding: utf-8
from __future__ import unicode_literals

import logging

from collections import defaultdict
from vins_core.nlu.token_tagger import TokenTagger
from vins_core.utils.misc import ParallelItemError


logger = logging.getLogger(__name__)


class ExactMatchTokenTagger(TokenTagger):
    def __init__(self, nlu_sources_data=None, matching_score=1.0, exact_matched_intents=None,
                 samples_extractor=None, **kwargs):
        super(ExactMatchTokenTagger, self).__init__(**kwargs)

        self._samples_extractor = samples_extractor
        self._exact_matched_intents = exact_matched_intents or set()
        self._loaded_intents = set()
        self._nlu_sources_data = nlu_sources_data
        self._data = defaultdict(defaultdict)
        self._matching_score = matching_score

        if self._nlu_sources_data is not None:
            logger.info("ExactMatchTokenTagger: exact matched intents: %s", ",".join(self._exact_matched_intents))
            for intent_name in self._exact_matched_intents:
                self._load_intent(intent_name)

    def _load_intent(self, intent_name):
        if intent_name in self._loaded_intents:
            return

        logger.info("ExactMatchTokenTagger: loading intent %s", intent_name)
        items = self._nlu_sources_data[intent_name]
        for item in items:
            sample = self._samples_extractor([item])[0]
            if isinstance(sample, ParallelItemError):
                logger.warning("Bad sample. %r", sample)
                continue
            self._data[intent_name][sample.text] = sample.tags
        self._loaded_intents.add(intent_name)

    def predict(self, features, intent, **kwargs):
        check_for_loaded_intents, exact_matched_intents = self._get_exact_matched_intents()

        if self._nlu_sources_data is None or intent not in exact_matched_intents:
            return [[] for _ in features], [[] for _ in features]

        result = []
        scores = []

        if check_for_loaded_intents:
            for intent_name in exact_matched_intents:
                self._load_intent(intent_name)

        for feature in features:
            if feature.sample.text in self._data[intent]:
                result.append([self._data[intent][feature.sample.text]])
                scores.append([self._matching_score])
            else:
                result.append([])
                scores.append([])

        return result, scores

    def _get_exact_matched_intents(self):
        return False, self._exact_matched_intents

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        return self

    def save(self, archive, name):
        pass

    def load(self, archive, name):
        pass
