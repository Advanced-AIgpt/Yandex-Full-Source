# coding: utf-8
from __future__ import unicode_literals

import logging

from abc import ABCMeta, abstractmethod
from requests import RequestException

from vins_core.ext.general_conversation import GeneralConversationAPI
from vins_core.dm.response import FormRestoredMeta

logger = logging.getLogger(__name__)


class EventHandler(object):
    __metaclass__ = ABCMeta

    def __init__(self, **kw):
        for k, v in kw.items():
            setattr(self, k, v)

    @abstractmethod
    def handle(self, **kw):
        pass


class NLGEventHandler(EventHandler):
    def handle(self, form, session, req_info, response, app, context=None, **kw):
        render_result = app.render_phrase(self.phrase_id, form=form, context=context, req_info=req_info)
        app.track_nlg_render_history_record(req_info, response, phrase_id=self.phrase_id, form=form, context=context)
        if getattr(self, 'question', False):
            response.ask(render_result.voice, render_result.text, append=True)
        else:
            response.say(render_result.voice, render_result.text, append=True)


class SayEventHandler(EventHandler):
    def handle(self, response, **kw):
        if getattr(self, 'question', False):
            response.ask(self.say, append=True)
        else:
            response.say(self.say, append=True)


class CallbackEventHandler(EventHandler):
    def handle(self, session, app, **kwargs):
        params = dict(getattr(self, 'params', {}))
        params.update(kwargs)
        app.handle_callback(callback_name=self.name, session=session, **params)


def _clear_slot(form, slot_name):
    slot = form.get_slot_by_name(slot_name)
    if slot:
        slot.reset_value()
    else:
        logger.warning('Not found slot with name "%s"', slot_name)


class ClearSlotEventHandler(EventHandler):
    def handle(self, form, **kwargs):
        _clear_slot(form, self.slot)


class ClearSlotsIfAnyUpdatedEventHandler(EventHandler):
    def handle(self, form, frame, **kwargs):
        if any(slot_name in frame['slots'] for slot_name in self.slots_to_check):
            for slot_name in self.slots_to_clear:
                _clear_slot(form, slot_name)


class ChangeSlotPropertiesEventHandler(EventHandler):
    def handle(self, form, **kwargs):
        new_optional_value = getattr(self, 'optional', None)
        if new_optional_value:
            form.get_slot_by_name(self.slot).optional = new_optional_value


class RestorePrevFormEventHandler(EventHandler):
    def handle(self, session, app, form, req_info, response, add_overriden_meta=True, **kwargs):
        last_turn = session.dialog_history.last()
        prev_form = last_turn and last_turn.form

        if prev_form is not None:
            prev_form.is_overridden = True
            app.change_form(session=session, form=prev_form, rerun_dm=False)
            if add_overriden_meta:
                response.add_meta(FormRestoredMeta(overriden_form=form.name))
        else:
            nothing_restored_phrase_id = getattr(self, 'nothing_restored_phrase_id', None)
            if nothing_restored_phrase_id is not None:
                nothing_restored_render_result = app.render_phrase(nothing_restored_phrase_id, form, context=None)
                app.track_nlg_render_history_record(req_info, response, phrase_id=nothing_restored_phrase_id, form=form)
                response.say(nothing_restored_render_result.voice, nothing_restored_render_result.text)


class GeneralConversationEventHandler(EventHandler):
    def __init__(self, **kwargs):
        super(GeneralConversationEventHandler, self).__init__(**kwargs)
        self._gc = GeneralConversationAPI(**self._get_gc_config())

    def handle(self, form, sample, session, app, req_info, response, **kwargs):
        max_context_len = self.max_context_len if hasattr(self, 'max_context_len') else 3
        prev_phrases = [lp.text for lp in session.dialog_history.last_phrases(session, count=max_context_len)]
        prev_phrases.append(sample.text)
        try:
            gc_response = self._gc.handle(prev_phrases[-max_context_len:])
            response.say(gc_response)
        except RequestException:
            error_phrase_id = getattr(self, 'error_phrase_id', 'error')
            error_render_result = app.render_phrase(error_phrase_id, form, context=None)
            app.track_nlg_render_history_record(req_info, response, phrase_id=error_phrase_id, form=form)
            response.say(error_render_result.voice, error_render_result.text)

    def _get_gc_config(self):
        return getattr(self, 'gc_config', {})
