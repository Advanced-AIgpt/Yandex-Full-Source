# coding: utf-8
from __future__ import unicode_literals

import logging
import inspect
import re

from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.session import InMemorySessionStorage
from vins_core.dm.response import NlgRenderHistoryRecord, VinsResponse
from vins_core.config.app_config import AppConfig
from vins_core.utils.data import load_data_from_file
from vins_core.utils.metrics import sensors
from vins_core.utils.misc import str_to_bool


logger = logging.getLogger(__name__)


def callback_method(f):
    f._callback_method = True
    return f


class VinsApp(object):
    """
    Example:

        class SomeApp(VinsApp):
            @callback_method
            def submit(self, uuid, form, client_time, **kwargs):
                # do something with form
                return ResetAction()

            @callback_method
            def dummy(self, uuid, form, client_time, **kwargs):
                pass


        app = SomeApp(vins_file='/path/to/Vinsfile.json')
        app.handle_request(request_info, **kwargs)
    """
    dm_class = DialogManager

    def __init__(self, dm=None, vins_file=None,
                 session_storage=None, app_id='local_app',
                 app_conf=None, intent_renames=None, nlg_checks=None, **kwargs):
        """
        Args:
            dm (vins_core.dm.DialogManager): DialogManager instance.
            vins_file (str or unicode): Path to vins config file (json or yaml).
            app_conf (vins_core.config.app_config.AppConfig): AppConfig instance.
            session_storage (vins_core.dm.session.BaseSessionStorage, optional): If not set,
                InMemorySessionStorage will be used.
            app_id (str or unicode, optional): Should be unique for one storage, default is 'local_app'.
        """
        self._session_storage = session_storage or InMemorySessionStorage()
        if dm:
            self._dm = dm
        elif vins_file or app_conf:
            if not app_conf:
                app_conf = AppConfig()
                app_conf.parse_vinsfile(vins_file, nlg_checks=nlg_checks)
            self._dm = self.dm_class.from_config(app_conf, **kwargs)
        self._app_id = app_id
        method_names = [attr for attr in dir(self) if inspect.ismethod(getattr(self, attr))]
        callback_method_names = filter(lambda x: getattr(getattr(self, x), '_callback_method', False), method_names)
        self._callbacks_map = {
            method_name: getattr(self, method_name)
            for method_name in callback_method_names
        }
        if intent_renames:
            self._intent_renames = load_data_from_file(intent_renames)
        else:
            self._intent_renames = None

    def load_or_create_session(self, req_info):
        return self._session_storage.load_or_create(
            self._app_id,
            req_info.uuid,
            req_info=req_info
        )

    def save_session(self, session, req_info, response=None):
        self._session_storage.save(session, req_info=req_info, response=response)

    def _load_session(self, req_info):
        session = self.load_or_create_session(req_info)
        logger.debug('Session loaded: %s', session)

        # A request might initiate a new session
        if req_info.reset_session:
            logger.debug('Session cleared as requested')
            session.clear()

        # A session might contain intents that are no longer valid DIALOG-502
        intent_name = session.intent_name
        if intent_name is not None and not self._dm.has_intent(intent_name):
            logger.warning('Unknown intent %s found in session, resetting.', intent_name)
            session.clear()

        return session

    def has_phrase(self, phrase_id, intent_name=None):
        return self._dm.nlg is not None and self._dm.nlg.has_phrase(phrase_id=phrase_id, intent_name=intent_name)

    def has_card(self, card_id, intent_name=None):
        return self._dm.nlg is not None and self._dm.nlg.has_card(card_id=card_id, intent_name=intent_name)

    def has_cardtemplate(self, template_id, intent_name=None):
        return self._dm.nlg is not None and self._dm.nlg.has_cardtemplate(template_id=template_id, intent_name=intent_name)

    def render_phrase(self, phrase_id, form=None, context=None, req_info=None, postprocess_list=None, session=None):
        return self._dm.nlg.render_phrase(
            phrase_id=phrase_id, form=form, context=context, req_info=req_info, postprocess_list=postprocess_list,
            session=session
        )

    def render_card(self, card_id, form=None, context=None, req_info=None, schema=None):
        return self._dm.nlg.render_card(card_id=card_id, schema=schema, form=form, context=context, req_info=req_info)

    def render_cardtemplate(self, template_id, form=None, context=None, req_info=None, schema=None):
        return self._dm.nlg.render_cardtemplate(template_id=template_id, schema=schema, form=form, context=context, req_info=req_info)

    def track_nlg_render_history_record(self, req_info, response, phrase_id=None, card_id=None, form=None, context=None):
        if not response:
            return

        nlg_render_history_record = NlgRenderHistoryRecord(phrase_id=phrase_id, card_id=card_id)

        if req_info and str_to_bool(req_info.experiments['dump_nlg_render_context']):
            nlg_render_history_record.req_info = req_info
            nlg_render_history_record.form = form
            nlg_render_history_record.context = context

        response.add_nlg_render_history_record(nlg_render_history_record)

    def _nlg_callback_impl(self, req_info, response, phrase_id, form, context=None, question=False, append=True):
        render_result = self.render_phrase(phrase_id, form=form, req_info=req_info, context=context)
        self.track_nlg_render_history_record(req_info, response, phrase_id=phrase_id, form=form, context=context)
        if question:
            response.ask(render_result.voice, render_result.text, append=append)
        else:
            response.say(render_result.voice, render_result.text, append=append)

    @callback_method
    def nlg_callback(self, req_info, session, response, phrase_id, form=None, question=False, append=True, **kwargs):
        self._nlg_callback_impl(req_info, response, phrase_id, form=form, question=question, append=append)

    @sensors.with_timer('app_handle_request_time')
    def _handle_request(self, req_info, session=None, **kwargs):
        if session is None:
            with sensors.timer('app_load_session_time'):
                session = self._load_session(req_info)

        # Clearing annotations before each turn.
        if session.annotations:
            session.annotations.clear()

        response_class = kwargs.get('response_class')
        if response_class:
            response = response_class()
        else:
            response = VinsResponse()

        skip_relevant_intent_for_session = kwargs.get('skip_relevant_intent_for_session')
        if skip_relevant_intent_for_session and session.intent_name in skip_relevant_intent_for_session:
            logger.info('set skip_relevant_intent for handle_request by hints: %r', skip_relevant_intent_for_session)
            req_info.additional_options['skip_relevant_intents'] = True

        nlu_result = None
        if req_info.utterance:
            nlu_result = self._dm.handle(
                session=session,
                req_info=req_info,
                response=response,
                app=self,
                **kwargs
            )
        elif req_info.callback_name:
            if req_info.callback_args:
                kwargs.update(req_info.callback_args)
            self.handle_callback(
                session, req_info, req_info.callback_name, session.form, response=response, **kwargs)

        if self.should_store_in_history(session):
            session.dialog_history.add(
                utterance=req_info.utterance,
                response=response,
                form=session.form,
                datetime=req_info.client_time,
                annotations=session.annotations
            )

        with sensors.timer('app_save_session_time'):
            self.save_session(session, req_info=req_info, response=response)

        return response, nlu_result

    def handle_request(self, req_info, session=None, **kwargs):
        return self._handle_request(req_info, session, **kwargs)[0]

    def handle_semantic_frames(self, req_info, session=None, **kwargs):
        return self._handle_request(req_info, session, **kwargs)[1].semantic_frames

    def handle_features(self, req_info, session=None, **kwargs):
        response, nlu_result = self._handle_request(req_info, session, **kwargs)
        return (nlu_result.sample_features, response)

    def handle_callback(self, session, req_info, callback_name, form, response, **kwargs):
        logger.debug('Got callback from dm: %s', callback_name)
        if callback_name not in self._callbacks_map:
            self.on_unknown_callback(req_info=req_info, form=form, session=session,
                                     response=response, callback_name=callback_name, **kwargs)
        else:
            callback_method_obj = self._callbacks_map[callback_name]
            callback_method_obj(req_info=req_info, form=form, session=session, response=response, **kwargs)

    def on_unknown_callback(self, req_info, form, session, response, callback_name, **kwargs):
        logger.warning('Not found callback "%s". "on_unknown_callback" is called.', callback_name)

    def new_form(self, form_name, req_info):
        return self._dm.new_form(form_name, req_info=req_info)

    def change_form(self, session, form, req_info=None, sample=None, response=None, rerun_dm=True):
        session.change_intent(self._dm.get_intent(form.name))
        session.change_form(form)
        if rerun_dm:
            self._dm.handle_form(
                sample=sample,
                sample_features=None,
                session=session,
                req_info=req_info,
                response=response,
                app=self
            )

    def register_callback(self, name, func):
        self._callbacks_map[name] = func

    @property
    def dm(self):
        return self._dm

    @property
    def nlu(self):
        return self._dm.nlu

    @property
    def samples_extractor(self):
        return self._dm.samples_extractor

    def try_rename_intent(self, intent_name, key='true_intent'):
        if not intent_name or not self._intent_renames:
            return intent_name
        for pattern, renaming in self._intent_renames[key].iteritems():
            if re.match(pattern, intent_name):
                return renaming
        return intent_name

    @staticmethod
    def should_store_in_history(session):
        # we shouldn't store translate response after repeating voice via horn on the div card
        return (session.form is None or
                session.form.name is None or
                not session.form.name.startswith('personal_assistant.scenarios.translate') or
                not session.form.repeat_voice.value)
