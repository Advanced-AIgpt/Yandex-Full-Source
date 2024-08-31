# coding: utf-8
from __future__ import unicode_literals

import logging
import jsonschema
import marisa_trie
import re
import random
from collections import Iterable

import attr

from vins_core.config import schemas
from vins_core.nlu.flow_nlu_factory.transition_model import MarkovTransitionModel
from vins_core.nlu.nlu_utils import (
    is_allowed_app, is_allowed_device_state, is_allowed_experiment, is_allowed_active_slot
)
from vins_core.utils.data import load_data_from_file

from personal_assistant.item_selection import get_item_selector_for_intent
from personal_assistant import intents, clients
from personal_assistant.is_allowed_intent.config_loader import load_config
from personal_assistant.is_allowed_intent.resolver import Resolver
from personal_assistant.is_allowed_intent.predicates import reg
from .transition_model_rules import create_transition_rules

logger = logging.getLogger('transition_model')


def get_boosts(priority_boost, boosts):
    def to_priority_boost(boost_val):
        if boost_val == 'priority_boost':
            return priority_boost
        return boost_val

    @attr.s(frozen=True, slots=True)
    class Boosts(object):
        active_slot_ellipsis = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        anaphora = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        chat_tab = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        internal = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        internal_search = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        navigator = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_fairy_tale = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_music = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_open_or_continue = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_open_or_continue_on_video_screens = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_open_site_or_app = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_stop_buzzing = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_tv = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        quasar_video_mode = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        stroka_desktop = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        video_without_screen = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        browser_reading = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        autoapp = attr.ib(type=float, converter=to_priority_boost, default=1.0)
        image_what_is_this_specific = attr.ib(type=float, converter=to_priority_boost, default=1.0)

    return Boosts(**boosts)


