# coding: utf-8
from __future__ import unicode_literals

import attr
import inspect
import json
import logging
import numpy as np

from copy import deepcopy, copy
from jsonschema import ValidationError, validate
from emoji import emojize
from requests.exceptions import RequestException

from vins_core.common.annotations import AnnotationsBag
from vins_core.common.sample import Sample
from vins_core.common.utterance import Utterance
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.response import FormInfoFeatures
from vins_core.nlu.features.base import SampleFeatures, IntentScore
from vins_core.nlu.anaphora.annotation_utils import extract_anaphora_annotations, rename_response_to_turn_annotations
from vins_core.dm.request_events import ImageInputEvent, MusicInputEvent, VoiceInputEvent, RequestEvent
from vins_core.dm.form_filler.events import RestorePrevFormEventHandler, CallbackEventHandler
from vins_core.dm.form_filler.models import Slot
from vins_core.dm.response import (
    ActionButton, ThemedActionButton, ClientActionDirective, ServerActionDirective, DivCard, Div2Card, UniproxyActionDirective,
    ErrorMeta, TypedSemanticFrameDirective, register_meta, get_special_button, has_special_button
)
from vins_core.utils.config import get_setting, get_bool_setting
from vins_core.utils.data import (
    load_jsonschema_from_file, load_data_from_file, validate_json, get_resource_full_path, TarArchive
)
from vins_core.utils.metrics import sensors
from vins_core.utils.misc import str_to_bool
from vins_core.config import schemas
from vins_core.ext.base import BaseHTTPAPI

from vins_sdk.app import VinsApp

from personal_assistant.callback import callback_method, bass_result_key, can_not_be_pure
from personal_assistant.meta import (
    AttentionMeta, RepeatMeta, ExternalSkillMeta, DebugInfoMeta, CancelListening,
    GeneralConversationMeta, GeneralConversationSourceMeta, SensitiveMeta
)
from personal_assistant.stroka_callbacks import StrokaCallbacksMixin
from personal_assistant.navi_callbacks import NaviCallbacksMixin
from personal_assistant.blocks import (
    AttentionBlock, AnalyticsInfoBlock, AutoactionDelayMsBlock, CardBlock, ClientFeaturesBlock, CommandBlock,
    CommitCandidateBlock, DebugInfoBlock, Div2CardBlock, ErrorBlock, FeaturesDataBlock, PlayerFeaturesBlock,
    SensitiveBlock, SilentResponseBlock, SpecialButtonBlock, StopListeningBlock, SuggestBlock, TextCardBlock,
    TypedSemanticFrameBlock, UniproxyActionBlock, UserInfoBlock, FrameActionBlock, ScenarioDataBlock,
    StackEngineBlock
)
from personal_assistant.hardcoded_responses import HardcodedResponses
from personal_assistant.bass_result import BassReportResult, BassFormInfo, BassFormInfoDeserializationError
from personal_assistant.ontology.onto import Ontology
from personal_assistant.ontology.onto_synthesis_engine import OntologicalSynthesisEngine
from personal_assistant.general_conversation import GeneralConversation
from personal_assistant.item_selection import (
    get_item_selector_for_intent, register_item_selector, TfIdfItemMatcher,
    VideoGalleryItemSelector, TVGalleryItemSelector, ContactItemSelector, EtherVideoItemSelector,
    SelectTaxiCardNumber
)
from personal_assistant import intents, clients, nlg_globals, skills, bass_session_state
from personal_assistant.api.personal_assistant import (
    PersonalAssistantAPI,
    PersonalAssistantAPIError,
    FAST_BASS_QUERY, SLOW_BASS_QUERY, HEAVY_BASS_QUERY)

from personal_assistant.autoapp_directives_translator import (
    to_autoapp_directives, is_autoapp_teach_me_intent, autoapp_add_navi_state
)

from personal_assistant.intents import parent_intent_name, INTERNAL_INTENT_SEPARATOR

logger = logging.getLogger(__name__)

GC_SKILL_NAME = 'Чат с Алисой'

LED_GIF_URL = 'https://static-alice.s3.yandex.net/led-production'

CLIENTS_CHECKS = {
    name: function
    for name, function in inspect.getmembers(clients, inspect.isfunction)
}

NLG_CHECKS = {
    name: function
    for name, function in inspect.getmembers(nlg_globals, inspect.isfunction)
}


@attr.s
class FormSetup(object):
    meta = attr.ib()
    form = attr.ib()
    precomputed_data = attr.ib()


register_meta(AttentionMeta, 'attention')
register_meta(RepeatMeta, 'repeat')
register_meta(ExternalSkillMeta, 'external_skill')
register_meta(DebugInfoMeta, 'debug_info')
register_meta(CancelListening, 'cancel_listening')
register_meta(GeneralConversationMeta, 'general_conversation')
register_meta(GeneralConversationSourceMeta, 'gc_source')
register_meta(SensitiveMeta, 'sensitive')


class PersonalAssistantErrorBlockError(Exception):
    def __init__(self, blocks):
        self.blocks = blocks


class GeneralConversationTopics(object):
    GC_DJ_TIMEOUT = 0.1

    def __init__(self, seed=None):
        topics = load_data_from_file('personal_assistant/config/general_conversation/topics.yaml')
        self._topics = [topic["topic"] for topic in topics]
        self._topic_proba = np.array([topic["popularity"] for topic in topics], dtype=float)
        self._topic_proba /= np.sum(self._topic_proba)
        self._topic_suggest_proba = float(get_setting('GC_TOPIC_SUGGEST_PROBA', 0.1))
        self._rng = np.random.RandomState(seed)
        self._http_session = BaseHTTPAPI(timeout=self.GC_DJ_TIMEOUT)

    @staticmethod
    def _json_to_topics(data):
        if not isinstance(data, dict) or 'items' not in data:
            return []
        items = data['items']
        if not isinstance(items, list):
            return []
        return [x['id'] for x in items if isinstance(x, dict) and 'id' in x and isinstance(x['id'], basestring)]

    def _recommend_topics(self, uuid):
        try:
            url = get_setting('ALICE_GC_DJ', default='http://alice.gc.dj.n.yandex-team.ru/recommender')
            r = self._http_session.get(url, params={
                'experiment': 'topics',
                'uuid': uuid
            })
            r.raise_for_status()
            return GeneralConversationTopics._json_to_topics(r.json())
        except (RequestException, ValueError) as e:
            logger.warning('Recommender service request failed: %s', str(e))
            return []

    def generate_topic(self, req_info, always_generate=False):
        if always_generate or self._rng.rand() < self._topic_suggest_proba:
            if req_info.experiments['personalized_topic'] is not None:
                topics = self._recommend_topics(req_info.uuid)
                if topics:
                    logger.debug('Topics for uuid %s: %s', req_info.uuid, '|'.join(topics))
                    return self._rng.choice(topics)
                else:
                    logger.info('Topic was not found for uuid: %s', req_info.uuid)
            return self._rng.choice(self._topics, p=self._topic_proba)
        else:
            return None


class UserInfoContainer(object):
    def __init__(self, username, is_silent):
        self._is_name_used = username is None or len(username) == 0
        self._is_silent = is_silent
        if not self._is_name_used:
            self._username = username.capitalize()
        else:
            self._username = None

    def username(self):
        self._is_name_used = True
        return self._username

    def is_used(self):
        return self._is_name_used

    def is_silent(self):
        return self._is_silent


class PersonalAssistantDialogManager(DialogManager):
    def new_form(self, intent_name, req_info=None, **kwargs):
        if intents.intent_in_irrelevant_list(intent_name, req_info):
            prev_intent_name = intent_name
            intent_name = intents.get_irrelevant_intent(intent_name, req_info)
            logger.info('Got irrelevant scenarios %s. Changing form to %s', prev_intent_name, intent_name)
            form = super(PersonalAssistantDialogManager, self).new_form(intent_name, req_info=req_info, **kwargs)
            form.intent_name.set_value(prev_intent_name, 'string')
            return form

        if intents.is_prohibited_intent(intent_name, req_info):
            prev_intent_name = intent_name
            intent_name = intents.get_prohibition_error_intent(req_info)
            logger.info('Got prohibited scenarios %s. Changing form to %s', prev_intent_name, intent_name)
        return super(PersonalAssistantDialogManager, self).new_form(intent_name, req_info=req_info, **kwargs)

    def get_suggests(self, intent_name, req_info):
        assert self.has_intent(intent_name), 'Intent name {} not found in the dialog manager'.format(intent_name)
        return self.get_intent(intent_name).suggests or []

    def get_change_form(self, intent_name):
        form = self.get_intent(intent_name).submit_form
        if form:
            form = deepcopy(form)
            form['name'] = intents.compose_name(form['name'])
        return form

    def can_fill_form(self, req_info, form, intent_name):
        # the previous condition, session.form.name == intent_name, is too strict
        # for example, it does not allow reusing forms with elliptic intents
        if intents.intent_has_irrelevant_name(form.name):
            return intents.intent_has_irrelevant_name(intent_name)
        if form.name == intents.get_prohibition_error_intent(req_info):
            return False
        return True

    def _apply_frame_to_form(self, app, response, sample, session, req_info, frame, **kwargs):
        form = super(PersonalAssistantDialogManager, self)._apply_frame_to_form(
            app, response, sample, session, req_info, frame, **kwargs
        )
        if session.form:
            form.is_ellipsis = self._is_ellipsis(form, session.form)
        return form

    @staticmethod
    def _is_ellipsis(form, previous_form):
        return (parent_intent_name(form.name) == parent_intent_name(previous_form.name) and
                INTERNAL_INTENT_SEPARATOR in form.name)


