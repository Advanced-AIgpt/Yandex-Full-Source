import pytest
import json

from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint, HttpResponseStub
from google.protobuf.json_format import MessageToDict
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse
from dj.services.alisa_skills.server.proto.client.proactivity_request_pb2 import TProactivityRequest
from dj.services.alisa_skills.server.proto.client.onboarding_response_pb2 import TOnboardingResponse


def request_content_hasher(headers, content):
    content_obj = json.loads(content.decode('utf-8'))
    content_obj.pop('timetz', None)
    content_obj.pop('rng_seed', None)
    content_obj['alice_experiments'].sort()
    content_obj['platform_features'].sort()
    sorted_json = json.dumps(content_obj, ensure_ascii=False, sort_keys=True, indent=None)
    return sorted_json.encode('utf-8')

greetings_stubber = create_stubber_fixture(
    'skills-rec-test.alice.yandex.net', 80,
    [StubberEndpoint('/greetings', ['POST'], cgi_filter_regexps=['msid'])],
    type_to_proto={
        'pseudo_grpc_request': TProactivityRequest,
        'pseudo_grpc_response': TOnboardingResponse,
    },
    stubs_subdir='greetings',
    header_filter_regexps=['x-request-id', 'content-length']
)

div_render_stubber = create_stubber_fixture(
    '7wp6wqu2dfb5naua.sas.yp-c.yandex.net', 10000, [StubberEndpoint('/render', ['POST'])], stubs_subdir='div_render',
    type_to_proto={
        'render_data': TDivRenderData,
        'render_result': TRenderResponse,
    },
    pseudo_grpc=True,
    header_filter_regexps=['content-length']
)


@pytest.fixture(scope="function")
def srcrwr_params(greetings_stubber, div_render_stubber):
    return {
        'SKILL_PROACTIVITY_HTTP': f'localhost:{greetings_stubber.port}',
        'DIV_RENDERER': f'localhost:{div_render_stubber.port}'
    }


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['onboarding']


