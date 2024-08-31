# -*- coding: utf-8 -*-

import logging

from collections import defaultdict
from itertools import izip

from vins_core.common.entity import Entity
from vins_core.nlu.token_tagger import TokenTagger


logger = logging.getLogger(__name__)


class ProtocolSemanticFrameTagger(TokenTagger):
    def __init__(self, matching_score=1.0, **kwargs):
        super(ProtocolSemanticFrameTagger, self).__init__(**kwargs)

        self._matching_score = matching_score

    def predict_slots(self, batch_samples, batch_features, batch_entities, intent=None, req_info=None, **kwargs):
        if not req_info or not req_info.semantic_frames:
            return [], [], []

        form = self._try_get_form(req_info.semantic_frames, intent)
        if not form:
            return [], [], []

        slots, entities_list = self._semantic_frame_to_slots(form)
        scores_list = [self._matching_score]

        batch_slots_list, batch_entities_list, batch_scores_list = [], [], []
        for _ in xrange(len(batch_samples)):
            batch_slots_list.append([slots])
            batch_entities_list.append([entities_list])
            batch_scores_list.append(scores_list)

        out = ['Protocol Tagger output:']
        for slots, entities, scores in izip(batch_slots_list, batch_entities_list, batch_scores_list):
            out.append('{0}\t{1}\t(score={2})'.format(slots, entities, scores))
        logger.debug('\n'.join(out))

        return batch_slots_list, batch_entities_list, batch_scores_list

    @staticmethod
    def _try_get_form(semantic_frames, intent):
        for semantic_frame in semantic_frames:
            if semantic_frame['name'] == intent:
                return semantic_frame

        return None

    @staticmethod
    def _semantic_frame_to_slots(semantic_frame):
        slots, entities = defaultdict(list), []

        for slot in semantic_frame.get('slots', []):
            slot_value = slot['value']
            slot_type = slot['type']
            is_string = slot_type.lower() == 'string' and isinstance(slot_value, basestring)
            substr = slot_value if is_string else ''
            new_slot = dict(
                substr=substr,
                start=0,
                end=1,
                entities=[Entity(start=0, end=1, type=slot_type, value=slot_value, substr='', weight=None)],
                is_continuation=False,
            )
            slots[slot['name']].append(new_slot)

        return slots, entities

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        return self

    def save(self, archive, name):
        pass

    def load(self, archive, name):
        pass
