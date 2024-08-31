# coding: utf-8
from __future__ import unicode_literals

import json
import mock
import pytest
import falcon

from uuid import uuid4

from vins_core.dm.request_events import RequestEvent, TextInputEvent, VoiceInputEvent, ServerActionEvent
from vins_core.dm.response import VinsResponse
from vins_core.ext.speechkit_api import SpeechKitHTTPAPI
from vins_core.utils.strings import smart_utf8

from vins_api.speechkit.connectors.speechkit import SpeechKitConnector
from vins_api.speechkit.resources.common import parse_srcrwr, EOU_EXPECTED_CODE, CANCEL_LISTENING_CODE


def _voice_input(utterance, words=None, confidence=1.0):
    res = {
        'type': 'voice_input',
        'asr_result': [
            {
                'confidence': confidence,
                'utterance': utterance,
            }
        ]
    }
    if words is not None:
        res['asr_result'][0]['words'] = [{'confidence': confidence, 'value': word} for word in words]
    return res


def _request(sk_client, request, request_id='123', uuid=None, appname='test', device_id=None, header=None):
    default_header = {'request_id': request_id}
    default_header.update(header or {})
    resp = sk_client.simulate_post(
        '/speechkit/app/%s/' % appname,
        body=smart_utf8(json.dumps({
            'header': default_header,
            'application': {
                'uuid': str(uuid or uuid4()),
                'device_id': str(device_id or uuid4()),
                'client_time': '20161213T151558',
                'app_id': 'com.yandex.search',
                'app_version': '1.2.3',
                'os_version': '5.0',
                'platform': 'android',
                'lang': 'ru-RU',
                'timezone': 'Europe/Moscow',
                'timestamp': '1481631358',
            },
            'request': request,
        })),
        headers={'content-type': b'application/json'})
    return resp


def get_response_header(request_id='123', dialog_id=None, prev_req_id=None, sequence_number=None, **kwargs):
    mock_resp_id = mock.MagicMock()
    mock_resp_id.__eq__.return_value = True
    return {
        'request_id': request_id,
        'dialog_id': dialog_id,
        'prev_req_id': prev_req_id,
        'sequence_number': sequence_number,
        'response_id': mock_resp_id,
    }


def test_bad_uuid(sk_client):
    resp = _request(sk_client, {'event': {'type': 'text_input', 'text': 'привет'}}, uuid='123')
    assert resp.status_code == 400
    assert 'X-Yandex-Vins-OK' not in resp.headers


def test_device_id_not_uuid_format(sk_client):
    device_id = '1234'
    with mock.patch.object(SpeechKitConnector, 'handle_request') as m:
        m.return_value = VinsResponse()
        _request(sk_client, {'event': {'type': 'text_input', 'text': 'привет'}}, device_id=device_id)
        assert str(m.mock_calls[0][1][0].device_id) == device_id


def test_device_id(sk_client):
    device_id = uuid4()
    with mock.patch.object(SpeechKitConnector, 'handle_request') as m:
        m.return_value = VinsResponse()
        _request(sk_client, {'event': {'type': 'text_input', 'text': 'привет'}}, device_id=str(device_id))
        assert str(m.mock_calls[0][1][0].device_id) == device_id.hex


def test_win_uuid(sk_client):
    resp = _request(
        sk_client, {'event': {'type': 'text_input', 'text': 'привет'}},
        uuid='{3626B74D-38B5-47C9-B86C-BDFFDD44DDB6}')
    assert resp.status_code == 200
    assert resp.headers['X-Yandex-Vins-OK'] == 'true'


