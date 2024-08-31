# -*- coding: utf-8 -*-

import logging

from vins_core.nlu.classifier import Classifier

logger = logging.getLogger(__name__)


# Add an intent you want to be processed by GranetClassifier by default here:
_DEFAULT_ALLOWED_INTENTS = {
}


class GranetClassifier(Classifier):
    def __init__(self, matching_score=1.0, **kwargs):
        super(GranetClassifier, self).__init__(**kwargs)

        self._matching_score = matching_score

    def _process(self, sample_features, req_info=None, **kwargs):
        experiment_allowed_intents = set()
        experiment_allowed_prefix = None
        if req_info:
            if req_info.experiments['granet_classifier'] is not None:
                experiment_allowed_intents = set(req_info.experiments['granet_classifier'].split(','))
            experiment_allowed_prefix = req_info.experiments['granet_classifier_prefix']

        parsed_by_granet_intents = sample_features.sparse.get('granet', [])
        return {
            intent.value: self._matching_score
            for intent in parsed_by_granet_intents
            if intent.value in _DEFAULT_ALLOWED_INTENTS or intent.value in experiment_allowed_intents or
            (experiment_allowed_prefix and intent.value.startswith(experiment_allowed_prefix))
        }

    @property
    def classes(self):
        return []

    @property
    def default_score(self):
        return 1.0

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
