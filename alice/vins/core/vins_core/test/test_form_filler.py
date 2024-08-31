# coding: utf-8
from __future__ import unicode_literals

import functools

from uuid import uuid4 as gen_uuid

from vins_core.common.sample import Sample
from vins_core.dm.form_filler.models import Form, Slot, Handler, Event, RequiredSlotGroup
from vins_core.dm.response import VinsResponse
from vins_core.config.app_config import Project, Intent, AppConfig
from vins_core.dm.form_filler.dialog_manager import DialogManager, FormFillingDialogManager
from vins_core.ner.fst_base import Entity
from vins_sdk.app import VinsApp
from vins_sdk.connectors import TestConnector
from vins_core.config.app_config import NluSourcesConfig
from vins_core.dm.request import create_request
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.nlg.template_nlg import TemplateNLG


def _make_app_from_config(intents):
    app_cfg = AppConfig([
        Project(name='test_project', intents=intents)
    ])
    app_cfg.nlu['feature_extractors'] = [
        {'type': 'ngrams', 'id': 'word', 'n': 1},
        {'type': 'ngrams', 'id': 'bigram', 'n': 2},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'}
    ]
    app_cfg.nlu['intent_classifiers'] = [{
        'model': 'maxent', 'name': 'intent_classifier_0',
        'features': ['word', 'bigram', 'ner', 'postag', 'lemma']
    }]
    app_cfg.nlu['utterance_tagger'] = {
        'model': 'crf',
        'features': ['word', 'ner', 'lemma'],
        'params': {
            'intent_conditioned': True
        }
    }
    app_cfg.nlu['fst'] = {
        'resource': '',
        'parsers': [],
    }
    dm = DialogManager.from_config(app_cfg, load_data=True)
    dm.app_cfg = app_cfg
    dm.nlu.train()
    app = VinsApp(dm=dm)
    connector = TestConnector(vins_app=app)

    return functools.partial(connector.handle_utterance, gen_uuid(), text_only=False)


def test_required_slot_groups():
    intent = Intent(
        name='intent',
        trainable_classifiers=['intent_classifier_0'],
        dm_form=Form(
            name='intent',
            slots=[
                Slot(
                    name='slot1',
                    types=['string'],
                    events=[Event(name='ask', handlers=[Handler(
                        handler='callback',
                        name='nlg_callback',
                        params={'phrase_id': 'ask_slot1'}
                    )])],
                    optional=False),
                Slot(name='slot2', types=['string'], optional=True)
            ],
            events=[],
            required_slot_groups=[RequiredSlotGroup(slots=['slot1', 'slot2'], slot_to_ask='slot1')]
        ),
        nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['intent']}]),
        nlg_sources="""
            {% phrase ask_slot1 %}
                ask_slot1
            {% endphrase %}
        """,
        total_fallback=True
    )
    f = _make_app_from_config([intent])
    assert f('intent')['voice_text'] == 'ask_slot1'


def test_required_slot_groups2():
    sample = Sample.from_string(u'видео про котиков')
    empty_sample = Sample.from_string(u'про котиков')
    frame = {
        'intent_candidate': IntentCandidate(name='yet_another_form'),
        'slots': {
            'slot_for_filling': [{
                'substr': u'видео',
                'start': 0,
                'end': 1,
                'entities': []
            }]
        }
    }
    form = Form.from_dict({
        'name': 'yet_another_form',
        'slots': [
            {
                'slot': 'slot_for_asking',
                'type': 'string',
                'optional': False,
                "events": [
                    {
                        "event": "ask",
                        "handlers": [
                            {
                                "handler": "callback",
                                "name": "nlg_callback",
                                "params": {
                                    "phrase_id": "ask_slot_for_asking",
                                    "question": True
                                }
                            }
                        ]
                    }
                ]
            },
            {
                'slot': 'slot_for_filling',
                'type': 'string',
                'optional': True,
                'events': []
            }
        ],
        "required_slot_groups": [
            {
                'slots': ['slot_for_asking', 'slot_for_filling'],
                'slot_to_ask': 'slot_for_asking'
            }
        ]
    })
    template_nlg = TemplateNLG()
    template_nlg.add_intent(
        'yet_another_form',
        """
            {% phrase ask_slot_for_asking %}
                ask_slot_for_asking
            {% endphrase %}
        """
    )

    slot_for_filling = form.slot_for_filling
    slot_for_asking = form.slot_for_asking

    dm = FormFillingDialogManager([], {}, None, nlg=template_nlg)
    app = VinsApp(dm=dm)
    dm._ask_slot(slot_for_asking, form, empty_sample, session=None, app=app, req_info=None, response=VinsResponse())

    assert slot_for_filling.active
    assert slot_for_asking.active

    dm._fill_form(form, frame, sample, session=None, app=app, req_info=None, response=None)
    assert slot_for_asking.optional
    assert slot_for_filling.value == u'видео'
    assert not slot_for_filling.active
    assert not slot_for_asking.active


