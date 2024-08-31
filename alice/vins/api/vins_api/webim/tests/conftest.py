# coding: utf-8

from __future__ import unicode_literals

import os
import mock
import mongomock
import pytest
import falcon
import falcon.testing
import yatest.common
import json

from vins_core.config.app_config import Intent, Project, AppConfig
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.form_filler.models import Form, Event, Handler
from vins_core.dm.response import ClientActionDirective, ServerActionDirective, ActionButton
from vins_core.config.app_config import NluSourcesConfig
from vins_core.ner.fst_presets import PARSER_RU_BASE_PARSERS

from vins_sdk.app import VinsApp, callback_method

from vins_api.webim import settings
from vins_api.webim.api import make_app


@pytest.fixture(scope='package', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')
    os.environ['CRMBOT_WEBIM_USER_DATABASE'] = 'test:test\nanother:test'
    key1 = "tehsuperduperrandomkey0123456789qwertyuiop"[:32].encode('hex')
    key2 = "anothertehsuperduperrandomkey0123456789qwertyuiop"[:32].encode('hex')
    os.environ['CRMBOT_FRONTEND_DECRYPTION_KEY'] = json.dumps({"key_v1": key1, "key_v2": key2})
    os.environ['WEBIM_DEFAULT_DEPARTMENT_KEY'] = 'dept_default'
    os.environ['WEBIM_AUTH_TOKEN'] = 'super_auth_token'
    os.environ['WEBIM_URL'] = 'http://localhost/with/a/totally/valid/path'
    os.environ['VINS_TSUM_TRACE_LOG'] = 'no_an_empty_string'
    os.environ['OCRM_AUTH_TOKEN'] = 'test:test'
    os.environ['OCRM_URL'] = 'http://localhost/with/a/totally/valid/ocrm/path'


class WebimAppForTest(VinsApp):

    def get_redirect_error_text(self, req_info):
        return "Everything went terribly wrong"

    @callback_method
    def message_with_suggest(self, response, **kwargs):
        response.say('hello')
        response.suggests.append(ActionButton('test suggest', directives=[
            ClientActionDirective('close_app', sub_name='some_close_app')
        ]))

    @callback_method
    def message_with_multiple_suggests(self, response, **kwargs):
        response.say('hello')
        response.suggests.append(
            ActionButton('test suggest 1', directives=[
                ServerActionDirective("suggest1", payload={"suggest_block": {"data": {}}})])
        )
        response.suggests.append(
            ActionButton('test suggest 2', directives=[
                ServerActionDirective("suggest2", payload={"suggest_block": {"data": {}}})])
        )
        response.suggests.append(
            ActionButton('test suggest 3', directives=[
                ServerActionDirective("suggest3", payload={"suggest_block": {"data": {"nobr": True}}})])
        )
        response.suggests.append(
            ActionButton('test suggest 4', directives=[
                ServerActionDirective("suggest4", payload={"suggest_block": {"data": {}}})])
        )


@pytest.fixture(scope='session')
def webim_app_cfg_for_test():
    cfg = AppConfig([Project('test_webim', intents=[
        Intent(
            'operator_redirect',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['are;jnbgrqebijdjafnbg;kjade eabrlkae']}]),
            nlg_sources="""
                    {% phrase operator_redirect %}
                        OPERATOR_REDIRECT
                    {% endphrase %}
                    """,
            dm_form=Form(
                'operator_redirect',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'operator_redirect'}
                )])],
            ),
            total_fallback=True
        ),
        Intent(
            'hello',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['привет']}]),
            nlg_sources="""
            {% phrase hello %}
                hello, username!
            {% endphrase %}
            """,
            dm_form=Form(
                'hello',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'hello'}
                )])],
            ),
            total_fallback=False
        ),
        Intent(
            'message_with_suggest',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['покажи мне кнопку']}]),
            dm_form=Form(
                'message_with_suggest',
                events=[Event('submit', handlers=[Handler(handler='callback', name='message_with_suggest')])],
            ),
        ),
        Intent(
            'message_with_multiple_suggests',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['покажи мне много кнопок']}]),
            dm_form=Form(
                'message_with_multiple_suggests',
                events=[Event('submit', handlers=[Handler(handler='callback', name='message_with_multiple_suggests')])],
            ),
        ),
        Intent(
            'message_with_targeted_operator_redirect',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['переведи меня на особого оператора']}]),
            nlg_sources="""
            {% phrase redirect %}
                Redirecting you to a very special operator

                OPERATOR_REDIRECT_123456789
            {% endphrase %}
            """,
            dm_form=Form(
                'message_with_targeted_operator_redirect',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'redirect'}
                )])],
            ),
            total_fallback=False
        ),
        Intent(
            'message_with_targeted_department_redirect',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['переведи меня в блатной отдел']}]),
            nlg_sources="""
            {% phrase redirect %}
                Redirecting you to a very special department

                DEPARTMENT_REDIRECT_123456789.qwer-ty
            {% endphrase %}
            """,
            dm_form=Form(
                'message_with_targeted_department_redirect',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'redirect'}
                )])],
            ),
            total_fallback=False
        ),
        Intent(
            'close_chat',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['закрой этот чат']}]),
            nlg_sources="""
            {% phrase close %}
                Closing this chat.

                CLOSE_CHAT
            {% endphrase %}
            """,
            dm_form=Form(
                'close_chat',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'close'}
                )])],
            ),
            total_fallback=False
        )
    ])])
    cfg.nlu['feature_extractors'] = [
        {'type': 'ngrams', 'id': 'word', 'n': 1},
        {'type': 'ngrams', 'id': 'bigram', 'n': 2},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'}
    ]
    cfg.nlu['intent_classifiers'] = [{
        'name': 'intent_classifier_0',
        'model': 'maxent',
        'features': ['word', 'bigram', 'ner', 'postag', 'lemma']
    }]
    cfg.nlu['utterance_tagger'] = {
        'model': 'crf',
        'features': ['word', 'ner', 'lemma'],
        'params': {'intent_conditioned': True}
    }
    cfg.nlu['fst'] = {
        'resource': 'resource://fst',
        'parsers': PARSER_RU_BASE_PARSERS
    }
    return cfg


@pytest.fixture(scope='module')
def webim_dm_mock(webim_app_cfg_for_test):
    dm = DialogManager.from_config(webim_app_cfg_for_test, load_data=True)
    dm.nlu.train()
    with mock.patch.object(DialogManager, 'from_config') as m:
        m.return_value = dm
        yield dm


@pytest.fixture
def webim_settings(webim_app_cfg_for_test):
    settings.CONNECTED_APPS = {
        'test_webim': {
            'app_config': webim_app_cfg_for_test,
            'class': WebimAppForTest,
            'resource': "webim"
        },
        'test_webim_v2': {
            'app_config': webim_app_cfg_for_test,
            'class': WebimAppForTest,
            'resource': "webim_v2"
        },
        'test_ocrm': {
            'app_config': webim_app_cfg_for_test,
            'class': WebimAppForTest,
            'resource': "ocrm"
        },
        'test_ocrm_v2': {
            'app_config': webim_app_cfg_for_test,
            'class': WebimAppForTest,
            'resource': "ocrm_v2"
        },
    }


@pytest.fixture
def webim_client(webim_settings, webim_dm_mock, mocker):
    mocker.patch('vins_api.common.resources.get_db_connection', return_value=mongomock.MongoClient().test_db)
    app, app_resources = make_app()
    return falcon.testing.TestClient(app), app_resources