class PersonalAssistantApp(StrokaCallbacksMixin, NaviCallbacksMixin, VinsApp):
    dm_class = PersonalAssistantDialogManager

    def __init__(self, vins_file, seed=None, **kwargs):
        request_filters = {}
        for key, f in inspect.getmembers(intents, inspect.isfunction):
            if f.func_code.co_varnames[:f.func_code.co_argcount] == ('req_info',):
                request_filters[key] = f
        kwargs['request_filters_dict'] = request_filters
        kwargs['nlg_checks'] = copy(NLG_CHECKS)
        kwargs['nlg_checks']['is_active_attention("cec_screen_off")'] = ''

        super(PersonalAssistantApp, self).__init__(vins_file=vins_file, **kwargs)

        register_item_selector(VideoGalleryItemSelector())
        register_item_selector(EtherVideoItemSelector())
        register_item_selector(TVGalleryItemSelector(
            synonym_files=['personal_assistant/config/scenarios/entities/tv_channels.json'],
            cache_file='personal_assistant/data/channel_normalization.json',
            matcher=TfIdfItemMatcher(
                helper_words={'тв', 'tv', 'канал', 'channel', 'телеканал', 'телевидение', 'номер'},
                number_idf=0.5
            )
        ))
        register_item_selector(ContactItemSelector())
        register_item_selector(SelectTaxiCardNumber())

        self._pa_api = PersonalAssistantAPI()

        self._hardcoded_responses = HardcodedResponses(self.dm.samples_extractor)

        self._onto_synth_engine = OntologicalSynthesisEngine(
            Ontology.from_file('personal_assistant/config/ontology.json')
        )

        self._gc = GeneralConversation()

        self._cards_schema = load_jsonschema_from_file(
            'personal_assistant/data/schema/api/search/2/div/div-data.json',
        )

        special_buttons_cfg_path = 'personal_assistant/config/special_buttons.yaml'
        special_buttons_data = load_data_from_file(special_buttons_cfg_path)
        validate(special_buttons_data, schemas.special_buttons_schema)

        self._special_buttons_cfg = special_buttons_data['special_buttons']
        self._special_buttons_desc = dict((x['name'], x) for x in special_buttons_data['feedback_negative_buttons'])

        self.gc_topics = GeneralConversationTopics(seed=seed)

        with TarArchive(get_resource_full_path("resource://external_skills_inference/ce_updater__skill")) as archive:
            logger.info('Updating custom entity %s', archive.path)
            self.dm.update_custom_entities(archive)
            logger.info('Updating custom entity %s done', archive.path)

        self._swear_parser = self.dm.nlu._fst_parser_factory.create_parser(['swear'])
        self.dm.samples_extractor.set_allow_wizard_request(kwargs.get('allow_wizard_request'))
        self.samples_extractor.set_allow_wizard_request(kwargs.get('allow_wizard_request'))

    def handle_request(self, req_info, **kwargs):
        # TODO(autoapp): remove when autoapp's dead
        if clients.is_autoapp(req_info.app_info):
            req_info = autoapp_add_navi_state(req_info)

        skip_relevant_intent_for_session = intents.get_sessions_for_skip_relevants_intents(req_info)
        if skip_relevant_intent_for_session:
            logger.info('add skip_relevant_intent_for_session: %r', skip_relevant_intent_for_session)
            kwargs['skip_relevant_intent_for_session'] = skip_relevant_intent_for_session

        return super(PersonalAssistantApp, self).handle_request(req_info, **kwargs)

    def get_api(self):
        return self._pa_api

    @callback_method
    def on_item_selection(self, req_info, session, form, response, sample, **kwargs):
        item_selector = get_item_selector_for_intent(form.name, req_info)
        assert item_selector is not None, (
            'If there is no item selector associated with this intent, '
            'it should not have on_item_selection callback'
        )

        items = item_selector.get_items(req_info, form)
        assert items or item_selector.allows_empty_item_list, (
            'If there are no items to select from, the transition model '
            'should not have allowed for this intent'
        )
        if item_selector.allows_empty_item_list and not items:
            return

        text_to_match = item_selector.get_text_to_match(form, sample, req_info)
        if text_to_match is None:
            logger.warning(
                'Text to match is None during item selection for intent {}, utterance is {}'.format(
                    repr(form.name).decode('utf-8'),
                    repr(sample.utterance).decode('utf-8')
                )
            )
            self._abort_item_selection(sample, req_info, session, form, response)
            return

        best_score = float('-inf')
        best_item_index = None
        for item_index, item in enumerate(items):
            max_item_score = max(item_selector.matcher.get_score(text_to_match, text) for text in item.texts)
            logger.debug('Matching score with item %d is %.2f, "%s" vs "%s" ',
                         item_index, max_item_score, text_to_match, " | ".join(item.texts))
            if max_item_score > best_score:
                best_score = max_item_score
                best_item_index = item_index

        if req_info.experiments['personal_item_selector_score_threshold'] is not None:
            score_threshold = item_selector.get_score_threshold()
        else:
            score_threshold = 0.47
        if best_score > score_threshold:
            logger.debug(
                'Selected item %s (confidence=%.2f), utterance is "%s"',
                items[best_item_index],
                best_score,
                sample.utterance
            )
            item_selector.on_item_selected(
                items[best_item_index], self, session, form, req_info, sample, response, score=best_score
            )
        else:
            logger.debug(
                'Cannot select item %s (confidence=%.2f) due to low confidence, utterance is "%s"',
                items[best_item_index],
                best_score,
                sample.utterance
            )
            self._abort_item_selection(sample, req_info, session, form, response)

    def _abort_item_selection(self, sample, req_info, session, form, response):
        # Restore the previous form, and don't write in meta that the original form was item selection
        RestorePrevFormEventHandler().handle(session, self, form, req_info, response, add_overriden_meta=False)

        # Skip the current intent during the next run
        skip_intents = session.get('skip_intents') or []
        skip_intents.append(form.name)
        session.set('skip_intents', skip_intents, transient=True)

        # Rerun NLU
        self.dm.handle_with_extracted_samples(sample, req_info, session, self, response)

    @callback_method
    def bass_action(self, req_info, session, response, **kwargs):
        return self._bass_action(req_info, session, response, **kwargs)

    def _bass_action(self, req_info, session, response, sample=None, **kwargs):
        bass_action = req_info.event.payload

        balancer_type = SLOW_BASS_QUERY

        # If the submit handler has another balancer type, use it
        if session.form:
            submit_handler = next(iter(session.form.get_event_handlers('submit')), None)
            if isinstance(submit_handler, CallbackEventHandler) and submit_handler.name == 'universal_callback':
                balancer_type = getattr(submit_handler, 'balancer_type') or balancer_type
            # Bass actions in external skills require previous form. See PASKILLS-3473
            if session.form.name not in ['personal_assistant.scenarios.external_skill__continue', 'personal_assistant.scenarios.external_skill']:
                logger.debug('Reset form %s', session.form.name)
                session.form = None

        self._universal_callback_impl(
            req_info, session, response, sample=None, balancer_type=balancer_type, bass_action=bass_action, **kwargs
        )

    @callback_method
    def bass_action_with_update_form_at_first(self, req_info, session, response, **kwargs):
        payload = req_info.event.payload

        self._update_form_action(req_info, session, response, form_update=payload['data']['form'], resubmit=False, **kwargs)
        self._bass_action(req_info, session, response, **kwargs)

    @callback_method
    def on_suggest(self, response, **kwargs):
        # Do not listen after suggest logging
        response.should_listen = False

    @callback_method
    def on_card_action(self, response, **kwargs):
        # Do not listen after card action logging
        response.should_listen = False

    @callback_method
    def external_source_action(self, response, **kwargs):
        # Do not listen after external source action logging
        response.should_listen = False
        response.set_analytics_info(
            intent=None,
            form=None,
            scenario_analytics_info=None,
            product_scenario_name='placeholder',
        )

    @callback_method
    def on_special_button(self, response, **kwargs):
        # Do not listen after special button logging
        response.should_listen = False

    @callback_method
    def on_reset_session(self, req_info, session, response, mode=None, first_name=None, **kwargs):
        session.clear()

        if mode == 'onboarding' or mode == 'help':
            new_form = self.new_form(intents.ONBOARDING, req_info)
        elif mode == 'automotive.greeting':
            new_form = self.new_form(intents.AUTOMOTIVE_GREETING, req_info)
            if first_name is not None:
                new_form.first_name.set_value(first_name, 'string')
        else:
            new_form = self.new_form(intents.SESSION_START, req_info)
            # Do not listen after session reset
            if clients.is_auto(req_info.app_info):
                response.should_listen = True
            else:
                response.should_listen = False

        new_form.mode.set_value(mode, 'string')
        self.change_form(session=session, form=new_form, req_info=req_info, sample=None, response=response)

    @callback_method
    def on_get_greetings(self, req_info, session, response, **kwargs):
        session.clear()
        if req_info.experiments['no_greetings'] is not None:
            new_form = self.new_form(intents.SESSION_START, req_info)
        else:
            new_form = self.new_form(intents.ONBOARDING, req_info)
            new_form.mode.set_value('get_greetings', 'string')
        response.should_listen = False

        self.change_form(session=session, form=new_form, req_info=req_info, sample=None, response=response)

    def _parse_form_info(self, session, req_info, form_update, response):
        try:
            return BassFormInfo.from_dict(form_update)
        except BassFormInfoDeserializationError:
            logger.error('Got incorrect form info: %s', json.dumps(form_update, ensure_ascii=False))
            response.add_meta(ErrorMeta(error_type='incorrect_form_update'))
            if self.should_render_nlg_in_hollywood(req_info):
                response.add_nlg_render_bass_block({'type': 'text_card', 'phrase_id': 'error'})
            else:
                render_result = self.render_phrase(phrase_id='error', form=session.form, req_info=req_info)
                self.track_nlg_render_history_record(req_info, response, phrase_id='error', form=session.form)
                response.say(render_result.voice, render_result.text)
            return None

    @callback_method
    def update_form(self, req_info, session, response, **kwargs):
        return self._update_form_action(req_info, session, response, **kwargs)

    def _update_form_action(
        self, req_info, session, response, form_update=None, prepend_response=None, resubmit=False, **kwargs
    ):
        if form_update is None:
            # TODO: would be good to return 400 BAD REQUEST in this case
            return

        # A custom response can be added in this callback
        if prepend_response is not None:
            text = prepend_response.get('text')
            voice = prepend_response.get('voice')
            if text is not None or voice is not None:
                response.say(voice, card_text=text)

        # Try to parse form
        form_update_info = self._parse_form_info(session, req_info, form_update, response)
        if not form_update_info:
            return

        # Now the form can be updated
        form, should_resubmit_form = self._update_form(session.form, req_info, form_update_info)
        self.change_form(
            session=session, form=form, req_info=req_info, sample=None, response=response,
            rerun_dm=should_resubmit_form or resubmit
        )

    def post_run_method(self, method_name, req_info, session, response, form_update, callback):
        event = RequestEvent.from_dict(callback['event'])
        original_req_info = req_info.replace(event=event, utterance=event.utterance)
        sample = None
        if 'sample' in callback:
            sample = Sample.from_dict(callback['sample'])
        elif event.utterance:
            sample = Sample.from_utterance(event.utterance)
        form_update_info = self._parse_form_info(session, original_req_info, form_update, response)
        logger.info('Received %s request(%s) for intent %s.', method_name, req_info.request_id, form_update_info.name)
        form, _ = self._update_form(session.form, original_req_info, form_update_info, drop_source_text=False)
        self.change_form(
            session=session, form=form, req_info=original_req_info, sample=sample, response=response, rerun_dm=False
        )
        callback_kwargs = dict(
            session=session,
            form=form,
            response=response,
            req_info=original_req_info,
            sample=sample,
        )
        callback_kwargs.update(callback['arguments'])
        logger.info('Restoring %s callback with arguments %s.', callback['name'], callback['arguments'])
        self._callbacks_map[callback['name']](**callback_kwargs)

    @callback_method
    def continue_request(self, req_info, session, response, form_update, callback, **kwargs):
        self.post_run_method('continue', req_info, session, response, form_update, callback)

    @can_not_be_pure
    @callback_method
    def apply_request(self, req_info, session, response, form_update, callback, **kwargs):
        # Ensure we always apply callback with end_of_utterance=True, because the partial without eou could win.
        callback['event']['end_of_utterance'] = True
        self.post_run_method('apply', req_info, session, response, form_update, callback)

    @callback_method
    def commit_request(self, req_info, session, response, **kwargs):
        balancer_type = SLOW_BASS_QUERY
        request = kwargs['request']
        bass_response = self._pa_api.proxy_commit(req_info, request, balancer_type=balancer_type)
        response.set_commit_response(bass_response)

    @callback_method
    def hardcoded_response(self, req_info, session, response, sample, **kwargs):
        resp = self._hardcoded_responses.get(sample)

        if resp is None:
            logger.warning('Unexpected empty hardcoded response for utterance "%s"', req_info.utterance.text)

            # Switch to dummy response in this case
            new_form = self.new_form(intents.GENERAL_CONVERSATION_DUMMY, req_info)
            self.change_form(session=session, form=new_form, req_info=req_info, sample=sample, response=response)
            return

        text, voice, links = resp

        buttons = [
            ActionButton(
                title=link['title'],
                directives=[
                    ClientActionDirective(
                        name='open_uri',
                        sub_name='harcoded_response_open_uri',
                        payload={'uri': link['url']}
                    )
                ]
            ) for link in links
        ]

        response.say(voice_text=voice, card_text=text, buttons=buttons)

    @callback_method
    def nlg_callback(self, req_info, session, response,
                     phrase_id=None, form=None, question=False, append=True, **kwargs):
        logger.debug('Running custom NLG handler with phrase_id=%s', phrase_id)

        bass_resp = kwargs.get(bass_result_key)
        context = self._make_nlg_context(bass_resp.blocks) if bass_resp is not None else self._make_nlg_context()
        if bass_resp is not None:
            for block in bass_resp.blocks:
                if not isinstance(block, FeaturesDataBlock):
                    continue
                if block.data and block.data.get('answers_expected_request') is not None:
                    form_info_features = response.features.get(FormInfoFeatures.FEATURE_TYPE)
                    form_info_features.answers_expected_request = block.data.get('answers_expected_request')
                    response.set_feature(form_info_features)

            suggests, card_buttons = self._render_buttons(req_info, response, session, bass_resp.blocks, context, set())
            response.suggests += suggests

            directives = self._make_directives(req_info, bass_resp.blocks)
            response.directives += directives
            # TODO(autoapp): remove should_render_result and directives translation when autoapp's dead
            if clients.is_autoapp(req_info.app_info):
                new_intent, directives = to_autoapp_directives(directives, form.name, req_info)
                if new_intent is None:
                    response.directives = directives

        if self.should_render_nlg_in_hollywood(req_info):
            response.should_listen = question
            response.add_nlg_render_bass_block({'type': 'text_card', 'phrase_id': phrase_id})
        else:
            self._nlg_callback_impl(
                req_info, response, phrase_id, form, context=context, question=question, append=append
            )

    @callback_method
    def restore_prev_response(self, req_info, session, form, response, **kwargs):
        # Restore prev form and intent
        RestorePrevFormEventHandler(nothing_restored_phrase_id='nothing_restored').handle(
            session, self, form, req_info, response, **kwargs
        )

        # Restore prev response (voice, text, cards etc.)
        last_turn = session.dialog_history.last()
        prev_response = last_turn and last_turn.response
        if prev_response is not None:
            response.__dict__.update(prev_response.__dict__)
            response.add_meta(RepeatMeta())
        elif last_turn is not None:
            response.voice_text = last_turn.voice_text
            response.add_meta(RepeatMeta())

    @callback_method
    def general_conversation(self, req_info, session, response, sample, form, **kwargs):
        if req_info.experiments['mm_gc_protocol_disable'] is None and not session.get('pure_general_conversation'):
            gc_response, suggests = None, []
        else:
            gc_response, suggests = self._gc.get_response_with_suggests(req_info, session, sample)

        gc_phrase, source, action = None, None, None
        if gc_response is not None:
            gc_phrase = gc_response.text
            source = gc_response.source
            action = gc_response.action

        additional_nlg_context = {'gc': gc_phrase}
        if source is not None:
            response.add_meta(GeneralConversationSourceMeta(source=source))

        if source == 'proactivity' and action is not None:
            response.add_megamind_action(
                action,
                nlu_frame='alice.general_conversation.proactivity_agree')

            megamind_action_suggest_block = SuggestBlock(type='suggest', suggest_type='gc_suggest', data=u'Давай')
            megamind_action_suggest_button, _ = self._gen_button(
                form, megamind_action_suggest_block,
                context={megamind_action_suggest_block.suggest_type: megamind_action_suggest_block},
                action='general_conversation',
                req_info=req_info,
                response=response,
            )
            response.suggests.append(megamind_action_suggest_button)

        for suggest in suggests:
            suggest_block = SuggestBlock(
                type='suggest',
                suggest_type='gc_suggest',
                data=suggest,
            )
            button, _ = self._gen_button(
                form, suggest_block,
                context={suggest_block.suggest_type: suggest_block},
                action='general_conversation',
                req_info=req_info,
                response=response,
            )
            response.suggests.append(button)

        self._universal_callback_impl(
            req_info, session, response, sample, balancer_type=FAST_BASS_QUERY,
            additional_nlg_context=additional_nlg_context,
            **kwargs
        )

        # experiment DIALOG-1544
        if req_info.experiments['gc_search_fallback'] is not None:
            response.suggests.sort(
                key=lambda x: x.title.startswith(emojize(':mag: ', use_aliases=True)),
                reverse=True
            )
        # we need to make the gc_exit suggest first
        response.suggests.sort(
            key=lambda x: x.title.startswith('Хватит болтать'),
            reverse=True
        )

    @callback_method
    def pure_general_conversation_callback(self, req_info, session, response, sample, bass_result=None,
                                           analytics_info=None, **kwargs):
        intent_name = session.form.name
        context = None
        cards = []
        if intents.is_gc_start(intent_name):
            if req_info.experiments['mm_gc_protocol_disable'] is not None:
                session.set('pure_general_conversation', True, persistent=False)
            suggest_topic = req_info.experiments['gc_skill_suggest_topic'] is not None
            cards.append(TextCardBlock(type='simple_text', phrase_id='disclaimer', data={}))
            if suggest_topic:
                topic = self.gc_topics.generate_topic(req_info, always_generate=req_info.experiments['gc_always_suggest_topic'] is not None)
                if topic:
                    card_data = {
                        'suggest_topic': topic
                    }
                    cards.append(TextCardBlock(type='simple_text', phrase_id='topic_suggestion', data=card_data))
        elif intents.is_gc_end(intent_name):
            session.set('pure_general_conversation', False, persistent=False)
            response.add_meta(ExternalSkillMeta(skill_name=GC_SKILL_NAME, deactivating=True))
            if clients.is_auto(req_info.app_info):
                # don't listen on gc exit: see st.yandex-team.ru/DIALOG-3803
                response.should_listen = False
        else:
            logger.warning('Intent name {} is unexpected for callback "pure_general_conversation"'.format(intent_name))

        bass_result = bass_result or BassReportResult.empty()
        bass_result.blocks += cards
        self._handle_response(
            bass_result, response=response, session=session, req_info=req_info, sample=sample, sample_features=None,
            additional_nlg_context=context, analytics_info=analytics_info
        )

    @callback_method
    def microintents_callback(self, req_info, session, response, sample, **kwargs):
        if not session.has('used_replies') or session.get('used_replies') is None:
            session.set('used_replies', [], persistent=False)
        context = {'used_replies': session.get('used_replies')}
        intent = self.dm.get_intent(session.intent_name)
        logger.info("Add context")
        logger.info(context)

        if intent.show_led_gif is not None and clients.has_led_display(req_info):
            response.directives += [
                self._make_force_display_cards_directive(),
                self._make_led_screen_directive(intent.show_led_gif),
            ]

        force_gc_microintent = (intent.gc_microintent
                                and req_info.experiments['vins_disable_gc_microintents'] is not None)
        force_swear_microintent = (intent.gc_swear_microintent
                                   and req_info.experiments['vins_disable_gc_swear_microintents'] is not None)
        force_gc_fallback = (intent.gc_fallback and req_info.experiments['no_force_gc_fallback'] is None
                             and session.get('pure_general_conversation')) or force_gc_microintent or force_swear_microintent
        if not force_gc_fallback:
            self._universal_callback_impl(req_info, session, response, sample,
                                          additional_nlg_context=context, balancer_type=FAST_BASS_QUERY, **kwargs)
        if (force_gc_microintent or force_swear_microintent or intent.gc_fallback) and not response.voice_text:
            form = self.new_form(intents.GENERAL_CONVERSATION, req_info)
            logger.debug('Changing form because of GC fallback')
            self.change_form(session, form, req_info, sample, response, rerun_dm=True)
            return
        form_cfg = self.dm.get_change_form(session.intent_name)
        if form_cfg:
            form = self.new_form(form_cfg['name'], req_info)
            for slot_name, slot in form_cfg['slots'].iteritems():
                form.get_slot_by_name(slot_name).set_value(slot['value'], slot['type'])

            # Remove previous suggests
            # TODO: to support previous suggests you should somehow remove
            # TODO: onboarding__what_can_you_do and search_internet_fallback
            response.suggests = []
            self.change_form(session, form, req_info, sample, response, rerun_dm=True)

    @callback_method
    def set_expects_request(self, session, form, response, **kwargs):
        form_info_features = response.features.get(FormInfoFeatures.FEATURE_TYPE)
        form_info_features.expects_request = True
        response.set_feature(form_info_features)

    def setup_forms(self, forms, req_info, session, balancer_type=FAST_BASS_QUERY, sample=None):
        """
            NOTE(the0): Watch out! This method may throw PersonalAssistantAPIError
                        on the self._pa_api.setup_forms(...) call.
        """
        bass_state = bass_session_state.get_bass_session_state(session)
        bass_setup_result = self._pa_api.setup_forms(
            req_info=req_info,
            session=session,
            forms=forms,
            balancer_type=balancer_type,
            bass_session_state=bass_state,
            is_banned=self._is_text_banned(sample) if sample else None
        )
        assert len(forms) == len(bass_setup_result.forms)

        setup_result = []
        for form, bass_form_setup in zip(forms, bass_setup_result.forms):
            updated_form, _ = self._update_form(form, req_info, bass_form_setup.info)
            setup_result.append(FormSetup(
                meta=bass_form_setup.meta,
                form=updated_form,
                precomputed_data=bass_form_setup.precomputed_data
            ))

        return setup_result

    @callback_method
    def universal_callback(self, req_info, session, response, sample,
                           balancer_type=FAST_BASS_QUERY,
                           bass_result=None,
                           sample_features=None,
                           analytics_info=None,
                           **kwargs):
        # TODO(autoapp): remove when autoapp's dead
        # BASS will cause change_form for certain scenarios and it's hard to intercept after this.
        if clients.is_autoapp(req_info.app_info) and is_autoapp_teach_me_intent(session.form):
            new_form = self.new_form(intents.TEACH_ME_INTENT, req_info)
            self.change_form(session=session, form=new_form, req_info=req_info, sample=None, response=response)
            return

        if not bass_result:
            self._universal_callback_impl(req_info, session, response, sample, sample_features=sample_features,
                                          balancer_type=balancer_type, analytics_info=analytics_info, **kwargs)
        else:
            self._handle_response(
                bass_result, response=response, session=session, req_info=req_info, sample=sample,
                sample_features=sample_features, additional_nlg_context=None, analytics_info=analytics_info
            )

    @callback_method
    def universal_callback_no_bass(self, req_info, session, response, sample,
                                   bass_result=None, analytics_info=None, **kwargs):
        bass_result = bass_result or BassReportResult.empty()

        self._handle_response(
            bass_result, response=response, session=session, req_info=req_info, sample=sample, sample_features=None,
            additional_nlg_context=None, analytics_info=analytics_info
        )

    @callback_method
    def external_skill_activate_only(self, session, form, response, req_info, bass_result=None, **kwargs):
        if bass_result is None:
            self._deactivate_skill()

        # save session with new dialog_id
        new_chat_session = deepcopy(session)
        new_bass_resp = deepcopy(bass_result)
        new_bass_resp.form_info.name = intents.EXTERNAL_SKILL_ACTIVATE
        new_bass_resp.blocks = filter(
            lambda x: not isinstance(x, CommandBlock), new_bass_resp.blocks
        )
        new_form, _ = self._update_form(new_chat_session.form, req_info, new_bass_resp.form_info)
        dialog_id = new_form.skill_id.value
        new_req = req_info.replace(dialog_id=dialog_id)
        new_chat_session.set('bass_result_for_chat', new_bass_resp.to_dict())
        self.change_form(session=new_chat_session, form=new_form, rerun_dm=False)
        self.save_session(new_chat_session, new_req, response)

        # create response directives
        skill_info = form.skill_info.value or {}
        bot_guid = skill_info.get('bot_guid')
        if bot_guid:
            if 'open_bot_url' in skill_info:
                open_bot_action_name = 'open_uri'
                open_bot_payload = {'uri': skill_info['open_bot_url']}
            else:
                open_bot_action_name = 'open_bot'
                open_bot_payload = {'bot_id': bot_guid}
                if form.request.value:
                    open_bot_payload['text'] = form.request.value

            open_action = ClientActionDirective(
                name=open_bot_action_name,
                sub_name='external_skill_activate_bot_' + open_bot_action_name,
                payload=open_bot_payload,
            )
        else:
            subdirectives = [
                ClientActionDirective(
                    name='end_dialog_session',
                    sub_name='external_skill_activate_end_dialog_session',
                    payload={
                        'dialog_id': dialog_id,
                    }
                )
            ]

            subdirectives += self._make_directives(req_info, bass_result.blocks)

            if form.request.value:
                subdirectives.append(
                    ClientActionDirective(
                        name='type_silent',
                        sub_name='external_skill_activate_type_silent',
                        payload={'text': form.request.value},
                    )
                )

            subdirectives.append(
                ServerActionDirective(
                    name='new_dialog_session',
                    payload={
                        'dialog_id': dialog_id,
                        'request': form.request.value,
                    }
                )
            )

            open_action = ClientActionDirective(
                name='open_dialog',
                sub_name='external_skill_activate_open_dialog',
                payload={
                    'dialog_id': dialog_id,
                    'directives': [d.to_dict() for d in subdirectives],
                })

        response.directives.append(open_action)

        # render response for Alice tab
        context = self._make_nlg_context()
        suggest = SuggestBlock(
            type='suggest',
            suggest_type='open_dialog_action',
            data=None
        )
        button, _ = self._gen_button(
            form, suggest,
            context=context,
            req_info=req_info,
            response=response,
            action='external_skill',
            directives=[open_action],
        )

        render_result = self.render_phrase(
            'render_result',
            form=form,
            context=context,
            req_info=req_info
        )
        self.track_nlg_render_history_record(req_info, response, phrase_id='render_result', form=form, context=context)
        response.say(
            render_result.voice, render_result.text,
            buttons=[button],
        )

    @callback_method
    def new_dialog_session(self, req_info, form, session, response,
                           dialog_id=None, request=None, analytics_info=None, **kwargs):
        bass_result = session.pop('bass_result_for_chat', None)
        logger.debug('Bass result for chat loaded: %s', bass_result)
        if bass_result is not None:
            bass_result = BassReportResult.from_dict(bass_result)

        if bass_result is None:
            new_form = self.dm.new_form(
                intents.EXTERNAL_SKILL_ACTIVATE,
                response=response,
                req_info=req_info,
                app=self,
            )
            new_form.skill_id.set_value(dialog_id, 'skill')
            if request is not None:
                new_form.request.set_value(request, 'string')

            self.change_form(
                req_info=req_info,
                form=new_form,
                session=session,
                response=response,
            )
        else:
            self._handle_response(
                bass_result, response=response, session=session, req_info=req_info, sample=None, sample_features=None,
                additional_nlg_context=None, analytics_info=analytics_info
            )

    @callback_method
    def external_skill(self, req_info, form, session, response, **kwargs):
        sample = kwargs.get('sample')
        if req_info.dialog_id is not None:
            form.skill_id.set_value(req_info.dialog_id, 'skill')
            if sample:
                form.request.set_value(sample.text, 'string')
        elif 'utterance_normalized' in form and sample:
            form.skill_id.set_value(None, 'skill')
            form.utterance_normalized.set_value(sample.text, 'string')
        elif form.skill_id.value is None:
            phrase = self.render_phrase(
                phrase_id='unknown_skill_id',
                form=form,
                req_info=req_info
            )
            self.track_nlg_render_history_record(req_info, response, phrase_id='unknown_skill_id', form=form)
            response.say(phrase.voice, phrase.text)
            response.add_meta(ErrorMeta(error_type='unknown_skill'))
            self._deactivate_skill(req_info, form, response, session)
            return

        if isinstance(req_info.event, (ImageInputEvent, MusicInputEvent)):
            self._deactivate_skill(req_info, form, response, session)

            intent_name = (
                intents.IMAGE_RECOGNIZER_INTENT if isinstance(req_info.event, ImageInputEvent)
                else intents.MUSIC_RECOGNIZER_INTENT
            )

            self.change_form(
                req_info=req_info,
                form=self.dm.new_form(
                    intent_name,
                    response=response,
                    req_info=req_info,
                    app=self,
                    previous_form=form,
                ),
                session=session,
                response=response,
            )
            return

        # Supply additional markup from our engine
        if req_info.utterance:
            form.vins_markup.set_value(
                self._get_vins_markup_for_external_skill(Sample.from_utterance(req_info.utterance)),
                'vins_markup'
            )

        def get_skill_id(session):
            return session.form.skill_id.value if 'skill_id' in session.form else None

        try:
            self._universal_callback_impl(
                req_info, session, response,
                form=form,
                balancer_type=HEAVY_BASS_QUERY,
                raise_error=True,
                **kwargs
            )
            if 'skill_info' in session.form:
                skill_name = session.form.skill_info.value.get('name') if session.form.skill_info.value else None
                response.add_meta(ExternalSkillMeta(skill_name=skill_name))
            response.add_meta(DebugInfoMeta(data=get_skill_id(session)))
        except PersonalAssistantAPIError:
            skill_id = get_skill_id(session)
            logger.error('Got error from BASS. Turn off active skill %s.', skill_id)
            self._deactivate_skill(req_info, form, response, session, with_error_message=True)
        except PersonalAssistantErrorBlockError as exc:
            if exc.blocks[0].error_type == 'external_skill_unknown':
                phrase = self.render_phrase(
                    phrase_id='unknown_skill_id',
                    form=form,
                    req_info=req_info
                )
                self.track_nlg_render_history_record(req_info, response, phrase_id='unknown_skill_id', form=form)
                response.say(phrase.voice, phrase.text)
                response.add_meta(ErrorMeta(error_type='unknown_skill'))
            else:
                skill_id = get_skill_id(session)
                logger.error(
                    'Got error blocks %s from BASS. Deactivate skill %s',
                    exc.blocks, skill_id,
                )
                response.add_meta(DebugInfoMeta(data=skill_id))
            self._deactivate_skill(req_info, form, response, session)

    def _deactivate_skill(self, req_info, form, response, session, with_error_message=False):
        new_form = self.dm.new_form(
            intents.EXTERNAL_SKILL_DEACTIVATE,
            response=response,
            req_info=req_info,
            app=self,
            previous_form=form,
        )

        if with_error_message:
            new_form.response.set_value(
                {'text': self.render_phrase(phrase_id='error', form=session.form, req_info=req_info).text},
                'response'
            )
            self.track_nlg_render_history_record(req_info, response, phrase_id='error', form=session.form)
        else:
            new_form.silent.set_value(True, 'boolean')

        self.change_form(
            req_info=req_info,
            form=new_form,
            session=session,
            response=response,
        )

    @callback_method
    def external_skill_deactivate(self, req_info, form, session, response, **kwargs):
        if req_info.experiments['close_external_skill_on_deactivate'] is not None:
            self.universal_callback(
                req_info=req_info, form=form, session=session, response=response, balancer_type=HEAVY_BASS_QUERY,
                **kwargs
            )
        else:
            self.universal_callback_no_bass(req_info=req_info, form=form, session=session, response=response, **kwargs)

            if req_info.dialog_id is not None:
                response.directives.append(
                    ClientActionDirective(
                        name='end_dialog_session',
                        sub_name='external_skill_deactivate_end_dialog_session',
                        payload={'dialog_id': req_info.dialog_id},
                    )
                )

        response.add_meta(ExternalSkillMeta(deactivating=True))

    @callback_method
    def on_external_button(self, req_info, form, response, session, sample=None, button_data=None, **kwargs):
        if form is None or not (intents.is_skill_continue(form.name) or intents.is_skill_activate(form.name)):
            logger.warning("'on_external_button' callback can't handle %s form", form.name if form else 'missing')
            response.add_meta(ErrorMeta(error_type='incorrect_state_on_external_button'))

            render_result = self.render_phrase(
                phrase_id='external_skill_deactivated',
                form=session.form,
                req_info=req_info
            )
            self.track_nlg_render_history_record(req_info, response, phrase_id='external_skill_deactivated', form=session.form)
            response.say(render_result.voice, render_result.text)
            return

        form.request.set_value({'payload': button_data}, 'button')
        self._universal_callback_impl(
            req_info, session, response,
            sample=None,
            balancer_type=HEAVY_BASS_QUERY,
            form=form,
            **kwargs
        )

    @callback_method
    def extract_entities_from_response(self, session, entity_source_fields=None, **kwargs):
        form = session.form

        response_text = self._extract_response_text(form, entity_source_fields)

        if not response_text:
            return

        response_sample = extract_anaphora_annotations(Sample.from_string(response_text), self.samples_extractor,
                                                       is_response=True)

        if not session.annotations:
            session.annotations = AnnotationsBag()

        rename_response_to_turn_annotations(response_sample.annotations)

        session.annotations.update(response_sample.annotations)

    @staticmethod
    def _extract_response_text(form, fields):
        for field in fields:
            current_slot = field['slot']
            if current_slot not in form:
                continue

            value = form.get_slot_by_name(current_slot).value
            if not value:
                continue
            if 'path_to_field_with_text' in field:
                path_to_field = field['path_to_field_with_text'].split('.')
            else:
                path_to_field = []
            for field_name in path_to_field:
                value = value.get(field_name)
                if not value:
                    break
            if value and isinstance(value, basestring):
                return value

        return None

    def _universal_callback_impl(
        self, req_info, session, response, sample, balancer_type,
            bass_action=None,
            additional_nlg_context=None,
            raise_error=False,
            precomputed_data=None,
            sample_features=None,
            analytics_info=None,
            **kwargs
    ):
        try:
            bass_state = bass_session_state.get_bass_session_state(session)
            bass_result = self._pa_api.submit_form(
                req_info, session=session, form=session.form, action=bass_action,
                balancer_type=balancer_type, bass_session_state=bass_state,
                precomputed_data=precomputed_data, is_banned=self._is_text_banned(sample) if sample else None
            )
            if session and session.form and session.form.name \
                    and bass_result and bass_result.form_info and bass_result.form_info.name \
                    and session.form.name != bass_result.form_info.name:
                data = {
                    'type': 'changed_form',
                    'data': {'before': session.form.name, 'after': bass_result.form_info.name}
                }
                response.add_meta(DebugInfoMeta(data=data))
        except PersonalAssistantAPIError:
            logger.warning('Got PersonalAssistantAPIError', exc_info=True)
            response.add_meta(ErrorMeta(error_type='bass_error'))

            if raise_error:
                raise

            if self.should_render_nlg_in_hollywood(req_info):
                response.add_nlg_render_bass_block({'type': 'text_card', 'phrase_id': 'error'})
            else:
                render_result = self.render_phrase(phrase_id='error', form=session.form, req_info=req_info)
                self.track_nlg_render_history_record(req_info, response, phrase_id='error', form=session.form)
                response.say(render_result.voice, render_result.text)
        else:
            self._handle_response(
                bass_result, response=response, session=session, req_info=req_info, sample=sample,
                sample_features=sample_features, additional_nlg_context=additional_nlg_context,
                raise_error=raise_error, analytics_info=analytics_info
            )

    def _gen_button(self, form, suggest_block, context, req_info, response, action, directives=None):
        intent_name = form and form.name
        is_external = suggest_block.suggest_type == 'external_skill'

        render_caption_phrase_id = '__'.join(['render_suggest_caption', suggest_block.suggest_type])
        render_asr_caption_phrase_id = '__'.join(['render_suggest_asr_caption', suggest_block.suggest_type])
        render_utterance_phrase_id = '__'.join(['render_suggest_utterance', suggest_block.suggest_type])
        render_user_utterance_phrase_id = '__'.join(['render_suggest_user_utterance', suggest_block.suggest_type])
        render_user_asr_utterance_phrase_id = '__'.join(['render_suggest_user_asr_utterance', suggest_block.suggest_type])
        render_uri_phrase_id = '__'.join(['render_suggest_uri', suggest_block.suggest_type])

        has_caption = self.has_phrase(phrase_id=render_caption_phrase_id, intent_name=intent_name)
        has_asr_caption = self.has_phrase(phrase_id=render_asr_caption_phrase_id, intent_name=intent_name)
        has_utterance = self.has_phrase(phrase_id=render_utterance_phrase_id, intent_name=intent_name)
        has_user_utterance = self.has_phrase(phrase_id=render_user_utterance_phrase_id, intent_name=intent_name)
        has_user_asr_utterance = self.has_phrase(phrase_id=render_user_asr_utterance_phrase_id, intent_name=intent_name)
        has_uri = self.has_phrase(phrase_id=render_uri_phrase_id, intent_name=intent_name)
        has_commands = suggest_block.data and 'commands' in suggest_block.data

        if req_info.event.event_type == 'voice_input':
            if has_user_asr_utterance and has_user_utterance:
                render_user_utterance_phrase_id = render_user_asr_utterance_phrase_id

            if has_asr_caption and has_caption:
                render_caption_phrase_id = render_asr_caption_phrase_id

        if req_info.experiments['themed_search_internet_fallback_suggest'] is not None and \
                suggest_block.suggest_type == 'search_internet_fallback':
            render_caption_phrase_id = '__'.join(['render_suggest_caption_with_image', suggest_block.suggest_type])

        if not has_caption:
            logger.warning(
                "Don't know how to render caption of suggest %s for intent %s",
                suggest_block.suggest_type, intent_name,
            )
            return None, None

        caption = self.render_phrase(
            phrase_id=render_caption_phrase_id, form=form, context=context, req_info=req_info
        ).text
        self.track_nlg_render_history_record(req_info, response, phrase_id=render_caption_phrase_id, form=form, context=context)

        directives = copy(directives) or []
        on_suggest_payload = {
            'suggest_block': suggest_block.to_dict(),
            'caption': caption,
            'request_id': req_info.request_id,
        }

        if has_user_utterance:
            user_utterance = self.render_phrase(
                phrase_id=render_user_utterance_phrase_id,
                form=form,
                context=context,
                req_info=req_info
            ).text
            self.track_nlg_render_history_record(req_info, response, phrase_id=render_user_utterance_phrase_id, form=form, context=context)
            if user_utterance:
                directives.append(
                    ClientActionDirective(
                        name='type_silent',
                        sub_name=action + '_type_silent',
                        payload={'text': user_utterance}
                    )
                )
                on_suggest_payload['user_utterance'] = user_utterance

        if has_uri:
            uri = self.render_phrase(phrase_id=render_uri_phrase_id, form=form, context=context, req_info=req_info).text
            self.track_nlg_render_history_record(req_info, response, phrase_id=render_uri_phrase_id, form=form, context=context)
            if uri:
                directives.append(
                    ClientActionDirective(
                        name='open_uri',
                        sub_name=action + '_open_uri',
                        payload={'uri': uri}
                    )
                )
                on_suggest_payload['uri'] = uri

        if has_utterance:
            utterance = self.render_phrase(
                phrase_id=render_utterance_phrase_id, form=form, context=context, req_info=req_info
            ).text
            self.track_nlg_render_history_record(req_info, response, phrase_id=render_utterance_phrase_id, form=form, context=context)
            if utterance:
                directives.append(
                    ClientActionDirective(
                        name='type',
                        sub_name=action + '_type',
                        payload={'text': utterance}
                    )
                )
                on_suggest_payload['utterance'] = utterance

        if has_commands:
            for command in suggest_block.data['commands']:
                directives.append(
                    ClientActionDirective(
                        name=command['command_type'],
                        sub_name=command.get('command_sub_type'),
                        payload=command['data'],
                    )
                )

        if suggest_block.form_update is not None:
            if has_uri or has_utterance:
                logger.warning('It is unusual to have both form update and utterance/uri associated with a button')

            form_update = suggest_block.form_update.copy()
            form_resubmit = form_update.pop('resubmit', False)

            directives.append(
                ServerActionDirective(
                    name='update_form',
                    payload={'form_update': form_update, 'resubmit': form_resubmit},
                )
            )

        if is_external and suggest_block.data.get('payload'):
            directives.append(
                ServerActionDirective(
                    name='on_external_button',
                    payload={
                        'button_data': suggest_block.data['payload'],
                        'request_id': req_info.request_id,
                    }
                )
            )
        else:
            # For logging all suggests and actions on our side
            directives.append(ServerActionDirective(
                name='on_suggest', payload=on_suggest_payload, ignore_answer=True))

        if is_external:
            is_suggest = suggest_block.data.get('hide', True)
        else:
            if suggest_block.data and 'attach_to_card' in suggest_block.data:
                is_suggest = not suggest_block.data['attach_to_card']
            else:
                # For now render suggests with associated urls or commands as actions
                is_suggest = not (has_uri or has_commands)

        if is_suggest and suggest_block.data and 'theme' in suggest_block.data:
            return ThemedActionButton(title=caption, theme=suggest_block.data['theme'], directives=directives), is_suggest
        else:
            return ActionButton(title=caption, directives=directives), is_suggest

    def _render_error_block(self, form, block, context, req_info, response, should_render_nlg_in_hollywood_flag=False):
        phrase_id = 'render_error__' + str(block.error_type)

        if form is None:
            logger.warning("Don't know how to render ErrorBlock's phrase %s for empty form", phrase_id)
        elif not self.has_phrase(phrase_id=phrase_id, intent_name=form.name):
            logger.warning("Don't know how to render ErrorBlock's phrase %s for intent %s", phrase_id, form.name)
        else:
            if should_render_nlg_in_hollywood_flag:
                response.add_nlg_render_bass_block(block.to_dict())
            else:
                render_result = self.render_phrase(phrase_id, form=form, context=context, req_info=req_info)
                self.track_nlg_render_history_record(req_info, response, phrase_id=phrase_id, form=form, context=context)
                response.say(render_result.voice, render_result.text, append=True)

    @staticmethod
    def should_render_nlg_in_hollywood(req_info):
        return str_to_bool(req_info.experiments['hw_render_vins_nlg'])

    @staticmethod
    def should_pass_through_bass_response(req_info):
        return str_to_bool(req_info.experiments['pass_through_bass_response_to_hw'])

    @staticmethod
    def _contains_start_recognition_action(blocks):
        return any((
            isinstance(block, CommandBlock) and
            block.command_type in ('start_music_recognizer', 'start_image_recognizer')
            for block in blocks
        ))

    @classmethod
    def _should_add_feedback(cls, intent_name, req_info, session, blocks):
        if (
            clients.is_smart_speaker(req_info.app_info) or
            clients.is_tv_device(req_info.app_info) or
            clients.is_navigator(req_info.app_info) or
            clients.is_auto(req_info.app_info) or
            clients.is_legatus(req_info.app_info)
        ):
            return False

        if intents.is_session_start(intent_name):
            # No feedback for session start
            return False

        if intents.is_feedback(intent_name):
            # No feedback when providing feedback
            return False

        if PersonalAssistantApp._contains_start_recognition_action(blocks):
            # Non feedback when starting recognition
            return False

        if intents.is_skill(intent_name) and not intents.is_skill_deactivate(intent_name):
            return False

        if intents.is_market_intent(intent_name) and not intents.is_market_intent_finished(intent_name):
            return False

        if (
            intents.is_stateful_scenario(intent_name) and
            not intents.is_open_stateful_transition(session.form, intent_name)
        ):
            return False

        if session.get('pure_general_conversation'):
            return False

        if intents.is_search(intent_name) or intents.is_direct_gallery(intent_name):
            return False

        return True

    @classmethod
    def _should_use_special_buttons(cls, req_info, client_features):
        return ('builtin_feedback' in client_features) and (req_info.experiments['builtin_feedback'] is not None)

    @staticmethod
    def _make_feedback_suggest_block(response_intent_name, is_positive):
        if is_positive:
            suggest_type = 'feedback__positive'
            intent_name = intents.FEEDBACK_POSITIVE
        else:
            suggest_type = 'feedback__negative'
            intent_name = intents.get_negative_feedback_intent(response_intent_name)

        return SuggestBlock(
            type='suggest',
            suggest_type=suggest_type,
            data=None,
            form_update={
                'name': intent_name
            }
        )

    @classmethod
    def _make_gc_feedback_suggest_blocks(cls):
        return [
            SuggestBlock(
                type='suggest',
                suggest_type='gc_feedback__positive',
                data=None,
                form_update={
                    'name': intents.GC_FEEDBACK_POSITIVE
                }
            ), SuggestBlock(
                type='suggest',
                suggest_type='gc_feedback__neutral',
                data=None,
                form_update={
                    'name': intents.GC_FEEDBACK_NEUTRAL
                }
            ), SuggestBlock(
                type='suggest',
                suggest_type='gc_feedback__negative',
                data=None,
                form_update={
                    'name': intents.GC_FEEDBACK_NEGATIVE
                }
            )
        ]

    def _modify_suggest_list(self, req_info, session, blocks, client_features):
        # Add textual suggests from the microintent config
        form = session.form
        intent_name = form and form.name
        microintent_suggests = []
        if intents.is_microintent(intent_name) and not session.get('pure_general_conversation'):
            suggests = self.dm.get_suggests(intent_name, req_info)
            for suggest in suggests:
                microintent_suggests.append(
                    SuggestBlock(type='suggest', suggest_type='from_microintent', data={'text': suggest})
                )

        # Always add feedback suggests to the response
        feedback_suggests = []
        if (self._should_add_feedback(intent_name, req_info, session, blocks) and
                not self._should_use_special_buttons(req_info, client_features)):

            if (
                intents.is_skill(intent_name) and skills.is_gc_skill(form.skill_id.value) or
                intents.is_gc_end(intent_name)
            ):
                feedback_suggests.extend(self._make_gc_feedback_suggest_blocks())
            else:
                feedback_suggests.append(self._make_feedback_suggest_block(intent_name, is_positive=True))
                feedback_suggests.append(self._make_feedback_suggest_block(intent_name, is_positive=False))

        is_search_suggest = lambda block: block.type == 'suggest' and block.suggest_type == 'search_internet_fallback'
        search_suggests = list(filter(is_search_suggest, blocks))
        should_put_search_suggest_before_feedback = req_info.experiments['search_suggest_first'] is not None
        should_put_search_suggest_after_feedback = req_info.experiments['search_suggest_second'] is not None

        # Remove search_internet_fallback when there is no utterance
        # or the request has been triggered by suggest
        if (
            not req_info.utterance or
            req_info.utterance.input_source == Utterance.SUGGESTED_INPUT_SOURCE or
            session.get('pure_general_conversation') or
            search_suggests and (should_put_search_suggest_before_feedback or
                                 should_put_search_suggest_after_feedback)
        ):
            blocks = [b for b in blocks if not is_search_suggest(b)]

        # Special suggests for quitting general conversation skill
        if session.get('pure_general_conversation'):
            exit_gc = [
                SuggestBlock(type='suggest', suggest_type='exit_general_conversation', data=None)
            ]
            if clients.is_auto_without_suggests(req_info.app_info):
                # generate a suggest for old Auto app to trigger its change in appearance
                exit_gc.append(SuggestBlock(
                    type='suggest',
                    suggest_type='external_skill_deactivate',
                    data='Этот саджест никто не должен увидеть',
                ))
        else:
            exit_gc = []

        if should_put_search_suggest_before_feedback:
            return exit_gc + search_suggests + feedback_suggests + microintent_suggests + blocks
        elif should_put_search_suggest_after_feedback:
            return exit_gc + feedback_suggests + search_suggests + microintent_suggests + blocks
        return exit_gc + feedback_suggests + microintent_suggests + blocks

    def _make_nlg_context(self, blocks=(), additional_nlg_context=None):
        context = {
            'onto': self._onto_synth_engine.ontology,
            'onto_synth': self._onto_synth_engine,
            'attention': [],
            'command': []
        }

        username = None
        is_username_silent = False
        for block in blocks:
            if isinstance(block, (ErrorBlock, TextCardBlock)):
                context[block.type] = block
            elif isinstance(block, AttentionBlock):
                context['attention'].append(block)
            elif isinstance(block, CommandBlock):
                context['command'].append(block)
            elif isinstance(block, UserInfoBlock):
                if self._is_text_swear(block.username):
                    logger.warning('Banned username: %s', block.username)
                else:
                    username = block.username
                    is_username_silent = block.is_silent
        context['userinfo'] = UserInfoContainer(username, is_username_silent)

        if additional_nlg_context:
            context.update(additional_nlg_context)

        return context

    @staticmethod
    def _track_bass_blocks_for_nlg_context(bass_blocks, response):
        for bass_block in bass_blocks:
            if isinstance(bass_block, AttentionBlock):
                response.add_nlg_render_bass_block(bass_block.to_dict())

    def _update_slots(self, form, bass_form_info, resubmit_form=False, drop_source_text=True):
        filled_slots = set()
        for slot in bass_form_info.slots:
            original_slot = form.get_slot_by_name(slot.name)
            if original_slot is None:
                logger.warning('Unknown slot %s of form %s. Skipped.', slot.name, form.name)
                continue

            if slot.optional is not None:
                original_slot.optional = slot.optional

            if original_slot.value != slot.value:
                # if bass changed some value we force source_text to be None
                source_text = None if drop_source_text else slot.source_text
                original_slot.set_value(slot.value, slot.type, source_text=source_text)
                if original_slot.value is not None:
                    original_slot.active = False
                    filled_slots.add(original_slot.name)

            if not original_slot.optional and original_slot.value is None:
                # Resubmit the form if after the update a required field has no value
                logger.debug('Resubmitting form because required slot %s has no value', original_slot.name)
                resubmit_form = True

        for slot_group in form.required_slot_groups:
            if set(slot_group.slots).intersection(filled_slots):
                for slot_from_group in slot_group.slots:
                    form.get_slot_by_name(slot_from_group).active = False

        return resubmit_form

    def _update_form(self, form, req_info, bass_form_info, drop_source_text=True):
        resubmit_form = False

        if bass_form_info is None:
            # Nothing to update
            return form, resubmit_form

        if form is not None and self._is_update_applicable(form, bass_form_info):
            # Update current form
            form = deepcopy(form)
        else:
            # Switch to a new form
            form = self.dm.new_form(bass_form_info.name, req_info, previous_form=form)
            resubmit_form = self._is_update_applicable(form, bass_form_info)

        resubmit_form = self._update_slots(form, bass_form_info, resubmit_form, drop_source_text) or resubmit_form

        return form, resubmit_form

    @staticmethod
    def _is_update_applicable(form, form_info):
        logger.debug(
            (
                '_is_update_applicable: form.name = %s, form_info.name = %s, ' +
                'form_info.set_new_form = %s, form_info.dont_resubmit = %s'
            ),
            form.name,
            form_info.name,
            form_info.set_new_form,
            form_info.dont_resubmit,
        )
        return form.name == form_info.name and not form_info.set_new_form and not form_info.dont_resubmit

    def _add_block_info_to_response_meta(self, blocks, response, form_name):
        for block in blocks:
            if isinstance(block, AttentionBlock):
                response.add_meta(AttentionMeta(attention_type=block.attention_type))
            elif isinstance(block, ErrorBlock):
                response.add_meta(ErrorMeta(error_type=block.error_type, form_name=form_name))
            elif isinstance(block, DebugInfoBlock):
                response.add_meta(DebugInfoMeta(data=block.data))
            elif isinstance(block, SensitiveBlock):
                response.add_meta(SensitiveMeta(data=block.data))

    def _make_directives(self, req_info, blocks):
        directives = []
        for block in blocks:
            if isinstance(block, CommandBlock):
                logger.debug('Handle %s command block', block)
                directives.append(
                    ClientActionDirective(name=block.command_type, sub_name=block.command_sub_type, payload=block.data)
                )
            elif isinstance(block, UniproxyActionBlock):
                logger.debug('Handle %s proxy action block', block)
                directives.append(UniproxyActionDirective(name=block.command_type, payload=block.data))
            elif isinstance(block, TypedSemanticFrameBlock):
                logger.debug('Handle %s typed semantic frame block', block)
                directives.append(TypedSemanticFrameDirective(name='@@mm_semantic_frame', payload=block.payload, analytics=block.analytics))

        return directives

    @staticmethod
    def _make_force_display_cards_directive(listening_is_possible=True):
        return ClientActionDirective(
            name='force_display_cards',
            sub_name='force_display_cards',
            payload={
                'listening_is_possible': listening_is_possible,
            },
        )

    @staticmethod
    def _make_led_draw_items(gif_names):
        items = [
            {
                'frontal_led_image': '{}/{}.gif'.format(
                    LED_GIF_URL,
                    np.random.choice(gif_name) if isinstance(gif_name, list) else gif_name,
                ),
            }
            for gif_name in gif_names
        ]
        if len(items) > 0:
            items[-1]['endless'] = True
        return items

    @staticmethod
    def _make_led_screen_directive(gif_names, listening_is_possible=True):
        return ClientActionDirective(
            name='draw_led_screen',
            sub_name='draw_led_screen',
            payload={
                'animation_sequence': PersonalAssistantApp._make_led_draw_items(gif_names),
                'listening_is_possible': listening_is_possible,
                'till_end_of_speech': True
            },
        )

    def _render_cardtemplate_nlg(self, id, data, form, context, req_info):
        if not self.has_cardtemplate(id, intent_name=form.name):
            logger.warning('Not found cardtemplate nlg template "%s"', id)
            return None

        render_context = context.copy()
        render_context['data'] = data

        body = None
        try:
            body = self.render_cardtemplate(id, form=form, context=render_context, req_info=req_info)
        except Exception as e:
            logger.error('cardtemplate render failed: %s', e, exc_info=True, extra={'data': {
                'cardtemplate': id,
                'block_data': data,
            }})
        return body

    def _render_card_nlg(self, block, form, context, req_info, response, schema=None):
        if not self.has_card(block.card_template, intent_name=form.name):
            logger.warning('Not found card nlg template "%s"', block.card_template)
            return None

        card_context = context.copy()
        card_context['data'] = block.data

        body = None
        try:
            body = self.render_card(block.card_template, form=form, context=card_context, req_info=req_info, schema=schema)
            self.track_nlg_render_history_record(req_info, response, card_id=block.card_template, form=form, context=card_context)
        except Exception as e:
            logger.error('Card render failed: %s', e, exc_info=True, extra={'data': {
                'card_template': block.card_template,
                'block_data': block.data,
            }})
        return body

    def _make_div_card(self, block, form, context, req_info, response):
        logger.debug('Handle %s div card block', block)

        cards = []
        if block.card_template is not None:
            body = self._render_card_nlg(block, form, context, req_info, response, schema=self._cards_schema)
            if body:
                cards.append(DivCard(body=body))
        elif block.card_layout is not None:
            try:
                validate_json(block.card_layout, self._cards_schema)
                cards.append(DivCard(body=block.card_layout))
            except ValidationError as e:
                logger.error(
                    'Pre-rendered div card validation failed: %s', e, exc_info=True,
                    extra={'data': {'card_layout': block.card_layout}}
                )
        else:
            logger.warning('Div card block received without either template or layout')

        return cards

    def _make_div2_card(self, block, form, context, req_info, response):
        logger.debug('Handle %s div2 card block', block)

        text = None
        if block.text_template:
            text = self.render_phrase(phrase_id=block.text_template, form=form, req_info=req_info).text
            self.track_nlg_render_history_record(req_info, response, phrase_id=block.text_template, form=form)
        elif block.text:
            text = block.text

        cards = []
        if block.card_template is not None:
            body = self._render_card_nlg(block, form, context, req_info, response)
            if body:
                cards.append(Div2Card(body=body, hide_borders=block.hide_borders, text=text))
        elif block.body is not None and block.hide_borders is not None:
            cards.append(Div2Card(body=block.body, hide_borders=block.hide_borders, text=text))
        else:
            logger.warning('Div2 card block received without either body or card_template')

        return cards

    def _make_cards(self, req_info, response, form, context, blocks):
        cards = []

        if get_bool_setting('DISABLE_DIV_CARDS'):
            # Can be useful for debug to emulate old clients using backend-side configuration only
            return cards

        for block in blocks:
            if isinstance(block, CardBlock):
                cards += self._make_div_card(block, form, context, req_info, response)
            elif isinstance(block, Div2CardBlock):
                cards += self._make_div2_card(block, form, context, req_info, response)

        return cards

    def _make_div2templates(self, req_info, form, context, blocks):
        templates = {}
        for block in filter(lambda b: isinstance(b, Div2CardBlock), blocks):
            logger.debug('Handle %s div2 cardtemplate block', block)

            if isinstance(block.templates, dict):
                for name, body in block.templates.items():
                    templates[name] = body

            if isinstance(block.template_names, list):
                for template_id in block.template_names:
                    body = self._render_cardtemplate_nlg(template_id, block.data, form, context, req_info)
                    if body:
                        templates[template_id] = body

        return templates

    def _render_buttons(self, req_info, response, session, blocks, context, client_features, should_render_nlg_in_hollywood_flag=False):
        # Add VINS-side suggests, remove some suggests based on information only VINS has
        form = session.form
        blocks = self._modify_suggest_list(req_info, session, blocks, client_features)

        suggests = []
        card_buttons = []
        for block in blocks:
            if not isinstance(block, SuggestBlock):
                continue

            if should_render_nlg_in_hollywood_flag:
                response.add_nlg_render_bass_block(block.to_dict())
                continue

            logger.debug('Handle %s suggest block', block)
            button, is_suggest = self._gen_button(
                form, block,
                context=dict(context, **{block.suggest_type: block}),
                action='render_buttons',
                req_info=req_info,
                response=response
            )
            if button:
                if is_suggest or not clients.supports_buttons(req_info.app_info):
                    suggests.append(button)
                else:
                    card_buttons.append(button)

        return suggests, card_buttons

    def _get_special_buttons_list(self, parent_name, customization, default_list=[]):
        if parent_name not in customization:
            return default_list
        buttons_list = []
        for button_name in customization[parent_name]:
            if button_name in self._special_buttons_desc:
                buttons_list.append(self._special_buttons_desc[button_name])
            else:
                logger.error('Got undefined special_button: %s', button_name)
        if not buttons_list:
            logger.error('Got empty special_button sublist: %s', json.dumps(customization, ensure_ascii=False))
            return default_list
        return buttons_list

    def _gen_special_button(self, request_id, button, customization, parent_type='special_button'):
        title = button.get('title', '')
        text = button.get('text', '')
        name = button.get('name', None)
        type_ = button.get('type', parent_type)
        sub_list = button.get('sub_list', None)

        directives = list()

        directives.append(ServerActionDirective(
            name='on_special_button',
            payload={
                'request_id': request_id,
                'type': type_,
                'name': name
            },
            ignore_answer=True))

        if sub_list:
            buttons_list = self._get_special_buttons_list(name, customization, sub_list['default_buttons'])
            buttons = [self._gen_special_button(request_id, x, customization, type_).to_dict() for x in buttons_list]

            directives.append(ClientActionDirective(
                name='special_button_list',
                sub_name='special_button_list',
                payload={
                    "title": sub_list['title'],
                    "special_buttons": buttons,
                    "default": self._gen_special_button(request_id, sub_list['default'], customization, type_)
                }
            ))

        if has_special_button(type_):
            return get_special_button(type_)(title=title, text=text, directives=directives)
        else:
            raise ValueError("Wrong Special Button type '%s'", type_)

    def _render_special_buttons(self, req_info, blocks):
        customization = {}
        for block in filter(lambda b: isinstance(b, SpecialButtonBlock), blocks):
            customization[block.button_type] = block.data.get('sub_list', [])
        special_buttons = [self._gen_special_button(str(req_info.request_id), button, customization)
                           for button in self._special_buttons_cfg]

        return special_buttons

    def _generate_text_cards(self, blocks, form, context, response, req_info, should_render_nlg_in_hollywood_flag=False):
        text_cards = []
        for block in filter(lambda b: isinstance(b, TextCardBlock), blocks):
            logger.debug('Render text card block %s', block)

            if not self.has_phrase(phrase_id=block.phrase_id, intent_name=form.name):
                logger.warning(
                    'Don\'t know how to render text card block %s for intent %s',
                    block, form.name,
                )
                continue

            if should_render_nlg_in_hollywood_flag:
                response.add_nlg_render_bass_block(block.to_dict())
                continue

            text_card_context = context.copy()
            text_card_context['data'] = block.data
            render_result = self.render_phrase(block.phrase_id, form=form, req_info=req_info,
                                               context=text_card_context)
            self.track_nlg_render_history_record(req_info, response, phrase_id=block.phrase_id, form=form, context=text_card_context)
            text_cards.append({
                'voice_text': render_result.voice or None,
                'card_text': render_result.text,
                'card_tag': (block.data or {}).get('card_tag'),
                'append': True,
            })
        return text_cards

    def _say_cards(self, cards, response):
        for card in cards:
            response.say(**card)

    def _is_text_banned(self, sample, banlist_name='banlist'):
        banlist = self.dm.nlu.get_classifier(banlist_name)
        for intent, proba in banlist(SampleFeatures(sample)).iteritems():
            if proba > 0 and intent == intents.GENERAL_CONVERSATION_DUMMY:
                return True
        return False

    def _is_text_swear(self, text):
        parser_result = self._swear_parser.parse(Sample.from_string(text))
        if len(parser_result) < 1:
            return False
        if 'ners' not in parser_result[0]:
            return False
        return len(parser_result[0]['ners']) > 0

    def _get_vins_markup_for_external_skill(self, sample):
        result = {}

        # Run GC banlist to figure out if the current utterance is dangerous
        if self._is_text_banned(sample):
            result['dangerous_context'] = True

        return result

    def _parse_client_features(self, blocks):
        client_features = set()

        for block in filter(lambda b: isinstance(b, ClientFeaturesBlock), blocks):
            for feature_name, value in block.data.get("features", {}).items():
                if value.get("enabled", False):
                    client_features.add(feature_name)

        return client_features

    @sensors.with_timer('pa_handle_response_time')
    def _handle_response(self, bass_resp, response, session, req_info, sample, sample_features,
                         additional_nlg_context, analytics_info, raise_error=False):
        form, should_resubmit_form = self._update_form(session.form, req_info, bass_resp.form_info)

        if self.should_pass_through_bass_response(req_info):
            response.set_bass_response(bass_resp)

        for block in bass_resp.blocks:
            if isinstance(block, AttentionBlock):
                if block.attention_type == 'irrelevant':
                    prev_intent_name = form.name
                    intent_name = intents.get_irrelevant_intent(intent_name=prev_intent_name, req_info=req_info)
                    logger.info('Got irrelevant block %s, data = (%s). Changing form to %s',
                                prev_intent_name, unicode(block.data), intent_name)
                    form = self.new_form(intent_name, req_info=req_info)
                    form.intent_name.set_value(prev_intent_name, 'string')
                    self.change_form(session=session, form=form, req_info=req_info, sample=None, response=response)
                    return

        if should_resubmit_form and sample_features is not None:
            intent_scores = [IntentScore(name=form.name)]
            sample_features.add_classification_scores('final_intent', intent_scores)

        self.dump_debug_info(sample, sample_features, req_info, response)

        # All NLG should be generated given of the updated form
        session.change_form(form)

        client_features = self._parse_client_features(bass_resp.blocks)

        if should_resubmit_form:
            # If the form needs to be resubmitted, there should be no result rendering

            # Saving bass result in order to use it in following
            # _handle_response instead of geting real BASS response once again
            session.set(bass_result_key, bass_resp, transient=True)
            self.change_form(session=session, form=form, req_info=req_info, sample=sample, response=response)
            return

        should_render_nlg_in_hollywood_flag = self.should_render_nlg_in_hollywood(req_info)

        context = self._make_nlg_context(bass_resp.blocks, additional_nlg_context)
        if should_render_nlg_in_hollywood_flag:
            self._track_bass_blocks_for_nlg_context(bass_resp.blocks, response)

        # Add info about error and attention blocks to response
        self._add_block_info_to_response_meta(bass_resp.blocks, response, form.name if form else None)

        if session.get('pure_general_conversation'):
            # the order of metas matters: in yandex-io-sdk, only the first meta is used
            response.add_meta(ExternalSkillMeta(skill_name=GC_SKILL_NAME))
            response.add_meta(GeneralConversationMeta(pure_gc=True))

        # Check if BASS altered the session state.
        if bass_resp.session_state is not None:
            bass_session_state.set_bass_session_state(session, bass_resp.session_state)

        # See if there are any error blocks
        error_blocks = [block for block in bass_resp.blocks if isinstance(block, ErrorBlock)]
        for error_block in error_blocks:
            self._render_error_block(session.form, error_block, context, req_info, response, should_render_nlg_in_hollywood_flag)
            sensors.inc_counter('pa_bass_error_block', labels={'error_type': error_block.error_type})

        if error_blocks and raise_error:
            raise PersonalAssistantErrorBlockError(error_blocks)

        # Produce directives, buttons and div-cards
        suggests, card_buttons = self._render_buttons(req_info, response, session, bass_resp.blocks, context,
                                                      client_features, should_render_nlg_in_hollywood_flag)
        div_cards = self._make_cards(req_info, response, session.form, context, bass_resp.blocks)
        templates = self._make_div2templates(req_info, session.form, context, bass_resp.blocks)
        directives = self._make_directives(req_info, bass_resp.blocks)
        # TODO(autoapp): remove should_render_result and directives translation when autoapp's dead
        if clients.is_autoapp(req_info.app_info):
            new_intent, directives = to_autoapp_directives(directives, form.name, req_info)
            if new_intent:
                new_form = self.new_form(new_intent, req_info)
                self.change_form(session=session, form=new_form, req_info=req_info, sample=None, response=response)
                return

        special_buttons = []
        if (self._should_add_feedback(session.form and session.form.name, req_info, session, bass_resp.blocks) and
                self._should_use_special_buttons(req_info, client_features)):
            special_buttons = self._render_special_buttons(req_info, bass_resp.blocks)

        # And add them to the response
        response.suggests += suggests
        response.directives += directives
        response.special_buttons += special_buttons

        if analytics_info is not None:
            for block in bass_resp.blocks:
                if isinstance(block, AnalyticsInfoBlock):
                    analytics_info.scenario_analytics_info_data = block.data

        # Whether we should always listen or never listen after providing the response
        # irregardless of the defaults
        should_listen = intents.should_listen(session.intent_name, req_info, form)
        if should_listen is not None:
            response.should_listen = should_listen

        if any(isinstance(block, StopListeningBlock) for block in bass_resp.blocks):
            response.should_listen = False

        for block in bass_resp.blocks:
            if isinstance(block, AutoactionDelayMsBlock):
                response.autoaction_delay_ms = block.delay_ms
                break

        for block in bass_resp.blocks:
            if isinstance(block, PlayerFeaturesBlock):
                response.player_features = block
                break

        response.force_voice_answer = intents.should_force_voice_answer(session.intent_name, form) or \
            req_info.experiments['force_voice_answer'] is not None

        # Do not render anything if there is SilentResponseBLock
        if any(isinstance(block, SilentResponseBlock) for block in bass_resp.blocks):
            response.should_listen = False
            return

        continuation_block = next((block for block in bass_resp.blocks if isinstance(block, CommitCandidateBlock)),
                                  None)
        if continuation_block is not None:
            response.set_commit_arguments(continuation_block.data)

        # Stop processing if there are any errors
        if error_blocks:
            return

        # Render result only if there are no errors

        # TODO: this is the old way of triggering client actions in response to a user action.
        # TODO: Remove it after all scenarios are rewritten using command blocks
        # If a URI can be rendered, tell the client to open it
        # TODO: we should learn to render 'render_uri' in hollywood also, like 'render_result'
        if self.has_phrase(phrase_id='render_uri', intent_name=form and form.name):
            uri = self.render_phrase(phrase_id='render_uri', form=form, context=context, req_info=req_info).text
            self.track_nlg_render_history_record(req_info, response, phrase_id='render_uri', form=form, context=context)
            if uri:
                response.directives.append(
                    ClientActionDirective(name='open_uri', sub_name=form.name, payload={'uri': uri})
                )

        text_and_voice_cards = self._generate_text_cards(bass_resp.blocks, form, context, response, req_info, should_render_nlg_in_hollywood_flag)
        card_with_buttons_id = 1

        if self.has_phrase(phrase_id='render_result', intent_name=form and form.name):
            if should_render_nlg_in_hollywood_flag:
                response.add_nlg_render_bass_block({'type': 'text_card', 'phrase_id': 'render_result'})
            else:
                postprocess_list = []
                if intents.is_username_autoinsert_allowed(form and form.name, req_info):
                    postprocess_list.append("postprocess__prepend_username")

                render_result = self.render_phrase(
                    'render_result', form=form, context=context, session=session,
                    req_info=req_info, postprocess_list=postprocess_list
                )
                self.track_nlg_render_history_record(req_info, response, phrase_id='render_result', form=form, context=context)

                # render usual text message only if there is no div_cards
                if not div_cards:
                    if render_result.text:
                        text_and_voice_cards.append({
                            "voice_text": render_result.voice or None,
                            "card_text": render_result.text,
                            "append": True,
                        })
                    elif render_result.voice:
                        card_with_buttons_id += 1
                        text_and_voice_cards.append({
                            "voice_text": render_result.voice,
                            "card_text": False,
                            "append": True
                        })
                else:
                    # otherwise render only voice
                    if render_result.voice:
                        card_with_buttons_id += 1
                        text_and_voice_cards.append({
                            "voice_text": render_result.voice,
                            "card_text": False,
                            "append": True
                        })

        if len(text_and_voice_cards) >= card_with_buttons_id:
            text_and_voice_cards[-card_with_buttons_id]["buttons"] = card_buttons

        # add text and voice cards, then div_cards, then templates for div2 cards
        self._say_cards(text_and_voice_cards, response)
        response.cards += div_cards
        response.templates = templates

        # We should stop listening if the call results in a client action
        # (not always though)
        listening_is_possible = set()
        for directive in response.directives:
            if directive.payload and directive.payload.get('listening_is_possible'):
                listening_is_possible.add(directive.name)

        for directive in response.directives:
            if directive.name not in listening_is_possible:
                response.should_listen = False

        # Add frame actions
        for block in bass_resp.blocks:
            if isinstance(block, FrameActionBlock):
                action_id = block.action_id
                frame_action = block.frame_action
                response.frame_actions[action_id] = frame_action

        # Add scenario data
        for block in bass_resp.blocks:
            if isinstance(block, ScenarioDataBlock):
                response.scenario_data = block.scenario_data

        # Add stack engine
        for block in bass_resp.blocks:
            if isinstance(block, StackEngineBlock):
                response.stack_engine = block.data
                response.should_listen = False

    def dump_debug_info(self, sample, sample_features, req_info, response):
        if (req_info.experiments['debug_classification_scores'] is not None and
                sample_features is not None):

            data = {
                'type': 'intent_predictions',
                'data': [{stage: [score.to_dict() for score in scores]}
                         for stage, scores in sample_features.classification_scores.iteritems()]
            }
            response.add_meta(DebugInfoMeta(data=data))

        if (req_info.experiments['debug_tagger_scores'] is not None and
                sample_features is not None):

            data = {
                'type': 'tagger_predictions',
                'data': {
                    'tokens': sample.tokens,
                    'scores': [{stage: [score.to_dict() for score in scores]}
                               for stage, scores in sample_features.tagger_scores.iteritems()]
                }
            }
            response.add_meta(DebugInfoMeta(data=data))

    @callback_method
    def phone_call_callback(self, req_info, session, response, sample,
                            balancer_type=FAST_BASS_QUERY,
                            bass_result=None,
                            sample_features=None,
                            analytics_info=None,
                            **kwargs):
        if not bass_result:
            if session.form and isinstance(req_info.event, VoiceInputEvent):
                possible_contacts = req_info.event.context_hint('possible_contacts')
                if possible_contacts and isinstance(possible_contacts, list):
                    slot = session.form.get_slot_by_name('personal_asr_value')
                    if slot:
                        slot.set_value(possible_contacts[0].lower(), 'string')
                    else:
                        session.form.slots.append(Slot(
                            name='personal_asr_value',
                            optional=True,
                            types=['string'],
                            value_type='string',
                            value=possible_contacts[0].lower())
                        )

            self._universal_callback_impl(req_info, session, response, sample, sample_features=sample_features,
                                                  balancer_type=balancer_type, analytics_info=analytics_info, **kwargs)
        else:
            self._handle_response(
                bass_result, response=response, session=session, req_info=req_info, sample=sample,
                sample_features=sample_features, additional_nlg_context=None, analytics_info=analytics_info
            )