def test_slot_fill_event():
    intent = Intent(
        name='intent',
        trainable_classifiers=['intent_classifier_0'],
        dm_form=Form(
            name='intent',
            slots=[
                Slot(
                    name='slot1',
                    types=['string'],
                    events=[Event(name='fill', handlers=[Handler(
                        handler='callback',
                        name='nlg_callback',
                        params={'phrase_id': 'fill_slot1'}
                    )])],
                    optional=False),
                Slot(name='slot2', types=['string'], optional=True)
            ],
            events=[Event(name='submit', handlers=[Handler(
                handler='callback',
                name='nlg_callback',
                params={'phrase_id': 'submit'}
            )])],
            required_slot_groups=[RequiredSlotGroup(slots=['slot1', 'slot2'], slot_to_ask='slot1')]
        ),
        nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['"intent"(slot1)']}]),
        nlg_sources="""
            {% phrase fill_slot1 %}
                fill_slot1
            {% endphrase %}
            {% phrase submit %}
                submit
            {% endphrase %}
        """,
        total_fallback=True
    )
    f = _make_app_from_config([intent])

    assert f('intent')['voice_text'] == 'fill_slot1\nsubmit'


def test_share_slot_value(simple_app):
    f = functools.partial(simple_app.handle_utterance, gen_uuid())
    assert f('что такое яблоко') == 'яблоко это...'
    assert f('а какого цвета') == 'яблоко... цвета'


def test_source_text_in_slot(simple_app):
    req_info = create_request('123', 'какого цвета яблоко')
    simple_app.handle_request(req_info)

    session = simple_app.vins_app._load_session(req_info)
    assert session.form.slots[0].source_text == 'яблоко'

    req_info = create_request('123', 'включи тест')
    simple_app.handle_request(req_info)

    session = simple_app.vins_app._load_session(req_info)
    assert session.form.slots[0].value == 'test' and session.form.slots[0].source_text == 'тест'


def test_multislot_form():
    sample = Sample.from_string(u'веселые видео про котиков')
    frame = {
        'intent_candidate': IntentCandidate(name='video_play'),
        'slots': {
            'search_text': [
                {
                    'substr': u'веселые',
                    'start': 0,
                    'end': 1,
                    'entities': []
                },
                {
                    'substr': u'про котиков',
                    'start': 2,
                    'end': 4,
                    'entities': []
                }
            ],
            'content_type': [{
                'substr': u'видео',
                'start': 0,
                'end': 1,
                'entities': [
                    Entity(
                        type='VIDEO_CONTENT_TYPE',
                        value='video',
                        start=0,
                        end=1
                    )
                ]
            }],
        }
    }
    form = Form.from_dict({
        'name': 'video_play',
        'slots': [
            {
                'slot': 'search_text',
                'type': 'string',
                'optional': True,
                'events': []
            },
            {
                'slot': 'content_type',
                'type': 'video_content_type',
                'optional': True,
                'events': []
            }
        ]
    })
    dm = FormFillingDialogManager([], {}, None, None)
    dm._fill_form(form, frame, sample, session=None, app=None, req_info=None, response=None)
    slot = form.search_text
    assert slot.value == [u'веселые', u'про котиков']
    assert slot.source_text == [u'веселые', u'про котиков']
    assert slot.value_type == ['string', 'string']