def test_text_input(sk_client):
    resp = _request(sk_client, {'event': {'type': 'text_input', 'text': 'привет'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }
    resp = _request(sk_client, {'event': {'type': 'text_input', 'text': 'покажи кнопку'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_suggested_input(sk_client):
    resp = _request(sk_client, {'event': {'type': 'suggested_input', 'text': 'привет'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'directives': [],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }
    resp = _request(sk_client, {'event': {'type': 'suggested_input', 'text': 'покажи кнопку'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_empty(sk_client):
    resp = _request(sk_client, {'event': {'type': 'text_input', 'text': 'совсем ничего'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'directives': [],
            'cards': [],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_additional_options(sk_client):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'return_additional_options', 'payload': {}},
        'additional_options': {'a': 1, 'b': {'c': 2}},
    })
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'directives': [{'name': 'additional_options',
                            'sub_name': 'some_options',
                            'payload': {'additional_options': {'a': 1, 'b': {'c': 2}}},
                            'type': 'client_action'}],
            'cards': [],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


TEST_LAAS_REGION = {
    'region_id': 2,
    'precision': 2,
    'latitude': 59.939095,
    'longitude': 30.315868,
    'should_update_cookie': False,
    'is_user_choice': True,
    'suspected_region_id': 213,
    'city_id': 213,
    'region_by_ip': 213,
    'suspected_region_city': 213,
    'location_accuracy': 15000,
    'location_unixtime': 1500477209,
    'suspected_latitude': 55.753960,
    'suspected_longitude': 37.620393,
    'suspected_location_accuracy': 15000,
    'suspected_location_unixtime': 1500477209,
    'suspected_precision': 2,
    'probable_regions':
    [
        {
            'region_id': 213,
            'weight': 0.160000
        },
        {
            'region_id': 2,
            'weight': 0.720000
        }
    ],
    'probable_regions_reliability': 1.70
}


def test_laas_region(sk_client):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'return_laas_region', 'payload': {}},
        'laas_region': TEST_LAAS_REGION,
    })
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'directives': [{'name': 'laas_region',
                            'sub_name': 'some_laas_region',
                            'payload': {'laas_region': TEST_LAAS_REGION},
                            'type': 'client_action'}],
            'cards': [],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_experiments(sk_client, sk_settings):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'experiment', 'payload': {}},
        'experiments': ['flag']
    })
    assert json.loads(resp.content)['response']['card']['text'] == 'flag'

    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'experiment', 'payload': {}},
        'experiments': []
    })
    assert json.loads(resp.content)['response']['card']['text'] == 'no experiments'

    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'experiment', 'payload': {}},
    })
    assert json.loads(resp.content)['response']['card']['text'] == 'no experiments'


def test_text_input_voice_session(sk_client):
    resp = _request(
        sk_client,
        {'event': {'type': 'text_input', 'text': 'привет'}, 'voice_session': True},
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': {'text': 'hello, username!', 'type': 'simple'},
            'should_listen': False,
            'directives': [],
        },
    }
    resp = _request(
        sk_client,
        {'event': {'type': 'text_input', 'text': 'покажи кнопку'}, 'voice_session': True},
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_voice_input_force_voice_session(sk_client):
    resp = _request(
        sk_client,
        {'event': _voice_input('привет'), 'voice_session': False}  # noqa
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': None,
            'should_listen': False,
            'directives': [],
        },
    }


def test_text_input_force_voice_session(sk_client):
    resp = _request(sk_client, {'event': {'type': 'text_input', 'text': 'пишу текстом, но хочу слышать голос'}})
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'все равно скажу', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'все равно скажу', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': {'text': 'все равно скажу', 'type': 'simple'},
            'should_listen': False,
            'directives': []
        },
    }


def test_voice_input(sk_client):
    resp = _request(
        sk_client,
        {'event': _voice_input('сидеть', words=['привет'])}  # utterance should be ignored
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': {'text': 'hello, username!', 'type': 'simple'},
            'should_listen': False,
            'directives': []
        },
    }

    reference = {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }

    resp = _request(
        sk_client,
        {'event': _voice_input('покажи кнопку', words=[])},
    )
    assert json.loads(resp.content) == reference
    resp = _request(
        sk_client,
        {'event': _voice_input('покажи кнопку')},
    )
    assert json.loads(resp.content) == reference


