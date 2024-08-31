# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.nlu.token_tagger import create_token_tagger
from vins_core.nlu.token_tagger import TokenTagger


logger = logging.getLogger(__name__)


class CombineScoresTokenTagger(TokenTagger):
    def __init__(self, taggers, **kwargs):
        super(CombineScoresTokenTagger, self).__init__(**kwargs)
        self._taggers = []
        for tagger_config in taggers:
            params = tagger_config.get('params', {})
            tagger_config.update(**kwargs)
            tagger_config.update(**params)

            self._taggers.append(create_token_tagger(
                **tagger_config
            ))

    def predict(self, features, verbose=True, **kwargs):
        final_slots_list = [[] for _ in xrange(len(features))]
        final_scores_list = [[] for _ in xrange(len(features))]
        for tagger in self._taggers:
            slots_list, scores_list = tagger.predict(features, verbose=verbose, **kwargs)
            if slots_list:
                for i in xrange(len(features)):
                    final_slots_list[i].extend(slots_list[i])
                    final_scores_list[i].extend(scores_list[i])

        return final_slots_list, final_scores_list

    def predict_slots(self, batch_samples, batch_features, batch_entities, **kwargs):
        final_batch_slots_list = [[] for _ in xrange(len(batch_features))]
        final_batch_entities_list = [[] for _ in xrange(len(batch_features))]
        final_batch_score_list = [[] for _ in xrange(len(batch_features))]

        for tagger in self._taggers:
            slots_list, entities_list, score_list = tagger.predict_slots(
                batch_samples, batch_features, batch_entities, **kwargs
            )
            if slots_list:
                for i in xrange(len(batch_features)):
                    final_batch_slots_list[i].extend(slots_list[i])
                    final_batch_entities_list[i].extend(entities_list[i])
                    final_batch_score_list[i].extend(score_list[i])

        return final_batch_slots_list, final_batch_entities_list, final_batch_score_list

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        for tagger in self._taggers:
            tagger.train(intent_to_features, reset_model=reset_model, intents_to_train=intents_to_train, **kwargs)
        return self

    def save(self, archive, name=None):
        for tagger in self._taggers:
            tagger.save(archive, tagger.name)
        return self

    def load(self, archive, name=None):
        for tagger in self._taggers:
            tagger.load(archive, tagger.name)