class PersonalAssistantTransitionModel(MarkovTransitionModel):
    def __init__(self, intents, priority_boost=1.000001, boosts=None, custom_rules=None, **kwargs):
        super(PersonalAssistantTransitionModel, self).__init__(**kwargs)
        self._priority_boost = priority_boost
        self.boosts = get_boosts(priority_boost, boosts or {})

        self._custom_rules = create_transition_rules(self._parse_transition_rules(custom_rules)) if custom_rules else []
        self._intents = {intent.name: intent for intent in intents}
        self._precompute_transition_probs(intents)
        config = load_config('personal_assistant/is_allowed_intent/config.yaml')
        self._resolver = Resolver('intent_name', self._intents.keys(), reg.predicates, config)

    def __call__(self, intent_name, session, req_info=None):
        current_form = session.form
        current_intent_name = session.intent_name

        # Basic score
        score = self.get_static_score(current_intent_name, intent_name)
        if score <= 1e-10:
            return 0.0

        allowed = True
        if req_info:
            if req_info.dialog_id is not None:
                allowed = intents.is_skill(intent_name)
            else:
                if intent_name in (session.get('skip_intents') or []) or not self._resolver.process(
                        {'intent_name': intent_name, 'session': session, 'req_info': req_info}):
                    allowed = False

            if not intents.allow_external_only_frame(intent_name, req_info):
                logger.debug('Intent %s is forbidden by allow_external_only_frame rule', intent_name)
                allowed = False

        if allowed:
            if (
                intent_name in intents.get_forbidden_intents(req_info) or
                (req_info and req_info.experiments['add_forbidden_intent=' + intent_name] is not None)
            ):
                allowed = False
                logger.debug('Intent %s is forbidden by experiments', intent_name)
        if not allowed:
            logger.debug('Intent %s is not allowed', intent_name)
            return 0.0

        item_selector = get_item_selector_for_intent(intent_name, req_info)
        if (
            item_selector and
            not item_selector.allows_empty_item_list and
            not item_selector.get_items(req_info, current_form)
        ):
            # Disallow item selection intents that have nothing to choose from
            logger.debug('Item selection for intent %s having nothing to choose from', intent_name)
            return 0.0

        if intents.is_stateful_scenario(current_intent_name):
            if intents.is_disallowed_stateful_transition(current_form, current_intent_name, intent_name):
                logger.debug('Intent %s is not allowed by state', intent_name)
                return 0.0

        # Dynamic boosts
        boost = 1.0

        active_rules = [
            rule for rule in self._custom_rules
            if rule.check(intent_name, current_intent_name, current_form)
        ]
        if active_rules:
            logger.debug(
                'Scenarios %s->%s active rules: %s',
                current_intent_name, intent_name, active_rules
            )
            if len(active_rules) > 1:
                logger.debug('More than one rule is active, their boosts will be multipled')
            for active_rule in active_rules:
                boost *= active_rule.get_boost(intent_name, current_intent_name, current_form)
                if boost <= 1e-10:
                    return 0.0
            logger.debug('Active rules boost (%s): %s', intent_name, boost)

        intent = self._intents.get(intent_name)

        # additional checks of allowed_apps, allowed_device_states and experiments from VinsProjectFile config
        if intent and not is_allowed_app(intent, req_info):
            logger.debug('Intent %s is not allowed by app', intent_name)
            return 0.0
        if intent and not is_allowed_device_state(intent, req_info):
            logger.debug('Intent %s is not allowed by device state', intent_name)
            return 0.0
        if intent and not is_allowed_experiment(intent, req_info):
            logger.debug('Intent %s is not allowed by experiment', intent_name)
            return 0.0
        if intent and not is_allowed_active_slot(intent, session):
            logger.debug('Intent %s is not allowed by active slot', intent_name)
            return 0.0

        if current_form and current_form.has_active_slots():
            # When expecting an answer to a question, boost the corresponding elliptical intent
            current_parent_name = intents.parent_intent_name(current_intent_name)
            if intents.is_ellipsis(intent_name) and intents.parent_intent_name(intent_name) == current_parent_name:
                boost *= self.boosts.active_slot_ellipsis
                logger.debug('Active slot ellipsis boost (%s) %s', intent_name, self.boosts.active_slot_ellipsis)

        if intents.requires_quasar_tv_boost(intent_name):
            # Boost screen-dependent intents if we are on the right screen
            boost *= self.boosts.quasar_tv
            logger.debug('Quasar TV boost (%s) %s', intent_name, self.boosts.quasar_tv)

        if intents.is_stop_buzzing_intent(intent_name):
            # Boost stop buzzing intents if buzzer is active
            boost *= self.boosts.quasar_stop_buzzing
            logger.debug('Quasar stop buzzing boost (%s) %s', intent_name, self.boosts.quasar_stop_buzzing)

        if intents.is_image_what_is_this_specific_intent(intent_name):
            # Boost image_what_is_this specific intent to outrun pure image_what_is_this
            boost *= self.boosts.image_what_is_this_specific
            logger.debug('Image_what_is_this specific boost (%s) %s', intent_name, self.boosts.image_what_is_this_specific)

        if (
            req_info and
            req_info.experiments['music_worse_classification'] is not None and
            intents.is_music_intent(intent_name) and
            random.randint(0, 9) == 0
        ):
            logger.debug('Music(%s) was blocked by music_worse_classification experiment', intent_name)
            return 0.0

        if (
                req_info and
                req_info.experiments['quasar_music_boost'] is not None and
                intents.requires_quasar_music_boost(intent_name)
        ):
            device_state = req_info.device_state or {}
            video_state = device_state.get('video') or {}
            current_screen = video_state.get('current_screen')
            if current_screen == 'music_player':
                boost *= self.boosts.quasar_music
                logger.debug('Quasar music boost (%s) %s', intent_name, self.boosts.quasar_music)

        if (
                req_info and
                req_info.experiments['quasar_video_mode_boost'] is not None and
                intents.requires_quasar_video_mode_boost(intent_name)
        ):
            device_state = req_info.device_state or {}
            video_state = device_state.get('video') or {}
            current_screen = video_state.get('current_screen')
            if current_screen is not None and current_screen not in ['main', 'music_player']:
                boost *= self.boosts.quasar_video_mode
                logger.debug('Quasar video mode boost (%s) %s', intent_name, self.boosts.quasar_video_mode)

        if req_info and clients.is_stroka(req_info.app_info) and intents.is_stroka_intent(intent_name):
            # Boost stroka intents for appropriate client.
            boost *= self.boosts.stroka_desktop
            logger.debug('Stroka boost (%s) %s', intent_name, self.boosts.stroka_desktop)

        if req_info and clients.is_smart_speaker(req_info.app_info) and intents.is_music_fairy_tale_intent(intent_name):
            # Boost fairy tale intent a little to avoid external skill activation
            boost *= self.boosts.quasar_fairy_tale
            logger.debug('Quasar tale intent boost (%s) %s', intent_name, self.boosts.quasar_fairy_tale)

        if req_info and clients.is_smart_speaker(req_info.app_info) and intents.is_open_site_or_app(intent_name):
            # Boost down open_site_or_app intent to allow competing video_play intent win in some cases
            boost *= self.boosts.quasar_open_site_or_app
            logger.debug('Quasar open site or app boost (%s) %s', intent_name, self.boosts.quasar_open_site_or_app)

        if req_info and clients.is_smart_speaker(req_info.app_info) and intents.is_open_or_continue_intent(intent_name):
            # boost down player continuation intent when it has a secondary role
            device_state = req_info.device_state or {}
            video_state = device_state.get('video', {})
            current_screen = video_state.get('current_screen')
            if current_screen not in ['description', 'video_player', 'payment', 'season_gallery']:
                boost *= self.boosts.quasar_open_or_continue
                logger.info('Quasar open or contine boost (%s) %s', intent_name, self.boosts.quasar_open_or_continue)
            else:
                # boost down just a little bit open_or_continue on video screens to avoid situation when
                # open_or_continue and video_play intents have exactly equal scores.
                boost *= self.boosts.quasar_open_or_continue_on_video_screens
                logger.info('Quasar open or contine boost on_video_screens (%s) %s', intent_name, self.boosts.quasar_open_or_continue_on_video_screens)

        if req_info and intents.is_video_play_intent(intent_name) and clients.is_smart_speaker_without_screen(req_info):
            boost *= self.boosts.video_without_screen
            logger.debug('Boost for video_play without screen (%s) %s', intent_name, self.boosts.video_without_screen)

        if req_info and clients.is_client_with_navigator(req_info) and intents.requires_navigator_boost(intent_name):
            # Boost navigator intents for appropriate client.
            boost *= self.boosts.navigator
            logger.debug('Navigator boost (%s) %s', intent_name, self.boosts.navigator)

        if (req_info and clients.is_autoapp(req_info.app_info) and
                intents.is_autoapp_microintent(intent_name)):
            # Boost AutoApp intents for appropriate client.
            boost *= self.boosts.autoapp
            logger.debug('AutoApp boost (%s) %s', intent_name, self.boosts.autoapp)

        if req_info and req_info.dialog_id is not None:
            boost *= self.boosts.chat_tab
            logger.debug('Chat tab boost (%s) %s', intent_name, self.boosts.chat_tab)

        if intents.is_browser_read_page_pause_continue_intent(intent_name):
            device_state = req_info.device_state if req_info and req_info.device_state else {}
            is_reading = device_state.get('browser', {}).get("is_reading")
            if is_reading:
                boost *= self.boosts.browser_reading
                logger.debug('Boost for browser is_reading (%s) %s', intent_name, self.boosts.browser_reading)
            else:
                return 0.0

        if boost != 1.0:
            logger.debug('Dynamic transition model boost %s for intent %s', boost, intent_name)

        return score * boost

    def is_priority_boost(self, boost):
        return boost == self._priority_boost

    def _precompute_transition_probs(self, intents):
        intent_trie = marisa_trie.Trie(intent.name for intent in intents)

        for intent in intents:
            self.add_transition(
                None,
                intent.name,
                self._compute_transition_score(intent_trie, None, intent)
            )

        for prev_intent in intents:
            for intent in intents:
                self.add_transition(
                    prev_intent.name,
                    intent.name,
                    self._compute_transition_score(intent_trie, prev_intent, intent)
                )

    @staticmethod
    def _intent_info(intent_trie, intent_name):
        parent_name = intents.parent_intent_name(intent_name)
        is_internal = intents.is_internal(intent_name)

        if is_internal:
            has_internal = True
        else:
            keys = intent_trie.keys(parent_name + intents.INTERNAL_INTENT_SEPARATOR)
            has_internal = bool(keys)

        return parent_name, is_internal, has_internal

    def _validate_transition_rules(self, rules):
        jsonschema.validate(rules, schemas.transition_rules_schema)

    def _parse_transition_rules(self, custom_rules):
        if custom_rules['enable']:
            path = custom_rules['path']
            rules = load_data_from_file(path)['rules']
            self._validate_transition_rules(rules)
            return rules

        return []

    def _is_possible_external_skill_transition(self, prev_intent, curr_intent):
        def not_skill(name):
            return not intents.is_skill(name)

        activ_only = intents.is_skill_activate_only
        activ = intents.is_skill_activate
        deact = intents.is_skill_deactivate
        cont = intents.is_skill_continue

        rules = [
            (activ_only, not_skill),
            (activ_only, activ),
            (not_skill, activ),
            (activ, cont),
            (activ, deact),
            (cont, cont),
            (cont, deact),
            (deact, not_skill),
            (deact, activ),
        ]

        for prev, cur in rules:
            if prev(prev_intent) and cur(curr_intent):
                return True

        return False

    def _compute_transition_score(self, intent_trie, prev_intent, intent):
        # The main idea behind this model:
        #  * P(intent_X | intent_X) = 1
        #  * P(intent_X | intent_X__internal) = 1
        #  * P(intent_X__internal | intent_Y) = 0 because we don't want 'internal' and elliptic utterances to trigger the intent.  # noqa
        #  * P(intent_X__internal | intent_X) > P(intent_Y | intent_X) because when we started doing something,
        #      we're more likely to continue than to switch to another task
        #  * P(intent_X__internal | intent_X__internal) > P(intent_Y | intent_X__internal) for the same reason

        # don't try to leave external_skill
        prev_intent_name = prev_intent and prev_intent.name
        intent_name = intent.name

        if prev_intent_name and (
                intents.is_skill(prev_intent_name) or intents.is_skill(intent_name)
        ):
            if not self._is_possible_external_skill_transition(prev_intent_name, intent_name):
                return 0

        # Are we currently inside multistep scenario?
        if intents.is_multistep_scenario(prev_intent_name):
            if intents.is_disallowed_multistep_transition(prev_intent_name, intent_name):
                return 0.0

        # is there a list of allowed previous intents in which the previous intent does not fit?
        allowed_prev_intents = intent.allowed_prev_intents
        if allowed_prev_intents:
            if not prev_intent_name:
                return 0.0
            elif isinstance(allowed_prev_intents, (str, unicode)):
                if not re.match(allowed_prev_intents, prev_intent_name):
                    return 0.0
            elif isinstance(allowed_prev_intents, Iterable):
                if intents.trim_name_prefix(prev_intent_name, 1) not in allowed_prev_intents:
                    return 0.0
        # for microintents, no more checks are required
        if intents.is_microintent(intent_name):
            return 1.0

        parent_name, is_internal, has_internal = self._intent_info(intent_trie, intent_name)

        # There is no previous intent

        if prev_intent_name is None:
            return 0.0 if is_internal else 1.0

        prev_parent_name, prev_is_internal, _ = self._intent_info(intent_trie, prev_intent_name)

        if intents.is_anaphora_resolving_intent(intent_name):
            if intents.is_anaphora_source_intent(prev_intent_name):
                return self.boosts.anaphora
            else:
                return 0.0

        # Transitioning into an internal intent

        if is_internal:
            # Can only transition from another internal intent with the same parent or the parent itself
            if parent_name == prev_parent_name:
                if intents.is_search(intent_name):
                    # Don't boost internal search intents
                    return self.boosts.internal_search
                else:
                    return self.boosts.internal
            else:
                return 0.0

        # Transitioning into a non-internal intent

        # We can enter intents with reset_form=true from anywhere
        if intent.reset_form:
            return 1.0

        # Intents with internal intents can only be entered from outside
        return 1.0 if not has_internal or parent_name != prev_parent_name else 0.0


def create_pa_transition_model(*args, **kwargs):
    return PersonalAssistantTransitionModel(*args, **kwargs)
