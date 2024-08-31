# coding: utf-8

from __future__ import unicode_literals

import json
from uuid import uuid4

import falcon
import falcon.testing
import mock
import mongomock
import pytest

from vins_core.nlu.features.base import SampleFeatures
from vins_core.schema import features_pb2
from vins_core.utils.strings import smart_utf8
from vins_api.speechkit import settings
from vins_api.speechkit.api import make_app


@pytest.fixture
def client(mocker):
    settings.CONNECTED_APPS = {
        'pa_test': {
            'path': 'personal_assistant/config/Vinsfile.json',
            'class': 'personal_assistant.app.PersonalAssistantApp',
            'intent_renames': 'personal_assistant/tests/validation_sets/toloka_intent_renames.json'
        }
    }
    mocker.patch('vins_api.common.resources.get_db_connection', return_value=mongomock.MongoClient().test_db)
    return falcon.testing.TestClient(make_app()[0])


def get_s3_data(self, key, *args, **kwargs):
    return True, None, None


def _request(client, request, uuid=None, appname='pa_test', device_id=None, header=None, platform='android',
             app_id='com.yandex.search', true_intent=None, prev_intent=None, pred_intent=None):
    default_header = {'request_id': '123'}
    default_header.update(header or {})

    with mock.patch('vins_core.ext.s3.S3DownloadAPI.get_if_modified', get_s3_data):
        resp = client.simulate_post(
            '/qa/%s/nlu' % appname,
            body=json.dumps({
                'header': default_header,
                'application': {
                    'uuid': str(uuid or uuid4()),
                    'device_id': str(device_id or uuid4()),
                    'client_time': '20161213T151558',
                    'app_id': app_id,
                    'app_version': '1.2.3',
                    'os_version': '5.0',
                    'platform': platform,
                    'lang': 'ru-RU',
                    'timezone': 'Europe/Moscow',
                    'timestamp': '1481631358',
                },
                'request': {
                    'event': request,
                    'true_intent': true_intent,
                    'prev_intent': prev_intent,
                    'pred_intent': pred_intent
                },
            }),
            headers={'content-type': b'application/json'})
    return resp


def _features_request(sk_client, request_or_text, uuid=None, appname='pa_test', device_id=None, header=None,
                      params=None, req_headers=None, light=False):
    default_header = {'request_id': '123'}
    default_header.update(header or {})
    if isinstance(request_or_text, (str, unicode)):
        request_or_text = {'event': {'type': 'text_input', 'text': request_or_text}}

    headers = {'content-type': b'application/json'}
    headers.update(req_headers or {})
    if light:
        uri_template = '/qa/%s/features_light'
    else:
        uri_template = '/qa/%s/features'

    with mock.patch('vins_core.ext.s3.S3DownloadAPI.get_if_modified', get_s3_data):
        resp = sk_client.simulate_post(
            uri_template % appname,
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
                'request': request_or_text,
            })),
            headers=headers,
            params=params,
        )
    return resp


@pytest.mark.parametrize('utterance', [
    'привет',
    'какая погода в москве',
    'кто сейчас президент',
])
def test_nlu_response(client, utterance):
    resp = _request(client, {
        'type': 'text_input',
        'text': utterance
    })
    assert resp.status_code == 200
    resp_data = json.loads(resp.content)
    assert resp_data
    assert len(resp_data) == 2
    assert resp_data['true_intent'] is None
    assert resp_data['semantic_frames']
    for item in resp_data['semantic_frames']:
        assert 'confidence' in item
        assert isinstance(item['confidence'], float)
        assert 'entities' in item
        assert isinstance(item['entities'], list)
        assert 'intent_candidate' in item
        assert isinstance(item['intent_candidate'], basestring)
        assert 'slots' in item
        assert isinstance(item['slots'], dict)
        assert 'tagger_score' in item
        assert item['tagger_score'] is None or isinstance(item['tagger_score'], float)


@pytest.mark.parametrize('utterance, prev_intent, true_intent, pred_intent, renamed_true_intent', [
    ('а на завтра?', 'personal_assistant.scenarios.get_weather', 'search.news_and_weather.weather',
     'personal_assistant.scenarios.get_weather', 'personal_assistant.scenarios.get_weather'),
    ('а на выходные?', 'personal_assistant.scenarios.get_weather', 'search.news_and_weather.weather',
     'personal_assistant.scenarios.get_weather', 'personal_assistant.scenarios.get_weather')
])
def test_nlu_response_ellipsis(client, utterance, prev_intent, true_intent, pred_intent, renamed_true_intent):
    resp = _request(client, {
        'type': 'text_input',
        'text': utterance
    }, prev_intent=prev_intent, true_intent=true_intent)
    assert resp.status_code == 200
    resp_data = json.loads(resp.content)
    assert resp_data['true_intent'] == renamed_true_intent
    assert resp_data['semantic_frames'][0]['intent_name'] == pred_intent


