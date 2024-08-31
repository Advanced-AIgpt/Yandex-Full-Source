# coding: utf-8

import base64
import json
import uuid as uuid_module

import attr

from google.protobuf import json_format
from alice.megamind.protos.common import iot_pb2

from alice.acceptance.modules.request_generator.lib import app_presets
from alice.acceptance.modules.request_generator.lib import helpers
from alice.acceptance.modules.request_generator.lib import vins
from alice.acceptance.modules.request_generator.lib import retries_profiles


UNIPROXY_MAPPER_KWARGS_DEFAULTS = {
    'process_id': None,
    'oauth_token': None,
    'experiments': None,
    'fetcher_mode': 'voice',
    'retry_profile': None,
    'additional_options': None,
    'deep_mode': False,
    'downloader_flags': None,
    'propagate_experiments_into_context': False,
    'filter_experiments': [],
    # TODO(@ran1s) delete
    'uniproxy_url': None,
}
UNIPROXY_SCHEMA = helpers.get_schema([
    ('VoiceData', 'string'),
    ('Uuid', 'string'),
    ('Topic', 'string'),
    ('Timezone', 'string'),
    ('RequestId', 'string'),
    ('Application', 'string'),
    ('Payload', 'string'),
    ('Format', 'string'),
    ('Lang', 'string'),
    ('AuthToken', 'string'),
    ('OAuthToken', 'string'),
    ('Text', 'string'),
    ('FetcherMode', 'string'),
    ('SyncStateExperiments', 'string'),
    ('ClientTime', 'string'),
    ('DisableLocalExperiments', 'boolean'),
    ('SessionId', 'string'),
    ('SessionSequence', 'int64'),
    ('Headers', 'string'),
    ('SetraceUrl', 'string'),
])
DEFAULT_ADVANCED_ASR_OPTIONS = {
    'manual_punctuation': False,
    'partial_results': True,
}


@attr.s
class FetcherModeSettings:
    audio_format = attr.ib()
    event = attr.ib()
    fetcher_mode = attr.ib(validator=attr.validators.in_(('text', 'voice')))
    predefined_asr_result = attr.ib()
    predefined_bio_scoring_result = attr.ib()
    predefined_bio_classify_result = attr.ib()
    text = attr.ib()
    voice_data = attr.ib()
    voice_session = attr.ib()


class ForbidSpeechbaseError(Exception):
    pass


def get_voice_data(url):
    # NOTE: forbid downloading from speechbase
    # for stimulate using voice_binary column
    raise ForbidSpeechbaseError('Please use "voice_binary" column in your basket. https://nda.ya.ru/t/XuRNcgyR3VwqhA')


def get_voice_data_from_row(request_id, row):
    if row.get('voice_binary') is None and row.get('voice_base64_binary') is None and row.get('voice_url') is None:
        raise ValueError('Empty voice data in row %s: %r' % (request_id, row))
    voice_data = row.get('voice_binary')
    voice_data = voice_data or base64.decodebytes(row.get('voice_base64_binary', '').encode('ascii'))
    voice_data = voice_data or get_voice_data(row['voice_url'])
    return voice_data


def is_action_request_in_deep_mode(row, downloader_flags):
    if 'use_reversed_session_sequence' in downloader_flags:
        try:
            return row['reversed_session_sequence'] == 0
        except KeyError:
            return row.get('session_sequence', 0) == 0
    return row.get('session_sequence', 0) == 0


def is_context_request_in_deep_mode(row, downloader_flags):
    return not is_action_request_in_deep_mode(row, downloader_flags)


def get_fetcher_mode_setting_for_text(row, request_id, downloader_flags):
    text = vins.get_text_from_row(row).strip()
    if not text and 'handle_empty_text_ADD-39' not in downloader_flags:
        raise ValueError('Empty text in row %s: %r' % (request_id, row))

    return FetcherModeSettings(
        audio_format=None,
        event=vins.make_event(text, 'text') if text else None,
        fetcher_mode='text',
        predefined_asr_result=None,
        predefined_bio_scoring_result=None,
        predefined_bio_classify_result=None,
        text=text,
        voice_data=None,
        voice_session=False,
    )