def test_voice_input_without_words(sk_client):
    resp = _request(
        sk_client,
        {'event': _voice_input('привет')}  # utterance should be used instead of words
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': {'text': 'hello, username!', 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': 'hello, username!', 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': {'text': 'hello, username!', 'type': 'simple'},
            'should_listen': False,
            'directives': [],
        },
    }
    resp = _request(
        sk_client,
        {'event': _voice_input('покажи кнопку')},
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_deserialize(sk_client):
    resp = _request(
        sk_client,
        {'event': _voice_input('привет')},
    )
    response = VinsResponse()
    SpeechKitHTTPAPI.deserialize_result(json.loads(resp.content), response)
    assert response.cards[0].text == 'hello, username!'
    assert not response.should_listen
    assert not response.force_voice_answer
    resp = _request(
        sk_client,
        {'event': _voice_input('покажи кнопку')},
    )
    response = VinsResponse()
    SpeechKitHTTPAPI.deserialize_result(json.loads(resp.content), response)
    assert [d.to_dict() for d in response.directives] == [
        {'name': 'do_nothing_show_suggest',
         'sub_name': 'some_do_nothing_show',
         'payload': {},
         'type': 'client_action'}
    ]
    assert [s.to_dict() for s in response.suggests] == [
        {'directives': [{'name': 'log_request',
                         'payload': {},
                         'type': 'server_action',
                         'ignore_answer': False}],
         'title': 'test suggest',
         'type': 'action'}
    ]


def test_callback(sk_client):
    resp = _request(
        sk_client,
        {'event': {'type': 'server_action', 'name': 'server_action_with_suggest', 'payload': {}}}
    )
    assert json.loads(resp.content) == {
        'header': get_response_header('123'),
        'response': {
            'card': None,
            'cards': [],
            'directives': [
                {
                    'name': 'do_nothing_show_suggest',
                    'sub_name': 'some_do_nothing_show',
                    'payload': {},
                    'type': 'client_action'
                }
            ],
            'suggest': {
                'items': [
                    {
                        'directives': [
                            {
                                'name': 'log_request',
                                'payload': {},
                                'type': 'server_action',
                                'ignore_answer': False
                            }
                        ],
                        'title': 'test suggest',
                        'type': 'action'
                    }
                ]
            },
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {'output_speech': None, 'should_listen': False, 'directives': []},
    }


def test_event_from_dict():
    import pytest

    e = RequestEvent.from_dict({'type': 'text_input', 'text': 'abc'})
    assert isinstance(e, TextInputEvent)
    assert e.utterance.text == 'abc'

    e = RequestEvent.from_dict({'type': 'voice_input', 'asr_result': [
        {'confidence': 0.8, 'utterance': 'a 1', 'words': [{'confidence': 1.0, 'value': 'a'},
                                                          {'confidence': 1.0, 'value': 'one'}]},
        {'confidence': 0.9, 'utterance': 'b', 'words': [{'confidence': 1.0, 'value': 'b'}]},
        {'confidence': 0.7, 'utterance': 'c', 'words': [{'confidence': 1.0, 'value': 'c'}]},
    ], "biometry_scoring": {
        "status": "ok",
        "scores": [],
        "request_id": "req",
        "group_id": "grp"
    }})
    assert isinstance(e, VoiceInputEvent)
    assert e.utterance.text == 'a one'
    assert e.asr_utterance() == 'a 1'
    assert e.biometrics_scores()["status"] == "ok"
    assert e.biometrics_scores()["request_id"] == "req"
    assert e.biometrics_scores()["group_id"] == "grp"

    scores_with_mode = [
        {
            "mode": "max_accuracy",
            "scores": [{"1": 0.9}]
        },
        {
            "mode": "high_tpr",
            "scores": [{"1": 0.95}]
        },
        {
            "mode": "high_tnr",
            "scores": [{"1": 0.8}]
        }
    ]
    e = RequestEvent.from_dict({'type': 'voice_input', 'asr_result': [
        {'confidence': 0.8, 'utterance': 'a 1', 'words': [{'confidence': 1.0, 'value': 'a'},
                                                          {'confidence': 1.0, 'value': 'one'}]},
        {'confidence': 0.9, 'utterance': 'b', 'words': [{'confidence': 1.0, 'value': 'b'}]},
        {'confidence': 0.7, 'utterance': 'c', 'words': [{'confidence': 1.0, 'value': 'c'}]},
    ], "biometry_scoring": {
        "status": "ok",
        "scores_with_mode": scores_with_mode,
        "request_id": "req",
        "group_id": "grp"
    }})
    assert isinstance(e, VoiceInputEvent)
    assert e.utterance.text == 'a one'
    assert e.asr_utterance() == 'a 1'
    assert e.biometrics_scores()["status"] == "ok"
    assert e.biometrics_scores()["request_id"] == "req"
    assert e.biometrics_scores()["group_id"] == "grp"
    assert e.biometrics_scores()["scores_with_mode"] == scores_with_mode

    e = RequestEvent.from_dict({'type': 'server_action', 'name': 'abc'})
    assert isinstance(e, ServerActionEvent)
    assert e.utterance is None
    assert e.action_name == 'abc'

    e = RequestEvent.from_dict({'type': 'server_action', 'name': 'abc', 'payload': {'a': 1}})
    assert isinstance(e, ServerActionEvent)
    assert e.utterance is None
    assert e.action_name == 'abc'
    assert e.payload == {'a': 1}

    with pytest.raises(ValueError) as excinfo:
        RequestEvent.from_dict({'type': 'client_action', 'name': 'abc', 'sub_name': 'some_abc'})

    assert 'Unknown event type' in str(excinfo.value)


@pytest.mark.parametrize('error_type, status_code', [
    ('bass_error', 512),
    ('unknown_skill', 200),
    ('any_error', 200),
])
def test_error_meta(sk_client, error_type, status_code):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'return_error_meta', 'payload': {}},
        'additional_options': {'error_type': error_type},
    })
    assert resp.status_code == status_code
    assert resp.headers['X-Yandex-Vins-OK'] == 'true'
    assert json.loads(resp.content)['response']['meta'] == [
        {
            'type': 'error',
            'error_type': error_type,
            'form_name': None
        }
    ]


