# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.config.app_config import Intent, Project, AppConfig
from vins_core.dm.form_filler.models import Form, Event, Handler
from vins_core.dm.request import create_request
from vins_core.dm.request_events import ServerActionEvent
from vins_core.dm.response import ClientActionDirective
from vins_sdk.app import VinsApp, callback_method
from vins_core.config.app_config import NluSourcesConfig


class VinsTestApp(VinsApp):
    @callback_method
    def server_action_a(self, response, **kwargs):
        response.directives.append(ClientActionDirective('action_a', sub_name='test_action_a'))

    @callback_method
    def server_action_b(self, response, **kwargs):
        response.say('hello', append=True)
        response.directives.append(ClientActionDirective('action_b', sub_name='test_action_b'))

    @callback_method
    def server_action_c(self, response, session, req_info, sample, **kwargs):
        new_form = self.dm.new_form('actionA', response=response, req_info=req_info, app=self)
        response.say('action_c')
        self.change_form(session=session, form=new_form, req_info=req_info, sample=sample, response=response)


@pytest.fixture
def vins_sdk_test_app():
    app_conf = AppConfig([Project('test', intents=[
        Intent(
            'actionA',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['сидеть']}]),
            dm_form=Form(
                'actionA',
                events=[Event('submit', handlers=[Handler(handler='callback', name='server_action_a')])],
            ),
            total_fallback=True
        ),
        Intent(
            'actionB',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['стоять']}]),
            dm_form=Form(
                'actionB',
                events=[Event('submit', handlers=[Handler(handler='callback', name='server_action_b')])],
            ),
        ),
        Intent(
            'actionC',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['лежать']}]),
            dm_form=Form(
                'actionC',
                events=[Event('submit', handlers=[Handler(handler='callback', name='server_action_c')])],
            ),
        ),
    ])])
    app_conf.nlu['feature_extractors'] = [
        {'type': 'ngrams', 'id': 'word', 'n': 1},
        {'type': 'ngrams', 'id': 'bigram', 'n': 2},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'},
    ]
    app_conf.nlu['intent_classifiers'] = [{
        'name': 'intent_classifier_0',
        'model': 'maxent',
        'features': ['word', 'bigram', 'ner', 'postag', 'lemma']
    }]
    app_conf.nlu['utterance_tagger'] = {
        'model': 'crf',
        'features': ['word', 'ner', 'lemma'],
        'params': {'intent_conditioned': True}
    }
    app_conf.nlu['fst'] = {
        'resource': '',
        'parsers': []
    }
    app = VinsTestApp(app_conf=app_conf, load_data=True)
    app.nlu.train()
    return app


def handle(app, req_info):
    app.handle_request(req_info)
    return app._load_session(req_info)


def test_change_intent(vins_sdk_test_app):
    session = handle(vins_sdk_test_app, create_request('123', 'лежать'))

    assert len(session.dialog_history) == 1

    turn = session.dialog_history.last()
    response = turn.response.to_dict()
    assert response['cards'] == [
        {'text': 'action_c', 'type': 'simple_text', 'tag': None},
    ]
    assert response['voice_text'] == 'action_c'
    assert response['directives'][0]['name'] == 'action_a'


def test_dialog_history(vins_sdk_test_app):
    session = handle(vins_sdk_test_app, create_request('123', 'сидеть'))

    turn = session.dialog_history.last()
    assert turn.response.directives[0].name == 'action_a'

    session = handle(
        vins_sdk_test_app,
        create_request('123', event=ServerActionEvent(name='server_action_b'))
    )

    assert len(session.dialog_history) == 2

    turn = session.dialog_history.last()
    assert turn.response.directives[0].name == 'action_b'
    assert turn.response.voice_text == 'hello'
