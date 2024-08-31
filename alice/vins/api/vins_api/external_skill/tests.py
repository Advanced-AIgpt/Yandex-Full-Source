# coding: utf-8
from __future__ import unicode_literals

import json
from uuid import uuid4

import pytest
import mongomock
import falcon
import falcon.testing

from vins_core.config.app_config import Intent, Project, AppConfig
from vins_core.dm.form_filler.models import Form, Event, Handler
from vins_core.dm.response import ClientActionDirective, ServerActionDirective, ActionButton
from vins_sdk.app import VinsApp, callback_method
from vins_api.external_skill.api import make_app
from vins_api.external_skill import settings


class AppForTest(VinsApp):
    @callback_method
    def simple_text(self, req_info, response, **kwargs):
        response.say('simple text')

    @callback_method
    def buttons(self, req_info, response, **kwargs):
        response.say('buttons', buttons=[
            ActionButton('action button', directives=[
                ClientActionDirective(name='open_uri', sub_name='button_open_site', payload={'uri': 'https://ya.ru/'}),
            ]),
        ])
        response.suggests.append(
            ActionButton('suggest button', directives=[
                ServerActionDirective('log_request', payload={'key': 'value'})
            ])
        )

    @callback_method
    def button_callback(self, req_info, response, callback_param, **kwargs):
        response.say(callback_param)

    @callback_method
    def app_info_callback(self, req_info, response, **kwargs):
        response.say(req_info.app_info.app_id)


def app_cfg_for_test():
    cfg = AppConfig([Project('test', intents=[
        Intent(
            'text',
            trainable_classifiers=['clf'],
            dm_form=Form(
                'text',
                events=[Event('submit', handlers=[Handler(handler='callback', name='simple_text')])],
            ),
        ),
        Intent(
            'buttons',
            trainable_classifiers=['clf'],
            dm_form=Form(
                'buttons',
                events=[Event('submit', handlers=[Handler(handler='callback', name='buttons')])],
            ),
        ),
        Intent(
            'dont_know',
            total_fallback=True,
            trainable_classifiers=['fallback_clf'],
            nlg_sources="""
            {% phrase dont_know %}
              don't know
            {% endphrase %}
            """,
            dm_form=Form(
                'dont_know',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'dont_know'}
                )])],
            ),
        ),
    ])])

    cfg.nlu['intent_classifiers'] = [{
        'model': 'data_lookup',
        'name': 'clf',
        'params': {
            'intent_texts': {
                'text': [
                    'text',
                ],
                'buttons': [
                    'buttons',
                ],
                'dont_know': [
                    "don't know",
                ]
            },
        },
        'fallback_threshold': 0,
    }]
    cfg.nlu['fallback_intent_classifiers'] = [{
        'model': 'data_lookup',
        'name': 'fallback_clf',
        'params': {
            'intent_texts': {
                'unknown': [
                    'unknown',
                ],
            },
        },
        'fallback_threshold': 0,
    }]
    cfg.nlu['utterance_tagger'] = {
        'model': 'crf',
        'features': [],
        'params': {'intent_conditioned': True}
    }
    cfg.nlu['fst'] = {
        'resource': '',
        'parsers': []
    }
    return cfg


@pytest.fixture(scope='function', autouse=True)
def skill_settings():
    settings.CONNECTED_APPS = {
        'skills_test': {
            'app_config': app_cfg_for_test,
            'class': AppForTest,
        }
    }
    return settings


@pytest.fixture
def client(skill_settings, mocker):
    mocker.patch('vins_api.common.resources.get_db_connection', return_value=mongomock.MongoClient().test_db)
    return falcon.testing.TestClient(make_app()[0])


@pytest.fixture
def http_request(client):
    def _request(utt=None, payload=None, uuid=None, type_='SimpleUtterance', new=False, client_id='test'):
        resp = client.simulate_post(
            '/external_skill/app/skills_test/',
            body=json.dumps({
                'version': '1.0',
                'request': {
                    'original_utterance': utt or '',
                    'command': utt or '',
                    'type': type_,
                    'payload': payload or {},
                },
                'meta': {
                    'client_id': client_id,
                    'timezone': 'Europe/Moscow',
                    'locale': 'ru-RU',
                },
                'session': {
                    'skill_id': 'skill',
                    'user_id': str(uuid or uuid4()),
                    'message_id': 1,
                    'session_id': 'session',
                    'new': new,
                }
            }),
            headers={'content-type': b'application/json'},
        )
        return resp
    return _request


def test_valid_uuid(http_request):
    resp = http_request('test', uuid='a' * 100)
    assert resp.status_code == 400


def test_valid_request(http_request):
    resp = http_request('test' * 1000)
    assert resp.status_code == 400


def test_simple_text(http_request):
    resp = http_request('text')
    assert json.loads(resp.content)['response'] == {
        'buttons': [],
        'text': 'simple text',
        'tts': 'simple text',
        'end_session': False,
    }


def test_buttons(http_request):
    resp = http_request('buttons')
    assert json.loads(resp.content)['response'] == {
        'text': u'buttons',
        'tts': u'buttons',
        'buttons': [
            {
                'hide': True,
                'payload': {'callback_args': {u'key': u'value'},
                            'callback_name': u'log_request'},
                'title': u'suggest button'
            },
            {
                'hide': False,
                'title': u'action button',
                'url': u'https://ya.ru/'
            }
        ],
        'end_session': False,
    }


def test_press_button(http_request):
    text = 'Hello World!'

    resp = http_request(
        payload={'callback_name': 'button_callback',
                 'callback_args': {'callback_param': text}},
        type_='ButtonPressed'
    )

    assert json.loads(resp.content)['response'] == {
        'buttons': [],
        'text': text,
        'tts': text,
        'end_session': False,
    }


def test_press_suggest(http_request):
    resp = http_request(
        utt='text',
        type_='ButtonPressed',
        payload={'key': 'value'},
    )
    assert json.loads(resp.content)['response'] == {
        'buttons': [],
        'text': 'simple text',
        'tts': 'simple text',
        'end_session': False,
    }


def test_app_id(http_request):
    resp = http_request(
        payload={'callback_name': 'app_info_callback',
                 'callback_args': {'callback_param': 'test'}},
        type_='ButtonPressed',
        client_id='ru.yandex.searchplugin/5.80 (Samsung Galaxy; Android 4.4)',
    )
    assert json.loads(resp.content)['response']['text'] == 'ru.yandex.searchplugin'


def test_broken_app_id(http_request):
    resp = http_request(
        payload={'callback_name': 'app_info_callback',
                 'callback_args': {'callback_param': 'test'}},
        type_='ButtonPressed',
        client_id='test',
    )
    assert json.loads(resp.content)['response']['text'] == 'test'