@pytest.mark.scenario(name='Onboarding', handle='onboarding')
class Tests:
    skills_request_node_name = 'ALICE_SKILLS_PROXY'
    supported_surfaces = [surface.searchapp]

    @pytest.mark.parametrize('surface', list(set(surface.actual_surfaces) - set(supported_surfaces)))
    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    def test_unsupported(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'Scenario'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        body = r.run_response.ResponseBody
        assert MessageToDict(body.AnalyticsInfo) == {
            'product_scenario_name': 'onboarding',
            'nlg_render_history_records': [
                {'template_name': 'onboarding_common', 'phrase_name': 'notsupported', 'language': 'L_RUS'}
            ],
        }
        assert not r.sources_dump.get_http_request(self.skills_request_node_name)

    @pytest.mark.parametrize('surface', supported_surfaces)
    def test_disabled(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        body = r.run_response.ResponseBody
        assert MessageToDict(body.AnalyticsInfo) == {
            'product_scenario_name': 'onboarding',
            'nlg_render_history_records': [
                {'template_name': 'onboarding_common', 'phrase_name': 'nothing_to_do', 'language': 'L_RUS'}
            ],
        }
        assert not r.sources_dump.get_http_request(self.skills_request_node_name)

    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    def test_supported(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        analytics_info = body.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'onboarding'
        assert analytics_info.Intent == 'alice.onboarding.get_greetings'
        # no cards, no speech, only directives
        assert MessageToDict(body.Layout) == {
            'directives': [
                {
                    'show_buttons': {
                        'buttons': [
                            {
                                'action_id': 'action_0',
                                'theme': {
                                    'image_url': 'https://avatars.mds.yandex.net/get-dialogs/758954/c2c04da829a68188a5c5/mobile-logo-round-x1'
                                },
                                'title': 'запусти навык Угадай мультфильмы',
                            },
                            {
                                'action_id': 'action_1',
                                'theme': {
                                    'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1530877/radioband/mobile-logo-round-x1'
                                },
                                'title': 'Включи «Подкасты недели»',
                            },
                            {
                                'action_id': 'action_2',
                                'theme': {
                                    'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1676983/goods_best_prices/mobile-logo-round-x1'
                                },
                                'title': 'Узнать, где дешевле',
                            },
                        ],
                        'screen_id': 'cloud_ui',
                    }
                },
            ],
            'should_listen': True,
        }

        actions = body.FrameActions
        assert len(actions) == 3
        assert MessageToDict(actions['action_0'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'запусти навык Угадай мультфильмы'}
        }
        assert MessageToDict(actions['action_1'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Включи «Подкасты недели»'}
        }
        assert MessageToDict(actions['action_2'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Узнать, где дешевле'}
        }
        # test logging
        for action in actions:
            callback = MessageToDict(actions[action])['directives']['list'][1]['callback_directive']
            assert callback['name'] == 'on_card_action'
            set(callback['payload'].keys()) == set(['request_id', 'card_id', 'case_name', 'intent_name', 'item_number'])

    @pytest.mark.experiments('hw_onboarding_enable_greetings')
    @pytest.mark.parametrize('surface', supported_surfaces)
    def test_image_url_sent(self, alice):
        """Tests image url creation for old backend. Request to /recommender fails, fallback values are taken"""
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        layout = body.Layout
        directive = layout.Directives[0]
        buttons = directive.ShowButtonsDirective.Buttons
        assert buttons
        assert MessageToDict(buttons[0].Theme) == {'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1535439/onboard_Wheather/mobile-logo-x1'}
        assert MessageToDict(buttons[1].Theme) == {'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-x1'}
        assert MessageToDict(buttons[2].Theme) == {'image_url': 'https://avatars.mds.yandex.net/get-dialogs/1535439/onboard_Map/mobile-logo-x1'}

    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_disable_greetings_images', 'hw_onboarding_greetings_use_updated_backend')
    @pytest.mark.parametrize('surface', supported_surfaces)
    def test_image_url_omitted(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        layout = body.Layout
        directive = layout.Directives[0]
        buttons = directive.ShowButtonsDirective.Buttons
        assert MessageToDict(buttons[0]) == {'title': 'Включи «Подкасты недели»', 'action_id': 'action_0'}
        assert MessageToDict(buttons[1]) == {'title': 'запусти навык Угадай мультфильмы', 'action_id': 'action_1'}
        assert MessageToDict(buttons[2]) == {'title': 'Узнать, где дешевле', 'action_id': 'action_2'}

    @pytest.mark.supported_features('show_view_layer_content', 'show_view_layer_footer')
    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    def test_supported_div(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        analytics_info = body.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'onboarding'
        assert analytics_info.Intent == 'alice.onboarding.get_greetings'

        directives = MessageToDict(body.Layout)['directives']
        assert len(directives) == 2
        assert directives[0]['show_view']['layer_name'] == 'content'
        assert directives[1]['show_view']['layer_name'] == 'footer'

        actions = body.FrameActions
        assert actions
        assert MessageToDict(actions['div_action_0'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Запусти навык Мой тамагочи'}
        }
        assert MessageToDict(actions['div_action_1'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Курс доллара'}
        }
        assert MessageToDict(actions['div_action_2'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Какие сейчас пробки?'}
        }
        # test logging
        for action in actions:
            callback = MessageToDict(actions[action])['directives']['list'][1]['callback_directive']
            assert callback['name'] == 'on_card_action'
            set(callback['payload'].keys()) == set(['request_id', 'card_id', 'case_name', 'intent_name', 'item_number'])

    @pytest.mark.supported_features('show_view_layer_content')
    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    def test_content_layer_supported(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp

        body = r.run_response.ResponseBody
        directives = MessageToDict(body.Layout)['directives']
        assert len(directives) == 1
        assert directives[0]['show_view']['layer_name'] == 'content'

        assert MessageToDict(body.Layout) == {
            'directives': [
                {
                'show_view': {
                    'div2_card': {
                    'body': {
                        'card': {
                        'log_id': 'onboarding_card',
                        'states': [
                            {
                            'div': {
                                'column_count': 2,
                                'items': [
                                {
                                    'action': {
                                    'log_id': 'button/0',
                                    'url': '@@mm_deeplink#div_action_0'
                                    },
                                    'margins': {
                                    'bottom': 6
                                    },
                                    'text': 'Сколько ехать до дома?',
                                    'type': 'onboardingGreeting'
                                },
                                {
                                    'action': {
                                    'log_id': 'button/1',
                                    'url': '@@mm_deeplink#div_action_1'
                                    },
                                    'margins': {
                                    'bottom': 6
                                    },
                                    'text': 'Расскажи сказку',
                                    'type': 'onboardingGreeting'
                                },
                                {
                                    'action': {
                                    'log_id': 'button/2',
                                    'url': '@@mm_deeplink#div_action_2'
                                    },
                                    'margins': {
                                    'bottom': 6,
                                    'left': 6
                                    },
                                    'text': 'Погода на завтра',
                                    'type': 'onboardingGreeting'
                                }
                                ],
                                'margins': {
                                'bottom': 10
                                },
                                'paddings': {
                                'left': 24,
                                'right': 24
                                },
                                'type': 'gallery'
                            },
                            'state_id': 0
                            }
                        ],
                        'variable_triggers': [],
                        'variables': []
                        },
                        'templates': {
                        'onboardingGreeting': {
                            'background': [
                            {
                                'color': '#fff',
                                'type': 'solid'
                            }
                            ],
                            'border': {
                            'corners_radius': {
                                'bottom-left': 16,
                                'bottom-right': 16,
                                'top-left': 2,
                                'top-right': 16
                            },
                            'stroke': {
                                'color': '#e6e1f5',
                                'width': 1
                            }
                            },
                            'font_size': 14,
                            'font_weight': 'medium',
                            'line_height': 16,
                            'paddings': {
                            'bottom': 14,
                            'left': 18,
                            'right': 18,
                            'top': 14
                            },
                            'text_color': '#000',
                            'type': 'text',
                            'width': {
                            'type': 'wrap_content'
                            }
                        }
                        }
                    }
                    },
                    "layer_name": "content",
                    "screen_id": "cloud_ui"
                }
                },
            ],
            'should_listen': True
        }

    @pytest.mark.supported_features('show_view_layer_footer')
    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    def test_footer_layer_supported(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp

        body = r.run_response.ResponseBody
        directives = MessageToDict(body.Layout)['directives']
        assert len(directives) == 2
        assert directives[0]['show_buttons']
        assert directives[1]['show_view']['layer_name'] == 'footer'

        assert directives[1] == {
            'show_view': {
                'div2_card': {
                'body': {
                    'card': {
                    'log_id': 'skills_card',
                    'states': [
                        {
                        'div': {
                            'items': [
                            {
                                'action': {
                                'log_id': 'item/0',
                                'url': '@@mm_deeplink#div_action_3'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_camera/orig',
                                'title': 'Умная камера',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/1',
                                'url': '@@mm_deeplink#div_action_4'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/1027858/onb_skill_chat/orig',
                                'title': 'Чат с Алисой',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/2',
                                'url': '@@mm_deeplink#div_action_5'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/1027858/onb_skill_what/orig',
                                'title': 'Что ты умеешь?',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/3',
                                'url': '@@mm_deeplink#div_action_6'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_music/orig',
                                'title': 'Включи музыку',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/4',
                                'url': '@@mm_deeplink#div_action_7'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/998463/onb_skill_smart_home/orig',
                                'title': 'Умный дом',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/5',
                                'url': '@@mm_deeplink#div_action_8'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/399212/onb_skill_shazam/orig',
                                'title': 'Что это играет?',
                                'type': 'aliceSkillsGalleryItem',
                            },
                            {
                                'action': {
                                'log_id': 'item/6',
                                'url': '@@mm_deeplink#div_action_9'
                                },
                                'iconUrl': 'https://avatars.mds.yandex.net/get-dialogs/758954/onb_skill_games/orig',
                                'title': 'Игры',
                                'type': 'aliceSkillsGalleryItem',
                            }
                            ],
                            'margins': {
                            'top': 22
                            },
                            'paddings': {
                            'left': 24,
                            'right': 24
                            },
                            'type': 'gallery'
                        },
                        'state_id': 0
                        }
                    ],
                    'variable_triggers': [],
                    'variables': []
                    },
                    'templates': {
                    'aliceSkillsGalleryItem': {
                        'background': [
                        {
                            'color': '#f1effd',
                            'type': 'solid'
                        }
                        ],
                        "border": {
                            "corner_radius": 20
                        },
                        "height": {
                            "type": "fixed",
                            "unit": "sp",
                            "value": 100
                        },
                        "items": [
                            {
                                "$text": "title",
                                "font_size": 14,
                                "font_weight": "medium",
                                "line_height": 16,
                                "margins": {
                                    "bottom": 14,
                                    "left": 12,
                                    "right": 12,
                                    "top": 10
                                },
                                "max_lines": 2,
                                "text_color": "#000000",
                                "type": "text"
                            },
                            {
                                "$image_url": "iconUrl",
                                "alignment_vertical": "bottom",
                                "height": {
                                    "type": "fixed",
                                    "unit": "sp",
                                    "value": 42
                                },
                                "type": "image",
                                "width": {
                                    "type": "fixed",
                                    "unit": "sp",
                                    "value": 44
                                }
                            }
                        ],
                        "orientation": "overlap",
                        "type": "container",
                        "width": {
                            "type": "fixed",
                            "unit": "sp",
                            "value": 100
                        }
                    }
                }
                }
                },
                "layer_name": "footer",
                "screen_id": "cloud_ui"
            }
        }

    @pytest.mark.experiments('hw_onboarding_enable_greetings', 'hw_onboarding_greetings_use_updated_backend')
    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.freeze_stubs(greetings_stubber={
        '/greetings': [
            HttpResponseStub(200, 'freeze_stubs/greetings_handler_empty_response.json'),
        ],
    })
    def test_greetings_handler_fallback(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        actions = body.FrameActions
        assert len(actions) == 3
        assert MessageToDict(actions['action_0'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Погода на завтра'}
        }
        assert MessageToDict(actions['action_1'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Расскажи сказку'}
        }
        assert MessageToDict(actions['action_2'])['directives']['list'][0] == {
            'type_text_directive': {'text': 'Где поужинать?'}
        }

    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_enable_greetings')
    def test_greetings_logging(self, alice):
        payload = {
            'typed_semantic_frame': {'onboarding_get_greetings_semantic_frame': {}},
            'analytics': {'purpose': 'onboarding', 'origin': 'SearchApp'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        body = r.run_response.ResponseBody
        actions = body.FrameActions
        assert len(actions) == 3
        assert MessageToDict(actions['action_0'])['directives']['list'] == [
            {'type_text_directive': {'text': 'Погода на завтра'}},
            {
                'callback_directive': {
                    'ignore_answer': True,
                    'name': 'on_card_action',
                    'payload': {
                        'request_id': '45075214-ff91-5b6c-b6f1-b8dabbadoo00',
                        'card_id': 'skill_recommendation',
                        'case_name': 'skill_recommendation__get_greetings__editorial#__onboarding_weather3',
                        'intent_name': 'personal_assistant.scenarios.skill_recommendation',
                        'item_number': '1',
                    }
                }
            }
        ]
        assert MessageToDict(actions['action_1'])['directives']['list'] == [
            {'type_text_directive': {'text': 'Расскажи сказку'}},
            {
                'callback_directive': {
                    'ignore_answer': True,
                    'name': 'on_card_action',
                    'payload': {
                        'request_id': '45075214-ff91-5b6c-b6f1-b8dabbadoo00',
                        'card_id': 'skill_recommendation',
                        'case_name': 'skill_recommendation__get_greetings__editorial#__onboarding_music_fairy_tale2',
                        'intent_name': 'personal_assistant.scenarios.skill_recommendation',
                        'item_number': '2',
                    }
                }
            }
        ]
        assert MessageToDict(actions['action_2'])['directives']['list'] == [
            {'type_text_directive': {'text': 'Где поужинать?'}},
            {
                'callback_directive': {
                    'ignore_answer': True,
                    'name': 'on_card_action',
                    'payload': {
                        'request_id': '45075214-ff91-5b6c-b6f1-b8dabbadoo00',
                        'card_id': 'skill_recommendation',
                        'case_name': 'skill_recommendation__get_greetings__editorial#__onboarding_find_poi2',
                        'intent_name': 'personal_assistant.scenarios.skill_recommendation',
                        'item_number': '3',
                    }
                }
            }
        ]
