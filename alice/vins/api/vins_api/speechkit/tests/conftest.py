# coding: utf-8

from __future__ import unicode_literals

import os
import mock
import mongomock
import pytest
import random
import falcon
import falcon.testing
import yatest.common
import pytz

from datetime import datetime

from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from alice.megamind.protos.scenarios.response_pb2 import TScenarioCommitResponse

from personal_assistant.meta import ExternalSkillMeta, GeneralConversationMeta

from vins_core.config.app_config import Intent, Project, AppConfig
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.form_filler.models import Form, Event, Handler
from vins_core.dm.response import (
    ClientActionDirective,
    ServerActionDirective,
    UniproxyActionDirective,
    ActionButton,
    ErrorMeta,
    Meta,
    ApplyArguments,
    FormInfoFeatures,
)
from vins_core.config.app_config import NluSourcesConfig
from vins_core.ner.fst_presets import PARSER_RU_BASE_PARSERS
from vins_core.utils.json_util import MessageToDict

from vins_sdk.app import VinsApp, callback_method
from vins_sdk.connectors import TestConnector

from vins_api.speechkit import settings
from vins_api.speechkit.api import make_app
from vins_api.speechkit.session import SKSessionStorage


@pytest.fixture(scope='package', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


class AppForTest(VinsApp):

    @callback_method
    def empty_response(self, req_info, form, **kwargs):
        pass

    @callback_method
    def experiment(self, req_info, response, **kwargs):
        if any(value is not None for x, value in req_info.experiments.items()):
            response.say((x for x, value in req_info.experiments.items() if value is not None).next())
        else:
            response.say('no experiments')

    @callback_method
    def location_response(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective('location', sub_name='some_location', payload={
            'location': req_info.location
        }))

    @callback_method
    def server_action_a(self, response, **kwargs):
        response.directives.append(ClientActionDirective('action_a', sub_name='some_action_a'))

    @callback_method
    def server_action_b(self, response, **kwargs):
        response.directives.append(ClientActionDirective('action_b', sub_name='some_action_b'))

    @callback_method
    def server_action_with_suggest(self, response, **kwargs):
        response.directives.append(ClientActionDirective('do_nothing_show_suggest', sub_name='some_do_nothing_show'))
        response.suggests.append(ActionButton('test suggest', directives=[ServerActionDirective('log_request')]))

    @callback_method
    def message_with_suggest(self, response, **kwargs):
        response.say('привет')
        response.suggests.append(ActionButton('test suggest', directives=[
            ClientActionDirective('close_app', sub_name='some_close_app')
        ]))

    @callback_method
    def text_message_with_forced_voice(self, response, **kwargs):
        response.say('все равно скажу', force_voice_answer=True)

    @callback_method
    def message_with_buttons(self, response, **kwargs):
        response.say('хорошо', buttons=[
            ActionButton('Открыть ya.ru', directives=[
                ClientActionDirective('open_uri', sub_name='open_ya', payload={'uri': 'https://ya.ru/'}),
            ]),
        ])

    @callback_method
    def return_additional_options(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective('additional_options', sub_name='some_options', payload={
            'additional_options': req_info.additional_options,
        }))

    @callback_method
    def return_laas_region(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective('laas_region', sub_name='some_laas_region', payload={
            'laas_region': req_info.laas_region,
        }))

    @callback_method
    def return_error_meta(self, req_info, response, **kwargs):
        response.add_meta(ErrorMeta(error_type=req_info.additional_options.get('error_type')))

    @callback_method
    def return_meta_type(self, req_info, response, type, **kwargs):
        response.add_meta(Meta(type))

    @callback_method
    def random_response(self, response, **kwargs):
        phrases = ['раз', 'два', 'три', 'четыре', 'пять']
        phrase = random.choice(phrases)
        response.say(phrase)

    @callback_method
    def external_skill_cities_response(self, response, **kwargs):
        response.set_apply_arguments(ApplyArguments(form_update={}, callback={}))

    @callback_method
    def search_response_continue(self, response, **kwargs):
        response.set_continue_arguments(ApplyArguments(form_update={}, callback={}))

    @callback_method
    def search_response_commit(self, response, **kwargs):
        response.say('Обязательно.')
        response.set_commit_arguments(
            {
                'IsFinished': False,
                'ObjectTypeName': 'TDirectGalleryHitConfirmContinuation',
                'State': 'null'
            }
        )

    @callback_method
    def weather_response(self, req_info, response, session, **kwargs):
        if session.intent_name == 'weather_response__ellipsis':
            response.say('И завтра нормальная')
        else:
            response.say('Нормальная')

    @callback_method
    def add_analytics_info(self, response, **kwargs):
        form_dict = {
            'form': 'personal_assistant.scenarios.search',
            'slots': [
                {
                    'slot': 'query',
                    'value': 'какое-то значение',
                    'value_type': 'string',
                    'types': [
                        'string'
                    ],
                    'optional': False
                },
                {
                    'slot': 'query_none',
                    'value': None,
                    'value_type': 'string',
                    'types': [
                        'string'
                    ],
                    'optional': False
                },
                {
                    'slot': 'where_from',
                    'value': 'home',
                    'value_type': 'named_location',
                    'types': [
                        'named_location'
                    ],
                    'optional': True
                },
                {
                    'name': 'search_results',
                    'value': None,
                    'value_type': 'search_results',
                    'types': [
                        'search_results'
                    ],
                    'optional': True
                },
                {
                    'name': 'unicode_data',
                    'value': {
                        'data': {
                            'ключ': 'значение',
                        }
                    },
                    'value_type': 'unicode_data',
                    'types': ['unicode_data'],
                    'optional': True
                }
            ]
        }
        response.set_analytics_info(
            intent=None,
            form=form_dict,
            scenario_analytics_info=TAnalyticsInfo(Intent='personal_assistant.scenarios.search'),
            product_scenario_name='search',
        )

    @callback_method
    def find_contact(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective('find_contacts'))
        response.directives.append(UniproxyActionDirective('add_contact_book_asr'))

    @callback_method
    def add_features(self, response, session, **kwargs):
        feature = FormInfoFeatures()

        feature.intent = session.intent_name
        feature.is_continuing = session.intent_name.endswith('__continue')

        response.set_feature(feature)

    @callback_method
    def set_intent_player_continue(self, response, **kwargs):
        features = FormInfoFeatures()
        features.intent = 'personal_assistant.scenarios.player_continue'
        response.set_feature(features)

        response.directives.append(ClientActionDirective(name='player_continue'))

    @callback_method
    def set_intent_player_continue_and_add_music_directive(self, response, **kwargs):
        features = FormInfoFeatures()
        features.intent = 'personal_assistant.scenarios.player_continue'
        response.set_feature(features)

        response.directives.append(ClientActionDirective(name='player_continue', payload={'player': 'music'}))

    @callback_method
    def set_intent_player_continue_and_add_video_directive(self, response, **kwargs):
        features = FormInfoFeatures()
        features.intent = 'personal_assistant.scenarios.player_continue'
        response.set_feature(features)

        response.directives.append(ClientActionDirective(name='video_play'))

    @callback_method
    def irrelevant_features(self, response, session, **kwargs):
        feature = FormInfoFeatures()

        feature.intent = session.intent_name
        feature.is_continuing = False
        feature.is_irrelevant = True

        response.set_feature(feature)

    @callback_method
    def get_time(self, req_info, response, session, **kwargs):
        client_time = req_info.client_time
        client_time = client_time.astimezone(pytz.timezone('Europe/Moscow'))
        response.say(datetime.strftime(client_time, '%H:%M'))

    @callback_method
    def image_search(self, req_info, response, **kwargs):
        if req_info.has_image_search_granet:
            response.say('Держите!')
        else:
            response.say('Поищите в Яндексе!')

    @callback_method
    def add_error_meta(self, response, **kwargs):
        response.add_meta(ErrorMeta(error_type='noresponse'))

    @callback_method
    def add_gc_intent_meta(self, response, **kwargs):
        response.add_meta(ExternalSkillMeta(deactivating=False, skill_name='Чат с Алисой'))
        response.add_meta(GeneralConversationMeta(pure_gc=True))

    @callback_method
    def text_with_suggest(self, response, **kwargs):
        response.say('text')
        response.suggests = [ActionButton('test suggest', directives=[
            ClientActionDirective('open_uri', sub_name='open_uri', payload={'uri': 'https://ya.ru/'})
        ])]

    @callback_method
    def text_with_frame_actions(self, response, **kwargs):
        response.say('text')
        response.frame_actions = {
            'id0': {
                'directives': {
                    'list': [
                        {
                            'type_text_directive': {
                                'text': 'Skol\'ko ehat\' do doma?',
                            },
                        },
                        {
                            'callback_directive': {
                                'ignore_answer': True,
                                'name': 'on_skolko_ehat',
                                'payload': {
                                    'date': 'today',
                                },
                            },
                        },
                    ],
                },
            },
            'id1': {
                'directives': {
                    'list': [
                        {
                            'type_text_silent_directive': {
                                'text': 'Tiho!',
                            },
                        },
                    ],
                },
            },
        }

    @callback_method
    def text_with_scenario_data(self, response, **kwargs):
        response.say('text')
        response.scenario_data = {
            'example_scenario_data': {
                'hello': 'test_hello'
            }
        }

    @callback_method
    def text_with_search_suggest(self, response, **kwargs):
        response.say('text')
        response.suggests = [ActionButton('test suggest', directives=[
            ClientActionDirective('type_silent', payload={'text': '🔍 "Погода завтра какая будет скажи мне пожалуйста"'}, type='client_action', sub_name='render_buttons_type_silent'),
            ServerActionDirective(
                name='on_suggest',
                payload={
                    'caption': '🔍 "Погода завтра какая будет..."',
                    'user_utterance': '🔍 "Погода завтра какая будет..."',
                    'suggest_block': {
                        'suggest_type': 'search_internet_fallback',
                        'type': 'suggest',
                        'form_update': {
                            'slots': [
                                {'optional': True, 'type': 'string', 'name': 'query', 'value': 'Погода завтра какая будет скажи мне пожалуйста'},
                                {'optional': True, 'type': 'bool', 'name': 'disable_change_intent', 'value': True},
                            ],
                            'resubmit': True,
                            'name': 'personal_assistant.scenarios.search',
                        },
                        'data': None,
                    },
                    'request_id': '0E7B1CC8-FEEF-48DC-966F-663407B1DFDA'
                },
                type='server_action',
                ignore_answer=True,
            ),
        ])]

    @callback_method
    def apply_request(self, req_info, session, response, callback, **kwargs):
        assert callback == {'arguments': {}, 'name': 'empty_response'}

    @callback_method
    def commit_request(self, req_info, session, response, **kwargs):
        bass_proto_response = TScenarioCommitResponse()
        response.set_commit_response(MessageToDict(bass_proto_response))


@pytest.fixture(scope='session')
def app_cfg_for_test():
    cfg = AppConfig([Project('test', intents=[
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
            total_fallback=True
        ),
        Intent(
            'actionA',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['сидеть']}]),
            dm_form=Form(
                'actionA',
                events=[Event('submit', handlers=[Handler(handler='callback', name='server_action_a')])],
            ),
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
            'ask',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['купи билеты в кино']}]),
            nlg_sources="""
            {% phrase ask %}
              на какой фильм?
            {% endphrase %}
            """,
            dm_form=Form(
                'ask',
                events=[Event('submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'ask', 'question': True}
                )])],
            ),
        ),
        Intent(
            'server_action_with_suggest',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['покажи кнопку']}]),
            dm_form=Form(
                'server_action_with_suggest',
                events=[Event('submit', handlers=[Handler(handler='callback', name='server_action_with_suggest')])],
            ),
        ),
        Intent(
            'message_with_suggest',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['скажи что-нибудь и покажи кнопку']}]),
            dm_form=Form(
                'message_with_suggest',
                events=[Event('submit', handlers=[Handler(handler='callback', name='message_with_suggest')])],
            ),
        ),
        Intent(
            'message_with_buttons',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['давай так чтобы кнопки не исчезали']}]),
            dm_form=Form(
                'message_with_buttons',
                events=[Event('submit', handlers=[Handler(handler='callback', name='message_with_buttons')])],
            ),
        ),
        Intent(
            'text_message_with_forced_voice',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['пишу текстом, но хочу слышать голос']}]),
            dm_form=Form(
                'text_message_with_forced_voice',
                events=[Event('submit', handlers=[Handler(handler='callback', name='text_message_with_forced_voice')])],
            ),
        ),
        Intent(
            'empty_response',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['совсем ничего']}]),
            dm_form=Form(
                'empty_response',
                events=[Event('submit', handlers=[Handler(handler='callback', name='empty_response')])],
            ),
        ),
        Intent(
            'location_response',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['я тут']}]),
            dm_form=Form(
                'location_response',
                events=[Event('submit', handlers=[Handler(handler='callback', name='location_response')])],
            ),
        ),
        Intent(
            'random_response',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['дай мне случайный ответ']}]),
            dm_form=Form(
                'random_response',
                events=[Event('submit', handlers=[Handler(handler='callback', name='random_response')])],
            ),
        ),
        Intent(
            'activate_external_skills__cities',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['запусти навык города']}]),
            dm_form=Form(
                'external_skill_cities_response',
                events=[Event('submit', handlers=[Handler(handler='callback', name='external_skill_cities_response')])],
            ),
        ),
        Intent(
            'search_response_continue',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['включи поиск с continue']}]),
            dm_form=Form(
                'search_response_continue',
                events=[Event('submit', handlers=[Handler(handler='callback', name='search_response_continue')])],
            ),
        ),
        Intent(
            'search_response_commit',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['включи поиск с commit']}]),
            dm_form=Form(
                'search_response_commit',
                events=[Event('submit', handlers=[Handler(handler='callback', name='search_response_commit')])],
            ),
        ),
        Intent(
            'weather_response',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['погода']}]),
            dm_form=Form(
                'weather_response',
                events=[Event('submit', handlers=[Handler(handler='callback', name='weather_response')])],
            ),
        ),
        Intent(
            'weather_response__ellipsis',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['а завтра']}]),
            dm_form=Form(
                'weather_response__ellipsis',
                events=[Event('submit', handlers=[Handler(handler='callback', name='weather_response')])],
            ),
        ),
        Intent(
            'analytics_info',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['аналитикс инфо']}]),
            dm_form=Form(
                'weather_response__ellipsis',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_analytics_info')])],
            ),
        ),
        Intent(
            'find_contact',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['find contact']}]),
            dm_form=Form(
                'find_contact',
                events=[Event('submit', handlers=[Handler(handler='callback', name='find_contact')])],
            ),
        ),
        Intent(
            'add_features',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['отсыпь фичей']}]),
            dm_form=Form(
                'add_features',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_features')])],
            ),
        ),
        Intent(
            'add_features__continue',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['а еще']}]),
            dm_form=Form(
                'add_features',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_features')])],
            ),
        ),
        Intent(
            'irrelevant',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['неуместный запрос']}]),
            dm_form=Form(
                'irrelevant_features',
                events=[Event('submit', handlers=[Handler(handler='callback', name='irrelevant_features')])],
            ),
        ),
        Intent(
            'personal_assistant.scenarios.common.irrelevant',
            trainable_classifiers=['intent_classifier_0'],
            dm_form=Form(
                'irrelevant_features',
                events=[Event('submit', handlers=[Handler(handler='callback', name='irrelevant_features')])],
            ),
        ),
        Intent(
            'not_irrelevant',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['not irrelevant']}]),
            dm_form=Form(
                'add_features',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_features')])],
            ),
        ),
        Intent(
            'get_time',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['который час']}]),
            dm_form=Form(
                'get_time',
                events=[Event('submit', handlers=[Handler(handler='callback', name='get_time')])]
            )
        ),
        Intent(
            'image_search',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['фотки котиков']}]),
            dm_form=Form(
                'image_search',
                events=[Event('submit', handlers=[Handler(handler='callback', name='image_search')])]
            )
        ),
        Intent(
            'error_meta',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['дай ошибку']}]),
            dm_form=Form(
                'some_error',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_error_meta')])],
            ),
        ),
        Intent(
            'gc_intent',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['давай поболтаем']}]),
            dm_form=Form(
                'pa.gc',
                events=[Event('submit', handlers=[Handler(handler='callback', name='add_gc_intent_meta')])],
            ),
        ),
        Intent(
            'text_with_suggest',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['дай саджест']}]),
            dm_form=Form(
                'text_with_suggest',
                events=[Event('submit', handlers=[Handler(handler='callback', name='text_with_suggest')])],
            ),
        ),
        Intent(
            'text_with_search_suggest',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['дай поисковый саджест']}]),
            dm_form=Form(
                'text_with_search_suggest',
                events=[Event('submit', handlers=[Handler(handler='callback', name='text_with_search_suggest')])],
            ),
        ),
        Intent(
            'player_continue_common',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['давай дальше играй']}]),
            dm_form=Form(
                'player_continue_common',
                events=[Event('submit', handlers=[Handler(handler='callback', name='set_intent_player_continue')])],
            ),
        ),
        Intent(
            'player_continue_music',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['следующую песню']}]),
            dm_form=Form(
                'player_continue_music',
                events=[
                    Event('submit', handlers=[Handler(handler='callback',
                                                      name='set_intent_player_continue_and_add_music_directive')])
                ],
            ),
        ),
        Intent(
            'player_continue_video',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['следующий фильм']}]),
            dm_form=Form(
                'player_continue_video',
                events=[
                    Event('submit', handlers=[Handler(handler='callback',
                                                      name='set_intent_player_continue_and_add_video_directive')])
                ],
            ),
        ),
        Intent(
            'text_with_frame_actions',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['дай фрейм экшны']}]),
            dm_form=Form(
                'text_with_frame_actions',
                events=[Event('submit', handlers=[Handler(handler='callback', name='text_with_frame_actions')])],
            ),
        ),
        Intent(
            'text_with_scenario_data',
            trainable_classifiers=['intent_classifier_0'],
            nlu_sources=NluSourcesConfig([{'source': 'data', 'data': ['данные сценария']}]),
            dm_form=Form(
                'text_with_scenario_data',
                events=[Event('submit', handlers=[Handler(handler='callback', name='text_with_scenario_data')])],
            ),
        ),
    ])])
    cfg.nlu['feature_extractors'] = [
        {'type': 'ngrams', 'id': 'word', 'n': 1},
        {'type': 'ngrams', 'id': 'bigram', 'n': 2},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'}
    ]
    cfg.nlu['intent_classifiers'] = [{
        'name': 'protocol_semantic_frame_classifier',
        'model': 'protocol_semantic_frame',
        'fallback_threshold': 0,
        'params': {
            'matching_score': 1,
        },
    }, {
        'name': 'irrelevant_classifier',
        'model': 'irrelevant',
        'fallback_threshold': 0,
        'params': {
            'matching_intent': 'personal_assistant.scenarios.common.irrelevant',
            'matching_score': 1,
        }
    }, {
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
def dm_mock(app_cfg_for_test):
    dm = DialogManager.from_config(app_cfg_for_test, load_data=True)
    dm.nlu.train()
    with mock.patch.object(DialogManager, 'from_config') as m:
        m.return_value = dm
        yield dm


@pytest.fixture(scope='module')
def sk_app(dm_mock):
    app = AppForTest(
        app_id='sk_app',
        dm=dm_mock,
        session_storage=SKSessionStorage(
            mongomock.MongoClient().test_db.sessions
        ),
        allow_wizard_request=True,
    )
    return TestConnector(vins_app=app)


@pytest.fixture
def sk_settings(app_cfg_for_test):
    settings.CONNECTED_APPS = {
        'test': {
            'app_config': app_cfg_for_test,
            'class': AppForTest,
        },
        'test_listen_by_default': {
            'app_config': app_cfg_for_test,
            'class': AppForTest,
        },
    }
    return settings


@pytest.fixture
def sk_client(sk_settings, dm_mock, mocker):
    mocker.patch('vins_api.common.resources.get_db_connection', return_value=mongomock.MongoClient().test_db)
    return falcon.testing.TestClient(make_app()[0])
