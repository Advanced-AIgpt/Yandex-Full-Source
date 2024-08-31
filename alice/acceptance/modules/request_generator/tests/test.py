# coding: utf-8

import json
import random
import uuid
from datetime import datetime, timedelta

import pytest
import pytz
from freezegun import freeze_time

from alice.acceptance.modules.request_generator.lib import app_presets
from alice.acceptance.modules.request_generator.lib import helpers
from alice.acceptance.modules.request_generator.lib import uniproxy
from alice.acceptance.modules.request_generator.lib import vins
from alice.acceptance.modules.request_generator.lib import retries_profiles


DEFAULT_APP = app_presets.DEFAULT_APP.application
DEFAULT_ROW = dict(
    request_id='ffffffff-ffff-ffff-f04b-89b0e74434d0',
    text='test',
    app=DEFAULT_APP,
)


@pytest.fixture(scope='function')
def default_app():
    return app_presets.DEFAULT_APP.application


@pytest.fixture(scope='function')
def default_row(default_app):
    return dict(
        request_id='ffffffff-ffff-ffff-f04b-89b0e74434d0',
        text='test',
        app=default_app,
    )


def uuid4_mock():
    return 'ffffffff-ffff-ffff-ffff-ffffffffffff'


@pytest.fixture(autouse=True)
def mock_fixture(monkeypatch):
    monkeypatch.setattr(uuid, 'uuid4', uuid4_mock)


@pytest.mark.parametrize('data, expected', [
    (DEFAULT_APP, DEFAULT_APP),
    ({'a': None}, {}),
    (dict(DEFAULT_APP, a=None), DEFAULT_APP),
])
def test_app_preparation(data, expected):
    assert vins.prepare_application(data) == expected


@pytest.mark.parametrize('data, expected', [
    ('', {}),
    ('gc_random_seed=0', {'gc_random_seed': '0'}),
    ('gc_random_seed=0,gc_random_seed=1', {'gc_random_seed': '1'}),
    ('gc_random_seed=0, awesome=tttt,,,', {'gc_random_seed': '0', 'awesome': 'tttt'}),
    ('mm_scenario=1=true', {'mm_scenario=1': 'true'}),
])
def test_parse_experiments_from_options(data, expected):
    assert helpers.parse_experiments_from_options(data) == expected


@pytest.fixture(autouse=True)
def mock_uuid4(mocker):
    yield mocker.patch(
        'alice.acceptance.modules.request_generator.lib.vins.uuid4',
        return_value='ffa7d412-cddf-465a-8d11-a2ebddb9aab7'
    )


@freeze_time('2019-07-08 13:07:54')
@pytest.mark.parametrize('experiments', [None, {}, {'gc_random_seed': '0'}])
@pytest.mark.parametrize('additional_options', [None, {}, {'nlu_tags': ['tags']}])
def test_canonical_response_vins(experiments, additional_options):
    row = DEFAULT_ROW.copy()
    row['additional_options'] = additional_options
    return next(
        vins.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments=experiments,
        )
    )


def test_device_id_override():
    row = DEFAULT_ROW.copy()
    row['device_id'] = '12345'
    request = next(vins.mapper(row))
    assert request[b'request'][b'application'][b'device_id'] == b'12345'

    row = DEFAULT_ROW.copy()
    request = next(vins.mapper(row))
    assert request[b'request'][b'application'][b'device_id'].decode('utf-8') == DEFAULT_APP['device_id']


def test_request_with_event():
    row = DEFAULT_ROW.copy()
    row['event'] = {'name': 'name', 'type': 'type'}
    request = next(vins.mapper(row))
    assert request[b'request'][b'request'][b'event'] == {b'name': b'name', b'type': b'type'}


@freeze_time('2019-07-08 13:31:50')
@pytest.mark.parametrize('experiments', [None, {}, {'gc_random_seed': '0'}])
@pytest.mark.parametrize('fetcher_mode', ['text', 'voice', 'auto'])
def test_canonical_response_uniproxy(experiments, fetcher_mode, mocker):
    mocker.patch(
        'alice.acceptance.modules.request_generator.lib.uniproxy.get_voice_data',
        return_value=b'sound binary data'
    )

    row = DEFAULT_ROW.copy()
    row['topic'] = 'quasar-general'
    row['voice_url'] = 'http://example.com/voice.opus'

    return next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments=experiments,
            fetcher_mode=fetcher_mode,
        )
    )