def get_fetcher_mode_setting_for_voice(row, request_id, downloader_flags):
    if row.get('predefined_asr_result') is not None:
        audio_format = None
        voice_data = None
    else:
        audio_format = 'audio/x-pcm;bit=16;rate=16000' if 'audio_format_wav' in downloader_flags else 'audio/opus'
        voice_data = get_voice_data_from_row(request_id, row)

    if 'disable_tts' in downloader_flags:
        voice_session = False
    else:
        voice_session = True
    return FetcherModeSettings(
        audio_format=audio_format,
        event={'type': 'voice_input'},
        fetcher_mode='voice',
        predefined_asr_result=row.get('predefined_asr_result'),
        predefined_bio_scoring_result=row.get('predefined_bio_scoring_result'),
        predefined_bio_classify_result=row.get('predefined_bio_classify_result'),
        text=None,
        voice_data=voice_data,
        voice_session=voice_session,
    )


def get_fetcher_mode_settings(global_fetcher_mode, row, request_id, deep_mode, downloader_flags):
    assert global_fetcher_mode in ['text', 'voice', 'auto'], 'Unknown uniproxy fetcher mode %s' % global_fetcher_mode

    if deep_mode and is_context_request_in_deep_mode(row, downloader_flags):
        global_fetcher_mode = 'text'
    result_fetcher_mode = global_fetcher_mode
    if result_fetcher_mode == 'auto':
        result_fetcher_mode = row.get('fetcher_mode')

    if result_fetcher_mode == 'text':
        fm_settings = get_fetcher_mode_setting_for_text(row, request_id, downloader_flags)
    elif result_fetcher_mode == 'voice':
        fm_settings = get_fetcher_mode_setting_for_voice(row, request_id, downloader_flags)
    else:
        try:
            fm_settings = get_fetcher_mode_setting_for_voice(row, request_id, downloader_flags)
        except ValueError:
            fm_settings = get_fetcher_mode_setting_for_text(row, request_id, downloader_flags)
    return fm_settings


def prepare_uniproxy_payload(row, vins_request, fetcher_mode_settings, downloader_flags):
    request = vins_request['request'].copy()
    request['reset_session'] = False
    request['voice_session'] = fetcher_mode_settings.voice_session
    request['event'] = fetcher_mode_settings.event

    if fetcher_mode_settings.predefined_asr_result is not None:
        request['predefined_asr_result'] = fetcher_mode_settings.predefined_asr_result
    if fetcher_mode_settings.predefined_bio_classify_result is not None:
        request['predefined_bio_classify_result'] = fetcher_mode_settings.predefined_bio_classify_result
    if fetcher_mode_settings.predefined_bio_scoring_result is not None:
        request['predefined_bio_scoring_result'] = fetcher_mode_settings.predefined_bio_scoring_result

    if row.get('activation_type'):
        request['activation_type'] = row['activation_type']
    if row.get('contacts'):
        request['predefined_contacts'] = json.dumps(row['contacts'])
    if row.get('iot_config'):
        iot_config = row['iot_config']
        if isinstance(iot_config, dict):
            iot_config = json.dumps(iot_config)
        iot_config_proto = json_format.Parse(iot_config, iot_pb2.TIoTUserInfo())
        for device in iot_config_proto.Devices:
            device.SkillId = 'QUALITY'
        request['iot_config'] = base64.b64encode(iot_config_proto.SerializeToString()).decode('utf-8')

    # uniproxy gets session from request field
    session = vins_request.get('session')
    if session is not None:
        request['session'] = session

    asr_options = row.get('asr_options', {})
    if 'asr_always_fill_response' in downloader_flags:
        asr_options['always_fill_response'] = True

    new_asr_options = DEFAULT_ADVANCED_ASR_OPTIONS.copy()
    new_asr_options.update(asr_options)
    return {
        'header': vins_request.get('header'),
        'advancedASROptions': new_asr_options,
        'punctuation': True,
        'application': vins_request['application'],
        'request': request,
        'disableAntimatNormalizer': True,
    }


