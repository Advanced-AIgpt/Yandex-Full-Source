# coding: utf-8
from __future__ import unicode_literals

import copy
import logging
from functools import wraps

import attr

from personal_assistant import analytics
from personal_assistant import clients
from personal_assistant import intents
from personal_assistant.bass_result import BassFormInfo
from personal_assistant.meta import CancelListening
from vins_core.dm.response import (
    AnalyticsInfoMeta, ErrorMeta, MegamindActionDirective, ApplyArguments, FormInfoFeatures
)
from vins_core.utils.metrics import sensors
from vins_core.utils.intent_renamer import get_intent_for_metrics
from vins_sdk.app import callback_method as callback_decorator

logger = logging.getLogger(__name__)

bass_result_key = 'bass_result'

intents_expects_request = {
    'personal_assistant.scenarios.create_reminder',
    'personal_assistant.scenarios.create_reminder__cancel',
    'personal_assistant.scenarios.create_reminder__ellipsis',
    'personal_assistant.scenarios.list_reminders',
    'personal_assistant.scenarios.list_reminders__scroll_next',
    'personal_assistant.scenarios.list_reminders__scroll_reset',
    'personal_assistant.scenarios.list_reminders__scroll_stop',
    'personal_assistant.scenarios.cancel_todo',
    'personal_assistant.scenarios.cancel_todo__ellipsis',
    'personal_assistant.scenarios.create_todo',
    'personal_assistant.scenarios.create_todo__cancel',
    'personal_assistant.scenarios.create_todo__ellipsis',
    'personal_assistant.scenarios.list_todo',
    'personal_assistant.scenarios.list_todo__scroll_next',
    'personal_assistant.scenarios.list_todo__scroll_stop',
    'personal_assistant.internal.bugreport',
    'personal_assistant.internal.bugreport__continue',
    'personal_assistant.internal.bugreport__deactivate',
    'personal_assistant.scenarios.how_much',
    'personal_assistant.scenarios.how_much__ellipsis',
    'personal_assistant.scenarios.market_native',
    'personal_assistant.scenarios.market_native_beru',
    'personal_assistant.scenarios.market',
    'personal_assistant.scenarios.market_beru',
    'personal_assistant.scenarios.market__cancel',
    'personal_assistant.scenarios.market__garbage',
    'personal_assistant.scenarios.market__repeat',
    'personal_assistant.scenarios.market__show_more',
    'personal_assistant.scenarios.market__start_choice_again',
    'personal_assistant.scenarios.market__market',
    'personal_assistant.scenarios.market__market__ellipsis',
    'personal_assistant.scenarios.market__market__params',
    'personal_assistant.scenarios.market__number_filter',
    'personal_assistant.scenarios.market__product_details',
    'personal_assistant.scenarios.market__beru_order',
    'personal_assistant.scenarios.market__add_to_cart',
    'personal_assistant.scenarios.market__go_to_shop',
    'personal_assistant.scenarios.market_product_details',
    'personal_assistant.scenarios.market_product_details__beru_order',
    'personal_assistant.scenarios.market__checkout',
    'personal_assistant.scenarios.market__checkout_items_number',
    'personal_assistant.scenarios.market__checkout_email',
    'personal_assistant.scenarios.market__checkout_phone',
    'personal_assistant.scenarios.market__checkout_address',
    'personal_assistant.scenarios.market__checkout_index',
    'personal_assistant.scenarios.market__checkout_delivery_intervals',
    'personal_assistant.scenarios.market__checkout_yes_or_no',
    'personal_assistant.scenarios.market__checkout_everything',
    'personal_assistant.scenarios.recurring_purchase',
    'personal_assistant.scenarios.recurring_purchase__login',
    'personal_assistant.scenarios.recurring_purchase__cancel',
    'personal_assistant.scenarios.recurring_purchase__garbage',
    'personal_assistant.scenarios.recurring_purchase__repeat',
    'personal_assistant.scenarios.recurring_purchase__ellipsis',
    'personal_assistant.scenarios.recurring_purchase__number_filter',
    'personal_assistant.scenarios.recurring_purchase__product_details',
    'personal_assistant.scenarios.recurring_purchase__checkout',
    'personal_assistant.scenarios.recurring_purchase__checkout_items_number',
    'personal_assistant.scenarios.recurring_purchase__checkout_index',
    'personal_assistant.scenarios.recurring_purchase__checkout_yes_or_no',
    'personal_assistant.scenarios.recurring_purchase__checkout_suits',
    'personal_assistant.scenarios.recurring_purchase__checkout_delivery_intervals',
    'personal_assistant.scenarios.recurring_purchase__checkout_everything',
    'personal_assistant.scenarios.market_beru_my_bonuses_list',
    'personal_assistant.scenarios.market_beru_my_bonuses_list__login',
    'personal_assistant.scenarios.shopping_list_add',
    'personal_assistant.scenarios.shopping_list_show',
    'personal_assistant.scenarios.shopping_list_show__show',
    'personal_assistant.scenarios.shopping_list_show__add',
    'personal_assistant.scenarios.shopping_list_show__delete_all',
    'personal_assistant.scenarios.shopping_list_show__delete_index',
    'personal_assistant.scenarios.shopping_list_show__delete_item',
    'personal_assistant.scenarios.shopping_list_delete_item',
    'personal_assistant.scenarios.shopping_list_delete_all',
    'personal_assistant.scenarios.shopping_list_login',
    'personal_assistant.scenarios.shopping_list_add_fixlist',
    'personal_assistant.scenarios.shopping_list_show_fixlist',
    'personal_assistant.scenarios.shopping_list_show__show_fixlist',
    'personal_assistant.scenarios.shopping_list_show__add_fixlist',
    'personal_assistant.scenarios.shopping_list_show__delete_all_fixlist',
    'personal_assistant.scenarios.shopping_list_show__delete_index_fixlist',
    'personal_assistant.scenarios.shopping_list_show__delete_item_fixlist',
    'personal_assistant.scenarios.shopping_list_delete_item_fixlist',
    'personal_assistant.scenarios.shopping_list_delete_all_fixlist',
    'personal_assistant.scenarios.shopping_list_login_fixlist',
    'personal_assistant.scenarios.voiceprint_enroll',
    'personal_assistant.scenarios.voiceprint_enroll__collect_voice',
    'personal_assistant.scenarios.voiceprint_remove',
    'personal_assistant.scenarios.voiceprint_remove__confirm'
}


