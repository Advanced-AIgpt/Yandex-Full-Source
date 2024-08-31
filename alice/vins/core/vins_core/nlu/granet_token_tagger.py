# -*- coding: utf-8 -*-

import attr
import logging

from vins_core.nlu.token_tagger import TokenTagger
from vins_core.utils.sequence_alignment import align_sequences

logger = logging.getLogger(__name__)


# Add an intent you want to be processed by GranetTokenTagger by default here:
_DEFAULT_ALLOWED_INTENTS = {
}


class GranetTokenTagger(TokenTagger):
    def __init__(self, matching_score=1.0, **kwargs):
        super(GranetTokenTagger, self).__init__(**kwargs)

        self._matching_score = matching_score

    def predict(self, batch_features, batch_samples=None, intent=None, req_info=None, **kwargs):
        batch_hypotheses, batch_scores = [], []

        for sample in batch_samples:
            hypothesis, score = self._predict_for_sample(sample, intent, req_info)
            batch_hypotheses.append(hypothesis)
            batch_scores.append(score)

        return batch_hypotheses, batch_scores

    def _get_sample_tokens(self, sample):
        tokens = sample.tokens or []
        granet_rule = self._get_granet_response(sample)
        if granet_rule and 'Tokens' in granet_rule:
            tokens = granet_rule['Tokens'] or []
            tokens = [token['Text'] for token in tokens]
        return tokens

    def _get_entities(self, sample, tokens, entities):
        def _is_fully_aligned_entity(entity):
            return all(
                0 <= index < len(alignment) and alignment[index] != -1
                for index in xrange(entity.start, entity.end)
            )

        def _update_entity(entity):
            new_start = alignment[entity.start]
            new_end = new_start + entity.end - entity.start
            return attr.evolve(entity, start=new_start, end=new_end)

        alignment = align_sequences(sample.tokens, tokens)

        return [_update_entity(entity) for entity in entities if _is_fully_aligned_entity(entity)]

    def _predict_for_sample(self, sample, intent, req_info):
        experiment_allowed_intents = []
        is_exp_prefix = False
        if req_info:
            if req_info.experiments['granet_tagger'] is not None:
                experiment_allowed_intents = req_info.experiments['granet_tagger'].split(',')
            experiment_allowed_prefix = req_info.experiments['granet_tagger_prefix']
            is_exp_prefix = experiment_allowed_prefix and intent.startswith(experiment_allowed_prefix)

        if intent not in _DEFAULT_ALLOWED_INTENTS and intent not in experiment_allowed_intents and not is_exp_prefix:
            return self._get_default_prediction_for_sample()

        granet_response = self._get_granet_response(sample)
        form = self._try_get_intent_form(granet_response, intent)
        if not form:
            return self._get_default_prediction_for_sample()

        tags = form.get('Tags', [])
        intent_tagging = ['O'] * len(granet_response.get('Tokens', []))
        for tag in tags:
            self._add_tag(tag, intent_tagging)

        return [intent_tagging], [self._matching_score]

    @staticmethod
    def _try_get_intent_form(granet_response, intent):
        parsed_forms = granet_response.get('Forms', [])
        for parsed_form in parsed_forms:
            if parsed_form['Name'] == intent:
                return parsed_form

        return None

    def _get_granet_response(self, sample):
        if 'wizard' in sample.annotations:
            wizard_rules = sample.annotations['wizard'].rules
            return wizard_rules.get('Granet', {})
        return {}

    @staticmethod
    def _add_tag(tag, result_tags):
        prefix_bio = 'B-'
        for position in xrange(tag['Begin'], tag['End']):
            result_tags[position] = prefix_bio + tag['Name']
            prefix_bio = 'I-'

    @staticmethod
    def _get_default_prediction_for_sample():
        return [], []

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        return self

    def save(self, archive, name):
        pass

    def load(self, archive, name):
        pass