@freeze_time('2019-07-08 13:31:50')
@pytest.mark.parametrize('app_preset', ['quasar', 'browser_prod'])
@pytest.mark.parametrize('experiments', [None, {}, {'gc_random_seed': '0'}])
@pytest.mark.parametrize('fetcher_mode', ['text', 'voice', 'auto'])
def test_uniproxy_response_with_app_preset(app_preset, experiments, fetcher_mode, mocker):
    mocker.patch(
        'alice.acceptance.modules.request_generator.lib.uniproxy.get_voice_data',
        return_value=b'sound binary data'
    )
    row = DEFAULT_ROW.copy()
    del row['app']
    row['app_preset'] = app_preset
    row['voice_url'] = 'http://example.com/voice.opus'
    return next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments=experiments,
            fetcher_mode=fetcher_mode,
        )
    )


@freeze_time('2019-07-08 13:31:50')
def test_uniproxy_text_mode():
    return next(
        uniproxy.mapper(
            DEFAULT_ROW,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )


@freeze_time('2019-07-08 13:31:50')
def test_uniproxy_text_mode_russian_text():
    row = DEFAULT_ROW.copy()
    row['text'] = 'русский текст'

    return next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )


@freeze_time('2019-07-08 13:31:50')
def test_app_with_app_preset():
    row = DEFAULT_ROW.copy()
    row['app_preset'] = 'browser_beta'
    return next(vins.mapper(row))


@freeze_time('2019-07-08 13:31:50')
def test_app_preset_only():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    return next(vins.mapper(row))


def test_session_is_on_top():
    row = DEFAULT_ROW.copy()
    row['session'] = 'abcd'
    assert vins.get_vins_request(row)['request']['session'] == 'abcd'


def test_there_are_permissions():
    permissions = [
        {
            "name": "read_contacts",
            "granted": True
        },
        {
            "name": "call_phone",
            "granted": True
        }
    ]

    row = DEFAULT_ROW.copy()
    row['permissions'] = permissions
    assert vins.get_vins_request(row)['request']['request']['additional_options']['permissions'] == permissions

    res = next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )

    assert json.loads(res[b'Payload'])['request']['additional_options']['permissions'] == permissions


def test_empty_permissions():
    row = DEFAULT_ROW.copy()
    row['permissions'] = []
    assert 'permissions' not in vins.get_vins_request(row)['request']['request']['additional_options']

    res = next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )

    assert 'permissions' not in json.loads(res[b'Payload'])['request']['additional_options']


def test_session_inside_request_for_uniproxy():
    row = DEFAULT_ROW.copy()
    row['session'] = 'abcd'
    res = next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )
    assert json.loads(res[b'Payload'])['request']['session'] == 'abcd'


def test_uniproxy_with_2_oauth_tokens():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    row['oauth'] = 'test_token_1'
    res = next(uniproxy.mapper(row, oauth_token='test_token_3', fetcher_mode='text'))
    payload = json.loads(res[b'Payload'])
    assert payload['request']['additional_options']['oauth_token'] == 'test_token_1'
    assert res[b'OAuthToken'] == b'test_token_1'


def test_uniproxy_with_insecurity_token_storage():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    additional_options = row.get('additional_options', {})
    additional_options['oauth_token'] = 'test_token_2'
    row['additional_options'] = additional_options
    with pytest.raises(ValueError):
        next(uniproxy.mapper(row, oauth_token='test_token_3', fetcher_mode='text'))


def test_uniproxy_with_1_oauth_token():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    res = next(uniproxy.mapper(row, oauth_token='test_token_3', fetcher_mode='text'))
    payload = json.loads(res[b'Payload'])
    assert payload['request']['additional_options']['oauth_token'] == 'test_token_3'
    assert res[b'OAuthToken'] == b'test_token_3'


def test_uniproxy_with_0_oauth_tokens():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    res = next(uniproxy.mapper(row, fetcher_mode='text'))
    payload = json.loads(res[b'Payload'])
    assert 'oauth_token' not in payload['request']['additional_options']
    assert b'OAuthToken' not in res