@attr.s
class AnalyticsInfo(object):
    scenario_analytics_info_data = attr.ib(default=None)


def can_not_be_pure(f):
    @wraps(f)
    @callback_decorator
    def inner(*args, **kwargs):
        req_info = kwargs['req_info']
        response = kwargs['response']

        if req_info.ensure_purity:
            response.add_meta(ErrorMeta(error_type='can_not_be_pure'))
            return

        return f(*args, **kwargs)

    return inner


def get_is_continuing_feature(session, is_resubmit):
    if session.form.is_continuing():
        return bool(
            not intents.is_how_much_intent(session.form.name) and
            not (intents.is_market_market_intent(session.form.name) and is_resubmit)
        )
    return bool(
        intents.is_gc_end(session.form.name) or
        session.get('pure_general_conversation') and not intents.is_gc_start(session.form.name)
    )


def _create_apply_payload(fname, form, req_info, feature, kwargs_dict):
    payload = {
        'form_update': BassFormInfo.from_form(form).to_dict(),
        'callback': {
            'name': fname,
            'arguments': {}
        }
    }

    if req_info.callback_name:
        if req_info.callback_args:
            payload['callback']['arguments'] = copy.copy(req_info.callback_args)
    else:
        callback_args = {
            k: v for k, v in kwargs_dict.iteritems() if k not in (
                'sample', 'form', 'session', 'req_info', 'response',
                'dm', 'sample_features', 'bass_result', 'precomputed_data',
            )
        }
        payload['callback']['arguments'] = callback_args
    sample = kwargs_dict.get('sample')
    if sample:
        payload['callback']['sample'] = sample.to_dict()
    payload['callback']['event'] = req_info.event.to_dict()

    payload['callback']['expects_request'] = feature.expects_request
    return payload


