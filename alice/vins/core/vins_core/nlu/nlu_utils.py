# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re
import logging

from collections import Mapping, Iterable
from vins_core.utils.misc import match_with_dict_of_patterns
from vins_core.nlu.features.base import TaggerScore, TaggerSlot


logger = logging.getLogger(__name__)


def log_allowed(prefix, allowed, intent='', expected=''):
    logger.debug('%s is%s allowed for intent %s, expected %s', prefix, ('' if allowed else ' not'), intent, expected)


def _get_intent_name(config_or_intent):
    if isinstance(config_or_intent, Mapping):
        return ''  # it's a context config, intent name is not known
    else:
        return config_or_intent.name


def is_allowed_app(config_or_intent, req_info):
    if isinstance(config_or_intent, Mapping):
        app = config_or_intent.get('app')
    else:
        app = config_or_intent.allowed_apps
    if app is None:
        return True

    result = bool(re.match(app, req_info.app_info.app_id or ''))
    log_allowed('App', result, intent=_get_intent_name(config_or_intent), expected=app)
    return result


def is_allowed_device_state(config_or_intent, req_info):
    if isinstance(config_or_intent, Mapping):
        device_state = config_or_intent.get('device_state')
    else:
        device_state = config_or_intent.allowed_device_states
    if device_state is None:
        return True

    result = match_with_dict_of_patterns(device_state, req_info.device_state, try_regex=True)
    log_allowed('Device state', result, intent=_get_intent_name(config_or_intent), expected=device_state)
    return result


def is_allowed_experiment(config_or_intent, req_info):
    if isinstance(config_or_intent, Mapping):
        experiments = config_or_intent.get('experiments')
    else:
        experiments = config_or_intent.experiments
    if experiments is None:
        return True

    result = req_info and req_info.experiments and \
             (any((experiments == experiment) for experiment, _ in req_info.experiments.items()))
    log_allowed('Experiment', result, intent=_get_intent_name(config_or_intent), expected=experiments)
    return result


def is_allowed_prev_intent(config_or_intent, session):
    if isinstance(config_or_intent, Mapping):
        prev_intents = config_or_intent.get('prev_intents')
    else:
        prev_intents = config_or_intent.allowed_prev_intents
    if prev_intents is None:
        return True

    result = session is not None and \
             re.match(prev_intents, session.intent_name or '')
    log_allowed('Prev intent', result, intent=_get_intent_name(config_or_intent), expected=prev_intents)
    return result


def is_allowed_active_slot(config_or_intent, session):
    if isinstance(config_or_intent, Mapping):
        allowed_active_slots = config_or_intent.get('active_slots')
    else:
        allowed_active_slots = config_or_intent.allowed_active_slots
    if allowed_active_slots is None:
        return True

    if session is None or session.form is None:
        log_allowed('Active slot', False)
        return False

    active_slots = {slot.name for slot in session.form.active_slots()}
    # The field "active_slots" can be only None, bool or list of strings
    result = isinstance(allowed_active_slots, bool) and allowed_active_slots == bool(active_slots) or \
             isinstance(allowed_active_slots, Iterable) and bool(set(allowed_active_slots).intersection(active_slots))
    log_allowed('Active slot', result, intent=_get_intent_name(config_or_intent), expected=allowed_active_slots)
    return result


def is_allowed_request(config_or_intent, filters_dict, req_info):
    if isinstance(config_or_intent, Mapping):
        filter_name = config_or_intent.get('request_filter')
    else:
        filter_name = config_or_intent.request_filter
    if filter_name is None:
        return True

    if req_info is None or filters_dict is None:
        log_allowed('Request', False)
        return False

    if filter_name not in filters_dict:
        logger.warning('Request filter "{}" was not found in filters dict.'.format(filters_dict))
    logger.debug('Checking request filter "{}": equals "{}"'.format(filter_name, filters_dict[filter_name](req_info)))
    result = filters_dict[filter_name](req_info)
    log_allowed('Request', result, intent=_get_intent_name(config_or_intent), expected=filter_name)
    return result


def should_skip_classifier(name, config_or_intent, req_info=None):
    if not req_info:
        return False
    if not is_allowed_app(config_or_intent, req_info):
        logger.debug('Classifier %s is not allowed for current app', name)
        return True
    return False


def semantic_frames_to_taggers_scores(semantic_frames):
    taggers_scores = []
    if semantic_frames is not None:
        for frame in semantic_frames:
            slots = []
            for slot_name, slot_values in frame['slots'].iteritems():
                for values in slot_values:
                    slots.append(TaggerSlot(
                        start=values['start'],
                        end=values['end'],
                        is_continuation=values['is_continuation'],
                        value=slot_name
                    ))
            taggers_scores.append(TaggerScore(intent=frame['intent_name'], score=frame['tagger_score'], slots=slots))
    return taggers_scores
