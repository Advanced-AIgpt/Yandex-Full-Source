# coding: utf-8

import calendar
import getpass
import os
import socket

from copy import copy, deepcopy
from datetime import datetime as base_datetime
from deepmerge import always_merger as dict_merger
from uuid import uuid4

import pytz

from alice.acceptance.modules.request_generator.lib import app_presets
from alice.acceptance.modules.request_generator.lib import helpers
from alice.acceptance.modules.request_generator.lib import state_preset


FQDN = os.getenv('TEST_FQDN', socket.getfqdn())
LOGNAME = os.getenv('TEST_LOGNAME', getpass.getuser())

VINS_MAPPER_KWARGS_DEFAULTS = {
    'process_id': None,
    'oauth_token': None,
    'experiments': None,
    'additional_options': None,
    'downloader_flags': None,
    'websearch_cache_mode': 0,
    'filter_experiments': [],
    # TODO(@ran1s) delete
    'uniproxy_url': None,
}
YANDEX_LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}
DEFAULT_TIMEZONE = 'Europe/Moscow'
TIMESTRING_FORMAT = '%Y%m%dT%H%M%S'
DEFAULT_DEVICE_STATE = {}
DEFAULT_EXPERIMENTS = {
    'analytics_info': True,
    'disable_related_facts_promo': True,
    'enable_multiple_hypotheses_to_client': True,
    'gc_random_salt_0': True,
    'hw_gc_disable_gif_show': True,
    'mm_dont_call_scenario_commit': True,
    'music_force_show_first_track': True,
    'only_100_percent_flags': True,
    'skillrec_disable': True,
    'stateless_uniproxy_session': True,
    'websearch_bass_cgi_exp_flags=advGoodwinBass': 0,
    'websearch_bass_music_cgi_waitall=da': True,
    'websearch_bass_music_cgi_timeout=10000000': True,
    'websearch_cgi_notests=da': True,
}
DEFAULT_VINS_EXPERIMENTS = {
    'mm_dont_defer_apply': True,
}
DEFAULT_WEBSEARCH_CACHE_MODE_EXPERIMENTS = {
    'websearch_report_cache_flags=priemka,disable-lookup': True,
}
WEBSEARCH_CACHE_MODE_EXPERIMENTS_MAP = {
    # CACHE_PUT
    0: DEFAULT_WEBSEARCH_CACHE_MODE_EXPERIMENTS,
    # CACHE_LOOKUP
    1: {
        'websearch_report_cache_flags=priemka,disable-put': True,
    },
}

VINS_SCHEMA = helpers.get_schema([
    ('request', 'any'),
    ('request_id', 'string'),
])


def get_utc_now(client_time_str):
    if client_time_str:
        return base_datetime.strptime(client_time_str, TIMESTRING_FORMAT).replace(tzinfo=pytz.utc)
    return base_datetime.now(tz=pytz.utc)


def datetime_to_timestamp(dt):
    if dt.tzinfo:
        # convert to utc
        dt = dt.astimezone(pytz.UTC)

    # else expect naive datetime in utc
    return calendar.timegm(dt.timetuple())


def gen_uuid_for_tests():
    base = str(uuid4())
    uuid_str = '-'.join(['deadbeef'] + base.split('-')[1:])
    return uuid_str


def gen_reqid_for_test():
    return '-'.join(
        ['ffffffff-ffff-ffff'] + str(uuid4()).split('-')[3:]
    )


def make_asr_result(text):
    return [
        {
            'utterance': text,
            'confidence': 1.0,
            'words': [
                {
                    'value': word,
                    'confidence': 1.0
                } for word in text.split()
            ]
        }
    ]


def make_event(text, text_source='text', biometry_scoring=None, asr_result=None, biometry_classification=None, asr_whisper=None):
    if text_source == 'text':
        return {'type': 'text_input', 'text': text}
    elif text_source == 'voice_input':
        result = {
            'type': 'voice_input',
            'asr_result': asr_result if asr_result else make_asr_result(text),
            'end_of_utterance': True,
        }
        if biometry_scoring:
            result['biometry_scoring'] = biometry_scoring
        if biometry_classification:
            result['biometry_classification'] = biometry_classification
        if asr_whisper:
            result['asr_whisper'] = asr_whisper
        return result
    else:
        raise ValueError('Unexpected text_source %s' % text_source)


def prepare_application(app):
    resp = {}
    for key, value in app.items():
        if value is None:
            continue
        resp[key] = str(value)
    return resp


def make_vins_request(
        request_id, text, event, lang,
        app=None,
        device_state=None,
        experiments=None,
        session=None,
        additional_options=None,
        uuid=None,
        process_id=None,
        oauth_token=None,
        location=None,
        client_time=None,
        timezone=None,
        personal_data=None,
        iot_user_info=None,
        memento=None,
        contacts=None,
        environment_state=None,
        guest_options=None,
        guest_data=None,
        notification_state=None,
        client_ip='77.88.55.77',
):

    if request_id is None:
        raise ValueError("Request id can't be None")
    if text is None and event is None:
        raise ValueError("Text or event can't be None")

    if process_id is None:
        process_id = 'unknown_process, user = {}, host = {}'.format(LOGNAME, FQDN)

    additional_options_default = {'bass_options': {
        'client_ip': client_ip,
        'process_id': process_id,
    }}
    if oauth_token is not None:
        additional_options_default['oauth_token'] = oauth_token
    if guest_options is not None:
        additional_options_default['guest_user_options'] = guest_options
    additional_options = dict_merger.merge(additional_options_default, additional_options or {})

    request = {
        'header': {
            'request_id': request_id,
        },
        'application': prepare_application(app),
        'request': {
            'event': event or make_event(text, 'voice_input'),
            'device_state': device_state or deepcopy(DEFAULT_DEVICE_STATE),
            'additional_options': additional_options,
            'experiments': experiments,
        },
    }

    if location is None:
        location = copy(YANDEX_LOCATION)
    if location:
        location['accuracy'] = 1
        request['request']['location'] = location

    if personal_data:
        request['request']['personal_data'] = personal_data

    if session is not None:
        request['session'] = session

    if iot_user_info is not None:
        request['iot_user_info_data'] = iot_user_info

    if memento is not None:
        request['memento'] = memento

    if contacts is not None:
        request['contacts'] = contacts

    if environment_state is not None:
        request['request']['environment_state'] = environment_state

    if guest_data is not None:
        request['guest_user_data'] = guest_data

    if notification_state is not None:
        request['request']['notification_state'] = notification_state

    curr_time = get_utc_now(client_time)
    request['application']['uuid'] = uuid or gen_uuid_for_tests()
    request['application']['lang'] = lang
    request['application']['client_time'] = curr_time.strftime(TIMESTRING_FORMAT)
    request['application']['timestamp'] = str(datetime_to_timestamp(curr_time))
    request['application']['timezone'] = timezone or DEFAULT_TIMEZONE
    return request