def test_app_with_2_oauth_tokens():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    row['oauth'] = 'test_token_1'
    res = next(vins.mapper(row, oauth_token='test_token_3'))
    assert res[b'request'][b'request'][b'additional_options'][b'oauth_token'] == b'test_token_1'


def test_app_with_insecurity_token_storage():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    additional_options = row.get('additional_options', {})
    additional_options['oauth_token'] = 'test_token_2'
    row['additional_options'] = additional_options
    with pytest.raises(ValueError):
        next(vins.mapper(row, token='test_token_3'))


def test_app_with_1_oauth_token():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    res = next(vins.mapper(row, oauth_token='test_token_3'))
    assert res[b'request'][b'request'][b'additional_options'][b'oauth_token'] == b'test_token_3'


def test_app_with_0_oauth_tokens():
    row = DEFAULT_ROW.copy()
    row['app'] = None
    row['app_preset'] = 'browser_beta'
    res = next(vins.mapper(row))
    assert b'oauth_token' not in res[b'request'][b'request'][b'additional_options']


@pytest.mark.parametrize('row_exp, option_exp, expected', [
    ({}, {}, []),
    ({'a': 1}, {}, ['a']),
    ({}, {'a': 1}, ['a']),
    ({'a': 1}, {'a': 2}, ['a']),
    ({'a': 1}, {'b': 1}, ['a', 'b']),
])
def test_uniproxy_sync_state_have_all_experiments(row_exp, option_exp, expected):
    row = DEFAULT_ROW.copy()
    row['experiments'] = row_exp
    res = next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            experiments=option_exp,
            fetcher_mode='text'
        )
    )
    assert (
        set(json.loads(res[b'SyncStateExperiments'])) - set(vins.DEFAULT_EXPERIMENTS)
    ) == set(expected)


def test_uniproxy_asr_options():
    row = DEFAULT_ROW.copy()
    row['asr_options'] = {'allow_multi_utt': False}
    res = next(
        uniproxy.mapper(
            row,
            process_id='test_process',
            oauth_token='test_token',
            fetcher_mode='text'
        )
    )
    result_asr_options = json.loads(res[b'Payload'])['advancedASROptions']
    assert result_asr_options == {
        'allow_multi_utt': False,
        'manual_punctuation': False,
        'partial_results': True,
    }


def test_client_time():
    row = DEFAULT_ROW.copy()
    res = next(uniproxy.mapper(row, fetcher_mode='text'))
    assert res[b'ClientTime'] is None

    gen_dt = datetime.now(tz=pytz.utc) - timedelta(hours=random.randint(1, 10))
    row['client_time'] = gen_dt.strftime('%Y%m%dT%H%M%S')
    res = next(uniproxy.mapper(row, fetcher_mode='text'))
    assert res[b'ClientTime'].decode('utf-8') == row['client_time']


@pytest.mark.parametrize('global_mode, row, expected, flags', [
    ('text', {'text': 'test'}, (None, {'type': 'text_input', 'text': 'test'}, 'text', None, None, None,  'test', None, False), []),
    ('voice', {'voice_binary': 'aaa'}, ('audio/opus', {'type': 'voice_input'}, 'voice', None, None, None, None, 'aaa', True), []),

    ('auto', {'fetcher_mode': 'text', 'text': 'test'}, (None, {'type': 'text_input', 'text': 'test'}, 'text', None, None, None, 'test', None, False), []),
    ('auto', {'fetcher_mode': 'voice', 'voice_binary': 'aaa'}, ('audio/opus', {'type': 'voice_input'}, 'voice', None, None, None, None, 'aaa', True), []),

    ('auto', {'text': 'test'}, (None, {'type': 'text_input', 'text': 'test'}, 'text', None, None, None, 'test', None, False), []),
    ('auto', {'voice_binary': 'aaa'}, ('audio/opus', {'type': 'voice_input'}, 'voice', None, None, None,  None, 'aaa', True), []),

    ('auto', {'voice_binary': 'aaa'}, ('audio/x-pcm;bit=16;rate=16000', {'type': 'voice_input'}, 'voice', None, None, None,  None, 'aaa', True), ['audio_format_wav']),
])
def test_fetcher_mode_settings(global_mode, row, expected, flags):
    actual_fm_settings = uniproxy.get_fetcher_mode_settings(global_mode, row, 'request_id', False, flags)
    assert actual_fm_settings == uniproxy.FetcherModeSettings(*expected)