def test_internal_error_400(sk_client):
    with mock.patch.object(SpeechKitConnector, 'handle_request') as m:
        m.side_effect = falcon.HTTPBadRequest()
        resp = _request(
            sk_client,
            {'event': _voice_input('привет')},
        )
        assert resp.status_code == 400
        assert 'X-Yandex-Vins-OK' not in resp.headers


def test_internal_error_500(sk_client):
    with mock.patch.object(SpeechKitConnector, 'handle_request') as m:
        m.side_effect = RuntimeError()
        resp = _request(
            sk_client,
            {'event': _voice_input('привет')},
        )
        assert resp.status_code == 500
        assert 'X-Yandex-Vins-OK' not in resp.headers


def test_eou_expected(sk_client):
    header = {
        'request_id': str(uuid4()),
        'prev_req_id': None,
        'sequence_number': 0,
        'hypothesis_number': 0,
        'end_of_utterance': False,
    }
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'return_error_meta', 'payload': {}},
        'additional_options': {'error_type': 'eou_expected'},
    }, header=header)

    assert resp.status == EOU_EXPECTED_CODE
    assert 'X-Yandex-Vins-OK' in resp.headers

    resp_header = json.loads(resp.content)['header']

    assert resp_header == get_response_header(**header)


def test_cancel_listening(sk_client):
    header = {
        'request_id': str(uuid4()),
        'prev_req_id': None,
        'sequence_number': 0,
        'hypothesis_number': 0,
        'end_of_utterance': False,
    }
    resp = _request(sk_client, {
        'event': {
            'type': 'server_action',
            'name': 'return_meta_type',
            'payload': {'type': 'cancel_listening'},
        },
    }, header=header)

    assert resp.status == CANCEL_LISTENING_CODE
    assert 'X-Yandex-Vins-OK' in resp.headers

    resp_header = json.loads(resp.content)['header']
    assert resp_header == get_response_header(**header)


