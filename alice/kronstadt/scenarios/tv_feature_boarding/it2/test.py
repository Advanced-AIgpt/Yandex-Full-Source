import base64
import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.protos.data.tv_feature_boarding.template_pb2 import TTemplateResponse
from hamcrest import assert_that, has_property, greater_than, equal_to


SCENARIO_NAME = 'TvFeatureBoarding'
SCENARIO_HANDLE = 'tv_feature_boarding'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'tv_feature_boarding'


@pytest.fixture(scope='function')
def srcrwr_params(kronstadt_grpc_port):
    return {
        'TV_FEATURE_BOARDING': f'localhost:{kronstadt_grpc_port}'
    }


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}')
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.memento({
    'UserConfigs': {
        'TandemPromoTemplateInfo': {
            'template_name': 'tandem_promo_template',
            'show_count': 0,
            'last_appearance_time': 0,
        }
    }
})
@pytest.mark.additional_options({
    'server_time_ms': 100500
})
class TestTvFeatureBoarding:
    def test_get_feature_boarding_template(self, alice):
        payload = {
            'typed_semantic_frame': {
                'tv_promo_request_semantic_frame': {
                    'chosen_template': {
                        'tandem_promo_template': {
                            'is_tandem_devices_available': True,
                            'is_tandem_connected': False
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_tv_feature_boarding_template'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        return str(r)

    def test_report_promo_template_shown(self, alice):
        payload = {
            'typed_semantic_frame': {
                'tv_promo_template_shown_report_semantic_frame': {
                    'chosen_template': {'tandem_promo_template': {}}
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'report_tv_feature_boarding_template_shown'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}
        return str(r)

    def test_get_feature_boarding_template_has_ttl(self, alice):
        payload = {
            'typed_semantic_frame': {
                'tv_promo_request_semantic_frame': {
                    'chosen_template': {
                        'tandem_promo_template': {
                            'is_tandem_devices_available': True,
                            'is_tandem_connected': False
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_tv_feature_boarding_template'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        request_proto = TTemplateResponse()
        request_proto.ParseFromString(base64.b64decode(r.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        assert_that(request_proto, has_property('Ttl', greater_than(0)))
        return str(request_proto)

    @pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}', 'zero_fb_ttl')
    def test_get_feature_boarding_template_no_ttl(self, alice):
        payload = {
            'typed_semantic_frame': {
                'tv_promo_request_semantic_frame': {
                    'chosen_template': {
                        'tandem_promo_template': {
                            'is_tandem_devices_available': True,
                            'is_tandem_connected': False
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_tv_feature_boarding_template'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        request_proto = TTemplateResponse()
        request_proto.ParseFromString(base64.b64decode(r.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        assert_that(request_proto.Ttl, equal_to(0))
        return str(request_proto)


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestNoTemplateTvFeatureBoarding:
    SERVER_ACTION = server_action(name='@@mm_semantic_frame', payload={
        'typed_semantic_frame': {
            'tv_promo_request_semantic_frame': {
                'chosen_template': {
                    'tandem_promo_template': {
                        'is_tandem_devices_available': True,
                        'is_tandem_connected': False
                    }
                }
            }
        },
        'analytics': {
            'origin': 'Scenario',
            'purpose': 'get_tv_feature_boarding_template'
        }
    })

    @pytest.mark.memento({
        'UserConfigs': {
            'TandemPromoTemplateInfo': {
                'template_name': 'tandem_promo_template',
                'show_count': 0,
                'last_appearance_time': 100500,
            }
        }
    })
    @pytest.mark.additional_options({
        'server_time_ms': 100501
    })
    def test_too_much_template_in_1_day(self, alice):
        r = alice(self.SERVER_ACTION)

        assert r.scenario_stages() == {'run'}
        return str(r)

    @pytest.mark.memento({
        'UserConfigs': {
            'TandemPromoTemplateInfo': {
                'template_name': 'tandem_promo_template',
                'show_count': 1,
                'last_appearance_time': 0,
            }
        }
    })
    @pytest.mark.additional_options({
        'server_time_ms': 9999999999
    })
    def test_show_limit_exceed(self, alice):
        r = alice(self.SERVER_ACTION)

        assert r.scenario_stages() == {'run'}
        return str(r)

    @pytest.mark.memento({
        'UserConfigs': {
            'TandemPromoTemplateInfo': {
                'template_name': 'tandem_promo_template',
                'show_count': 0,
                'last_appearance_time': 0,
            }
        }
    })
    @pytest.mark.additional_options({
        'server_time_ms': 100500
    })
    def test_tandem_already_connected(self, alice):
        payload = {
            'typed_semantic_frame': {
                'tv_promo_request_semantic_frame': {
                    'chosen_template': {
                        'tandem_promo_template': {
                            'is_tandem_devices_available': True,
                            'is_tandem_connected': True
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_tv_feature_boarding_template'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        return str(r)