def test_retry_profile_from_basket():
    row = DEFAULT_ROW.copy()
    # set default music + video profile for row
    row['retry_profile'] = 'music_video_search'
    request = next(uniproxy.mapper(row, oauth_token='token', fetcher_mode='text'))
    payload = json.loads(request[b'Payload'])
    experiments = payload['request']['experiments']
    assert all([experiments.get(k) == v for k, v in retries_profiles.MUSIC_VIDEO_SEARCH.items()])


def test_retry_profile_from_option_priority():
    row = DEFAULT_ROW.copy()
    # set default music + video profile for row
    row['retry_profile'] = 'music_video_search'
    request = next(uniproxy.mapper(row, oauth_token='token', fetcher_mode='text', retry_profile='no_retry'))
    payload = json.loads(request[b'Payload'])
    experiments = payload['request']['experiments']
    assert not any([experiments.get(k) == v for k, v in retries_profiles.MUSIC_VIDEO_SEARCH.items()])


def test_external_iot_config():
    row = DEFAULT_ROW.copy()
    row['iot_config'] = {'devices': [{'id': '123456', 'name': '123456'}]}
    request = next(uniproxy.mapper(row, oauth_token='token', fetcher_mode='text', retry_profile='no_retry'))
    payload = json.loads(request[b'Payload'])
    iot_config = payload['request'].get('iot_config')
    expected = 'GhkKBjEyMzQ1NhIGMTIzNDU2QgdRVUFMSVRZ'
    assert iot_config == expected, "Invalid external iot config in request [%r] expected [%r]" % (
        iot_config, expected
    )


def test_filter_experiments_get_vins_request(default_row):
    assert 'only_100_percent_flags' not in vins.get_vins_request(
        default_row, filter_experiments=['only_100_percent_flags'])['request']['request']['experiments']


def test_not_filter_experiments_get_vins_request(default_row):
    assert 'only_100_percent_flags' in vins.get_vins_request(
        default_row)['request']['request']['experiments']


def test_filter_experiments_uniproxy_mapper(default_row):
    res = next(
        uniproxy.mapper(
            default_row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text',
            filter_experiments=['only_100_percent_flags']
        )
    )

    assert 'only_100_percent_flags' not in json.loads(
        res[b'Payload'])['request']['experiments']

    assert 'only_100_percent_flags' not in json.loads(
        res[b'SyncStateExperiments'])


def test_not_filter_experiments_uniproxy_mapper(default_row):
    res = next(
        uniproxy.mapper(
            default_row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={},
            fetcher_mode='text'
        )
    )

    assert 'only_100_percent_flags' in json.loads(
        res[b'Payload'])['request']['experiments']

    assert 'only_100_percent_flags' in json.loads(res[b'SyncStateExperiments'])


def test_filter_kwargs_experiments_uniproxy_mapper(default_row):
    res = next(
        uniproxy.mapper(
            default_row,
            process_id='test_process',
            oauth_token='test_token',
            experiments={'lol': 'kek'},
            fetcher_mode='text',
            filter_experiments=['lol']
        )
    )

    assert 'lol' not in json.loads(res[b'Payload'])['request']['experiments']
    assert 'lol' not in json.loads(res[b'SyncStateExperiments'])

    assert 'only_100_percent_flags' in json.loads(
        res[b'Payload'])['request']['experiments']

    assert 'only_100_percent_flags' in json.loads(res[b'SyncStateExperiments'])


def test_filter_experiments_dict():
    assert {'kek': 'lol'} == helpers.filter_experiments(
        {'lol': 'kek', 'kek': 'lol'}, {'lol'})


def test_filter_experiments_list():
    assert ['kek'] == helpers.filter_experiments(['lol', 'kek'], {'lol'})


def test_filter_experiments_neither_dict_or_list():
    assert 123 == helpers.filter_experiments(123, {'lol'})