def mapper(row, **kwargs):
    voice_binary_data = row.pop(b'voice_binary', None)
    row = helpers.decode_value(row)
    row['voice_binary'] = voice_binary_data
    uniproxy_row = next(mapper_no_coding(
        row, **kwargs
    ))
    yield helpers.encode_value(uniproxy_row)


def mapper_no_coding(row, **kwargs):
    params = {k: kwargs.get(k, UNIPROXY_MAPPER_KWARGS_DEFAULTS[k]) for k in UNIPROXY_MAPPER_KWARGS_DEFAULTS.keys()}
    downloader_flags = set(params['downloader_flags'] or {})

    # ADI-320
    if (params['deep_mode'] and is_context_request_in_deep_mode(row, downloader_flags) and
            not params['propagate_experiments_into_context']):
        params.pop('experiments')

    raw_data = vins.get_vins_request(row, **params)
    request_id = raw_data['request_id']
    vins_request = raw_data['request']
    application = vins_request['application']
    experiments = vins_request['request']['experiments']
    syncstate_experiments = helpers.filter_experiments(
        vins.DEFAULT_EXPERIMENTS if params['deep_mode'] else experiments, set(params['filter_experiments']))

    if params['deep_mode'] and is_context_request_in_deep_mode(row, downloader_flags):
        experiments['uniproxy_vins_sessions'] = True
    if not params['deep_mode'] or is_action_request_in_deep_mode(row, downloader_flags):
        retry_profile = params['retry_profile'] or row.get('retry_profile')
        experiments.update(retries_profiles.get_profile_by_name_or_default(retry_profile))
    if 'silent_vins' in downloader_flags:
        experiments['silent_vins'] = True

    fm_settings = get_fetcher_mode_settings(params['fetcher_mode'], row, request_id, params['deep_mode'],
                                            downloader_flags)
    payload = prepare_uniproxy_payload(row, vins_request, fm_settings, downloader_flags)
    payload = json.dumps(payload, sort_keys=True)
    # DIALOG-6698 escape payload like java serializer escapes in quasar
    if row.get('app_preset') in app_presets.STATIONS:
        user_agent = vins_request['request']['additional_options']['bass_options']['user_agent']
        user_agent2 = user_agent.replace('/', r'\\/')
        payload = payload.replace(user_agent, user_agent2).replace('/', r'\/')

    rtlog_token = row.get('x-rtlog-token') or str(uuid_module.uuid4())
    headers = {
        'x-rtlog-token': rtlog_token,
    }
    headers = json.dumps(headers, sort_keys=True)
    uniproxy_row = {
        'Uuid': application['uuid'],
        'Topic': row.get('topic') or app_presets.get_preset_attr(row.get('app_preset'), 'asr_topic'),
        'Timezone': application['timezone'],
        'RequestId': request_id,
        'Application': json.dumps(application, sort_keys=True),
        'Payload': payload,
        'Format': fm_settings.audio_format,
        'Lang': row.get('lang', 'ru-RU'),
        'AuthToken': row.get('auth_token') or app_presets.get_preset_attr(row.get('app_preset'), 'auth_token'),
        'SyncStateExperiments': json.dumps(sorted(syncstate_experiments)),
        'DisableLocalExperiments': False,
        'ClientTime': application['client_time'] if row.get('client_time') else None,  # for gen in Uniproxy downloader
        'SessionId': row.get('session_id', ''),
        'SessionSequence': row.get('session_sequence', 0),
        'ReversedSessionSequence': row.get('reversed_session_sequence', 0),
        'Headers': headers,
        'SetraceUrl': 'https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=%s' % rtlog_token,
        'Text': fm_settings.text,
        'VoiceData': fm_settings.voice_data,
        'FetcherMode': fm_settings.fetcher_mode,
    }
    oauth_token = vins_request['request']['additional_options'].get('oauth_token', None)
    if oauth_token is not None:
        uniproxy_row['OAuthToken'] = oauth_token
    yield uniproxy_row
