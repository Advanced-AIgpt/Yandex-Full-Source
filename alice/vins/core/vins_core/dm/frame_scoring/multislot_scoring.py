# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import logging

from vins_core.dm.frame_scoring.base import FrameScoring

logger = logging.getLogger(__name__)


class MultislotFrameScoring(FrameScoring):
    """
    Rescores frame so that if there is a multiple slot not allowed in the form map,
    zero weight is assigned to that frame.
    Also can assign zero score to slots marked as continuation (because this flag should disappear during slot merge).
    """
    def __init__(self, ban_continuation=True, forms_map=None):
        self._ban_continuation = ban_continuation
        if not forms_map:
            logging.warning('Multislot frame scoring was called, but form map was not provided. '
                            'Assume no multiple slots are allowed')
            forms_map = {}
        self._allowed_slots_map = {intent_name: {s.name for s in forms_map[intent_name].slots if s.allow_multiple}
                                   for intent_name, intent_form in forms_map.iteritems()}

    def _is_slot_prohibited(self, slot_name, slot_values, intent_name):
        # ban multiple slots if they are not allowed in the forms map
        if len(slot_values) > 1:
            if intent_name not in self._allowed_slots_map:
                return True
            if slot_name not in self._allowed_slots_map[intent_name]:
                return True
        # ban continuation slots (those starting with I- tag)
        if self._ban_continuation:
            for single_value in slot_values:
                if single_value.get('is_continuation', False):
                    return True
        return False

    def _call(self, frames, session, **kwargs):
        output = []
        for frame in frames:
            score = 1
            for slot_name, slot_values in frame['slots'].iteritems():
                if self._is_slot_prohibited(slot_name, slot_values, frame['intent_name']):
                    score = 0
            output.append(score)
        return output