def get_text_from_row(row):
    return row.get('megamind_request_text') or row.get('text', '')


def get_vins_request(row, **kwargs):
    params = {k: kwargs.get(k, VINS_MAPPER_KWARGS_DEFAULTS[k]) for k in VINS_MAPPER_KWARGS_DEFAULTS.keys()}
    downloader_flags = set(params['downloader_flags'] or {})
    experiments_0 = params['experiments'] or {}
    experiments = DEFAULT_EXPERIMENTS.copy()
    # https://st.yandex-team.ru/ADI-412 these flags work incorrectly on prod environment
    if params['uniproxy_url'] and 'kpi' in params['uniproxy_url']:
        experiments.pop('websearch_bass_music_cgi_waitall=da')
        experiments.pop('websearch_bass_music_cgi_timeout=10000000')
    experiments.update(experiments_0)
    experiments.update(row.get('experiments') or {})
    if 'use_websearch_cache_experiments' in downloader_flags:
        experiments.update(WEBSEARCH_CACHE_MODE_EXPERIMENTS_MAP.get(
            params['websearch_cache_mode'],
            DEFAULT_WEBSEARCH_CACHE_MODE_EXPERIMENTS
        ))

    experiments = helpers.filter_experiments(experiments, set(params['filter_experiments']))

    app = (
        row.get('app', None) or
        app_presets.get_preset_attr(row.get('app_preset'), 'application') or
        app_presets.DEFAULT_APP.application
    )
    user_agent = app_presets.get_preset_attr(row.get('app_preset'), 'user_agent') or app_presets.DEFAULT_APP.user_agent
    supported_features = app_presets.get_preset_attr(row.get('app_preset'), 'supported_features')
    unsupported_features = app_presets.get_preset_attr(row.get('app_preset'), 'unsupported_features')
    preset_additional_options = {'bass_options': {'user_agent': user_agent}}
    if supported_features is not None:
        preset_additional_options['supported_features'] = supported_features
    if unsupported_features is not None:
        preset_additional_options['unsupported_features'] = unsupported_features
    additional_options_0 = dict_merger.merge(
        row.get('additional_options') or {},
        preset_additional_options
    )
    additional_options_1 = dict_merger.merge(additional_options_0, params['additional_options'] or {})
    permissions = row.get('permissions')
    if permissions:
        additional_options_1['permissions'] = permissions

    if row.get('device_id'):
        app['device_id'] = row['device_id']

    if additional_options_0.get('oauth_token'):
        raise ValueError('Insecurity oauth token storage, request_id %s' % row['request_id'])
    oauth_token_1 = row.get('oauth') or params['oauth_token']

    preset_capabilities = app_presets.get_preset_attr(row.get('app_preset'), 'capabilities')
    preset_environment_state = {
        "endpoints": [{
            "id": app['device_id'],
            "capabilities": preset_capabilities
        }]
    } if preset_capabilities else None

    device_state = row.get('device_state')

    state_preset_name = row.get('state_preset')
    app_preset_name = row.get('app_preset')
    if state_preset_name and app_preset_name:
        # try to set environment_state from state_preset
        evn_from_state_preset = state_preset.get_preset_attr(state_preset_name, app_preset_name, 'environment_state')
        if preset_environment_state is None and evn_from_state_preset is not None:
            preset_environment_state = evn_from_state_preset

        # try to set device_state from state_preset
        ds_from_state_preset = state_preset.get_preset_attr(state_preset_name, app_preset_name, 'device_state')
        if device_state is None and ds_from_state_preset is not None:
            device_state = ds_from_state_preset

    req_body = make_vins_request(
        row.get('request_id') or gen_reqid_for_test(),
        get_text_from_row(row),
        row.get('event'),
        lang=row.get('lang') or 'ru-RU',
        app=app,
        device_state=device_state,
        experiments=experiments,
        session=row.get('session'),
        additional_options=additional_options_1,
        uuid=row.get('uuid'),
        process_id=params['process_id'],
        oauth_token=oauth_token_1,
        location=row.get('location'),
        client_time=row.get('client_time'),
        timezone=row.get('timezone'),
        environment_state=preset_environment_state
    )
    return {'request': req_body, 'request_id': req_body['header']['request_id']}


def mapper(row, **kwargs):
    experiments_1 = DEFAULT_VINS_EXPERIMENTS.copy()
    experiments_1.update(kwargs.get('experiments') or {})
    kwargs['experiments'] = experiments_1
    row = helpers.decode_value(row)
    yield helpers.encode_value(get_vins_request(row, **kwargs))