def callback_method(f):
    @wraps(f)
    @callback_decorator
    def inner(*args, **kwargs):
        form = kwargs['form']
        intent_name = form and form.name
        req_info = kwargs['req_info']
        response = kwargs['response']
        session = kwargs['session']
        always_irrelevant = kwargs.get('always_irrelevant', [])
        irrelevant_exp = kwargs.get('irrelevant_exp')
        bass_result_value = session.pop(bass_result_key, kwargs.get(bass_result_key, None))
        kwargs[bass_result_key] = bass_result_value

        eou = req_info.utterance and req_info.utterance.end_of_utterance
        if form:
            sensors.inc_counter('pa_hit_scenario', labels={'intent_name': get_intent_for_metrics(intent_name)})

        feature = FormInfoFeatures()
        if form:
            if intents.intent_has_irrelevant_name(form.name):
                feature.intent = form.intent_name.value
                feature.is_irrelevant = True
            else:
                feature.intent = form.name
        if req_info.callback_name != 'update_form' and session.form:
            feature.is_continuing = get_is_continuing_feature(session, bass_result_value is not None)

        always_irrelevant_key = 'quasar' if clients.is_smart_speaker(req_info.app_info) else 'other'
        always_irrelevant_value = always_irrelevant_key in always_irrelevant

        # special kind of irrelevant that should be used very sparingly;
        # it is intransitive, meaning that the intent will still work if you change the form,
        # including an update_form server action
        if req_info.event is not None and getattr(req_info.event, 'action_name', '') != 'update_form':
            if always_irrelevant_value or (irrelevant_exp is not None and req_info.experiments[irrelevant_exp] is not None):
                if always_irrelevant_value:
                    logger.info('The intent %s is intransitively irrelevant, see always_irrelevant in the config' % form.name)
                else:
                    logger.info('The intent %s is intransitively irrelevant due to the experiment %s' % (form.name, irrelevant_exp))
                feature.is_irrelevant = True

        feature.expects_request = any((
            feature.intent in intents_expects_request,
            session and session.get('pure_general_conversation'),
            feature.intent and intents.is_gc_start(feature.intent),
        ))

        # feature can be rewritten by successive callback
        response.set_feature(feature)

        if form:
            fname = f.__name__

            if req_info.ensure_purity:
                if intents.is_with_side_effect(form.name):
                    logger.info(
                        'Received pure request(%s) for %s intent with side effects. Apply needed.',
                        req_info.request_id,
                        form.name
                    )

                    logger.info('Saving apply arguments for %s callback', fname)
                    payload = _create_apply_payload(f.__name__, form, req_info, feature, kwargs)
                    response.directives.append(
                        MegamindActionDirective(name='apply', payload=payload)
                    )
                    response.set_apply_arguments(ApplyArguments(**payload))
                    return

                elif not bass_result_value and intents.can_use_continue_stage(form.name, req_info) and \
                        fname in ['general_conversation', 'universal_callback', 'phone_call_callback']:
                    logger.info('Deferring execution for continue stage for %s callback', fname)
                    payload = _create_apply_payload(fname, form, req_info, feature, kwargs)
                    response.set_continue_arguments(ApplyArguments(**payload))
                    return

            if eou is False:
                response.add_meta(ErrorMeta(error_type='eou_expected'))
                return

        if form and eou is False and intents.should_cancel_listening(form.name):
            response.add_meta(
                CancelListening(form_name=form.name)
            )

        analytics_info = AnalyticsInfo()
        with sensors.labels_context({
            'intent_name': get_intent_for_metrics(intent_name),
        }):
            with sensors.timer('pa_callback_time'):
                if kwargs.get('analytics_info'):
                    analytics_info = kwargs.pop('analytics_info')
                result = f(*args, analytics_info=analytics_info, **kwargs)

        _intent = None
        _form = None
        _raw_intent = None
        _product_scenario_name = None

        if session.form:
            # Do not fill analytics info for overridden form
            if session.form.is_overridden and response.analytics_info_meta:
                return result
            _intent = session.form.name
            _form = session.form.to_dict(truncate=True)
            if intents.intent_has_irrelevant_name(_intent):
                _raw_intent = session.form.intent_name.value
                _product_scenario_name = intents.product_scenario_name(req_info)
            else:
                _raw_intent = _intent

        _scenario_analytics_info = AnalyticsInfoMeta.parse_scenario_analytics_info(
            analytics_info.scenario_analytics_info_data
        )

        if _intent or _form or _scenario_analytics_info:
            if not _product_scenario_name:
                if not _raw_intent and _scenario_analytics_info and _scenario_analytics_info.Intent:
                    _raw_intent = _scenario_analytics_info.Intent

                if not _raw_intent:
                    logger.error('Raw intent is None. Intent = {}'.format(_intent))

                _product_scenario_name = analytics.get_product_scenario_name(_raw_intent) if _raw_intent else None

            response.set_analytics_info(
                intent=_intent,
                form=_form,
                scenario_analytics_info=_scenario_analytics_info,
                product_scenario_name=_product_scenario_name,
            )

        return result

    return inner