@pytest.mark.parametrize('srcrwr_raw, srcrwr_parsed', [
    (None, {}),
    ('BASS=http://bass-prod.yandex.net/', {
        'BASS': 'http://bass-prod.yandex.net/'
    }),
    ('BASS=http://yandex.ru/;BASS=http://bass-prod.yandex.net/', {
        'BASS': 'http://bass-prod.yandex.net/'
    }),
    ('Music=http://music.yandex.ru;BASS=http://bass-prod.yandex.net/', {
        'Music': 'http://music.yandex.ru',
        'BASS': 'http://bass-prod.yandex.net/'
    }),
    ('Music=http://music.yandex.ru;BASS=http://bass-prod.yandex.net/,Search=http://ya.ru;Maps=http://maps.ya.ru', {
        'Music': 'http://music.yandex.ru',
        'BASS': 'http://bass-prod.yandex.net/',
        'Search': 'http://ya.ru',
        'Maps': 'http://maps.ya.ru'
    }),
])
def test_parse_srcrwr(srcrwr_raw, srcrwr_parsed):
    assert srcrwr_parsed == parse_srcrwr(srcrwr_raw)


@pytest.mark.parametrize('experiments_in,experiments_out', [
    ({'analytics_info': '', 'lol': ''}, {'analytics_info': '', 'lol': ''}),
    ({'analytics_info': None, 'lol': ''}, {'analytics_info': None, 'lol': ''}),
    ({'lol': ''}, {'lol': ''}),
    ({}, {})
])
def test_experiments_from_request_to_response(sk_client, experiments_in, experiments_out):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'experiment', 'payload': {}},
        'experiments': experiments_in
    })
    assert json.loads(resp.content)['response']['experiments'] == experiments_out


def test_experiments_from_request_to_response_none(sk_client):
    resp = _request(sk_client, {
        'event': {'type': 'server_action', 'name': 'experiment', 'payload': {}}
    })
    assert json.loads(resp.content)['response']['experiments'] == {}


def _make_minimal_voice_response(request_id, output_speech):
    return {
        'header': get_response_header(request_id),
        'response': {
            'card': {'text': output_speech, 'type': 'simple_text', 'tag': None},
            'directives': [],
            'cards': [{'text': output_speech, 'type': 'simple_text', 'tag': None}],
            'suggest': None,
            'meta': [],
            'features': {},
            'experiments': {}
        },
        'voice_response': {
            'output_speech': {'text': output_speech, 'type': 'simple'},
            'should_listen': False,
            'directives': [],
        },
    }


def test_random_response_has_fixed_seed(sk_client, monkeypatch):
    for i in range(10):
        resp = _request(
            sk_client,
            request_id='123',
            request={'event': _voice_input('дай мне случайный ответ')}
        )
        assert json.loads(resp.content) == _make_minimal_voice_response('123', 'три')

    # Now, we change request_id and get different random response
    resp = _request(
        sk_client,
        request_id='456',
        request={'event': _voice_input('дай мне случайный ответ')}
    )
    assert json.loads(resp.content) == _make_minimal_voice_response('456', 'четыре')

    # Now, we add some salt, response should change once again
    monkeypatch.setenv('VINS_RANDOM_SEED_SALT', str('salt'))
    resp = _request(
        sk_client,
        request_id='456',
        request={'event': _voice_input('дай мне случайный ответ')}
    )
    assert json.loads(resp.content) == _make_minimal_voice_response('456', 'два')