def test_waitall_present_experiments_uniproxy_mapper(default_row):
    res = next(
        uniproxy.mapper(
            default_row,
            process_id='test_process',
            oauth_token='test_token',
            fetcher_mode='text'
        )
    )

    assert 'websearch_bass_music_cgi_waitall=da' in json.loads(res[b'Payload'])[
        'request']['experiments']
    assert 'websearch_bass_music_cgi_timeout=10000000' in json.loads(
        res[b'Payload'])['request']['experiments']


def test_waitall_absent_experiments_uniproxy_mapper(default_row):
    res = next(
        uniproxy.mapper(
            default_row,
            process_id='test_process',
            oauth_token='test_token',
            fetcher_mode='text',
            uniproxy_url='1337kpilolkek'
        )
    )

    assert 'websearch_bass_music_cgi_waitall=da' not in json.loads(res[b'Payload'])[
        'request']['experiments']
    assert 'websearch_bass_music_cgi_timeout=10000000' not in json.loads(
        res[b'Payload'])['request']['experiments']


def test_event():
    row = {
        'additional_options': {
            'bass_options': {
                'client_ip': '194.4.164.174'
            }
        },
        'app_preset': 'tv',
        'asr_options': {
            'allow_multi_utt': False
        },
        'asr_text': 'кот',
        'basket': 'input_basket',
        'client_time': '20201202T005842',
        'counter': 1,
        'device_state': {
            'device_config': {
                'content_settings': 'medium'
            },
            'is_tv_plugged_in': True,
            'sound_level': 13,
            'tv_set': {

            }
        },
        'event': {
            'name': '@@mm_semantic_frame',
            'payload': {
                'analytics': {
                    'origin': 'Scenario',
                    'purpose': 'video_play'
                },
                'typed_semantic_frame': {
                    'video_play_semantic_frame': {
                        'search_text': {
                            'string_value': 'такси три'
                        }
                    }
                }
            },
            'type': 'server_action'
        },
        'experiments': None,
        'fetcher_mode': 'auto',
        'location': None,
        'mds_key': '4246223/adf11e53-7878-497b-a2da-f77669d81f97_d83b9eff-b2d5-4c67-8dc1-6ace031fb2ac_1.opus',
        'query_type': 'film_narrow',
        'real_reqid': '78b9a357-edba-414c-8ff2-b4f1c1326eaa',
        'real_session_id': 'uu/0502d02b554b46518464be6504ccc9ca_1606870722_0',
        'real_uuid': 'uu/0502d02b554b46518464be6504ccc9ca',
        'request_id': 'ffffffff-ffff-ffff-0008-350e0f981d9e',
        'request_source': None,
        'reversed_session_sequence': 0,
        'scenario_type': 'FILM_CARD',
        'session_id': 'ffffffff-ffff-ffff-0008-350e0f981d9e',
        'session_sequence': 0,
        'text': 'кот и мышь',
        'timezone': 'Europe/Moscow',
        'toloka_intent': 'other',
        'vins_intent': 'mm\\tpersonal_assistant\\tscenarios\\tvideo_play',
        # Because of https://a.yandex-team.ru/svn/trunk/arcadia/alice/acceptance/modules/request_generator/lib/uniproxy.py?rev=r9399191#L217
        b'voice_binary': 'lolkek',
        'voice_url': 'https://speechbase-yt.voicetech.yandex.net/getfile/4246223/adf11e53-7878-497b-a2da-f77669d81f97_d83b9eff-b2d5-4c67-8dc1-6ace031fb2ac_1.opus'
    }
    res = next(
        uniproxy.mapper(
            row,
            vins_url='http://vins.hamster.alice.yandex.net/speechkit/app/pa/',
            process_id='test_process',
            uniproxy_url='wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws',
            retry_profile='music_video_search',
            downloader_flags=["soy_decrypt_secrets", "silent_vins", "handle_empty_text_ADD-39", "use_websearch_cache_experiments",
                              "use_reversed_session_sequence", "asr_always_fill_response", "async_uniproxy_client"],
            oauth_token='test_token',
            experiments={},
            propagate_experiments_into_context=False,
            fetcher_mode='auto'
        )
    )

    expected = {'type': 'voice_input'}
    actual = json.loads(res[b'Payload'])['request']['event']

    assert expected == actual