@pytest.mark.parametrize(
    'utterance, true_intent, pred_intent, renamed_true_intent, renamed_pred_intent, app_id, platform, output_intent', [
        (None, 'search.news_and_weather.weather', 'personal_assistant.scenarios.get_weather__ellipsis',
         'personal_assistant.scenarios.get_weather', 'personal_assistant.scenarios.get_weather', None, None, None),
        ('выключи комп', 'action.pc.stop.power_off', None, 'personal_assistant.stroka.power_off',
         None, 'winsearchbar', 'windows', 'personal_assistant.stroka.power_off'),
    ])
def test_nlu_response_additional_params(
    client, utterance, true_intent, pred_intent, renamed_true_intent, renamed_pred_intent, app_id, platform,
    output_intent
):
    resp = _request(client, {
        'type': 'text_input',
        'text': utterance
    }, true_intent=true_intent, pred_intent=pred_intent, app_id=app_id, platform=platform)
    assert resp.status_code == 200
    resp_data = json.loads(resp.content)
    if true_intent:
        assert resp_data['true_intent'] == renamed_true_intent
    else:
        assert 'true_intent' not in resp_data
    if pred_intent:
        assert resp_data['pred_intent'] == renamed_pred_intent
    else:
        assert 'pred_intent' not in resp_data
    if utterance:
        assert resp_data['semantic_frames'][0]['intent_name'] == output_intent
    else:
        assert 'semantic_frames' not in resp_data


def test_bad_request(client):
    resp = _request(client, {})
    assert resp.status_code == 400
    resp = _request(client, {
        'type': 'text_input'
    })
    assert resp.status_code == 400


def test_features(client):
    phrase = 'дратути'
    resp = _features_request(client, phrase)
    assert resp.status_code == 200
    response_obj = features_pb2.SampleFeatures()
    response_obj.ParseFromString(bytes(resp.content))
    response_phrase = response_obj.sample.utterance.text
    assert phrase == response_phrase.decode('utf8')


def test_features_json_param(client):
    phrase = 'дратути'
    params = {b'_json': b'1'}
    resp = _features_request(client, phrase, params=params)
    assert resp.status_code == 200
    response_obj = json.loads(resp.content)
    response_phrase = response_obj['sample']['utterance']['text']
    assert phrase == response_phrase


def test_features_accept_header(client):
    phrase = 'дратути'
    headers = {'accept': b'application/json'}
    resp = _features_request(client, phrase, req_headers=headers)
    assert resp.status_code == 200
    response_obj = json.loads(resp.content)
    response_phrase = response_obj['sample']['utterance']['text']
    assert phrase == response_phrase


def test_classification_scores(client):
    phrase = 'дратути'
    resp = _features_request(client, phrase)
    assert resp.status_code == 200
    response_obj = features_pb2.SampleFeatures()
    response_obj.ParseFromString(bytes(resp.content))
    assert len(response_obj.classification_scores) > 0
    assert response_obj.classification_scores[0].name == 'predict_intents'


def test_tagger_scores(client):
    phrase = 'дратути'
    resp = _features_request(client, phrase)
    assert resp.status_code == 200
    response_obj = features_pb2.SampleFeatures()
    response_obj.ParseFromString(bytes(resp.content))
    assert len(response_obj.tagger_scores) > 0
    assert response_obj.tagger_scores[0].name == 'nlu_handle'
    assert response_obj.tagger_scores[1].name == 'post_processed'


def test_invalid_ut8_data_in_sample_token(client):
    phrase = 'поясни что за группировка играет'
    response = _features_request(client, phrase)
    assert response.status_code == 200
    proto_binary = response.content
    sf_obj = SampleFeatures.from_bytes(proto_binary)
    assert isinstance(sf_obj, SampleFeatures)


@pytest.fixture
def mocked_handle_form(mocker):
    mock_obj = mocker.patch('vins_core.dm.form_filler.dialog_manager.FormFillingDialogManager.handle_form')
    return mock_obj


def test_only_sample_features(client, mocked_handle_form):
    from vins_core.dm.form_filler.dialog_manager import FormFillingDialogManager as dm
    phrase = 'какая сегодня погода'
    response = _features_request(client, phrase, light=True)
    assert response.status_code == 200
    assert dm.handle_form.call_count == 0
    response = _features_request(client, phrase)
    assert response.status_code == 200
    assert dm.handle_form.call_count == 1


def test_nlu_tags(client):
    phrase = 'километраж скажи'
    additional_options = {"nlu_tags": ["B-route_action_type", "O"]}
    request = {
        'event': {'type': 'text_input', 'text': phrase},
        'additional_options': additional_options
    }
    response = _features_request(client, request, light=True)
    assert response.status_code == 200
    proto_binary = response.content
    sf_obj = SampleFeatures.from_bytes(proto_binary)
    assert sf_obj.sample.tags == additional_options['nlu_tags']
