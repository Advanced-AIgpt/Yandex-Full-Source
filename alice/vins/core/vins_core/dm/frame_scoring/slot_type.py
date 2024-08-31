# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import operator

from vins_core.dm.frame_scoring.base import FrameScoring
from vins_core.common.slots_map_utils import matched

logger = logging.getLogger(__name__)


class SlotTypeFrameScoring(FrameScoring):
    """
    Rescores frame so that if any slot type is mismatched with form's one,
    zero weight is assigned to that frame
    """

    def _call(self, frames, session, **kwargs):
        forms_map = kwargs.get('forms_map')
        if not forms_map:
            logging.warning(
                'Frame scoring targeting slot''s type has been specified, '
                'but form''s map wasn''t provided. Scoring skipped.'
            )
            return self._skip(frames)
        output = []
        for frame in frames:
            intent = frame['intent_name']
            valid_slot_types = {s.name: s.types for s in forms_map[intent].slots}
            matching_types = {s.name: s.matching_type for s in forms_map[intent].slots}
            slot_matches = {
                slot_name: self._match(slot_values, valid_slot_types.get(slot_name), matching_types.get(slot_name))
                for slot_name, slot_values in frame['slots'].iteritems()
            }
            output.append(all(slot_matches.values()))
        return output

    @classmethod
    def _match(cls, slot_values, target_types, matching_type):
        return all(any(matched(current_slot_value, target_types, matching_type)) for current_slot_value in slot_values)


class BaseSlotPropertyFrameScoring(FrameScoring):
    """
    If current frame contains slots defined by _get_target_slots predicate,
    then its confidence will be scaled by boosting_score
    """
    _DONT_BOOST = 1.

    def __init__(self, boosting_score=None):
        self._boosting_score = boosting_score if boosting_score is not None else self._DONT_BOOST

    def _call(self, frames, session, **kwargs):
        form = session.form
        target_slots = map(operator.attrgetter('name'), self._get_target_slots(form)) if form else []
        if not target_slots:
            return self._skip(frames)
        output = []
        for frame in frames:
            has_target_slots = len(set(target_slots) & set(frame['slots'])) > 0
            scale = self._boosting_score if has_target_slots else self._DONT_BOOST
            output.append(scale * frame['confidence'])
        return output

    def _get_target_slots(self, form):
        raise NotImplementedError()


class ActiveSlotFrameScoring(BaseSlotPropertyFrameScoring):

    def _get_target_slots(self, form):
        return form.active_slots()


class DisabledSlotFrameScoring(BaseSlotPropertyFrameScoring):

    def _get_target_slots(self, form):
        return form.disabled_slots()
