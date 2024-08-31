import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint


SCENARIO_NAME = 'TvHome'
SCENARIO_HANDLE = 'tv_home'

droideka_backend_stubber = create_stubber_fixture(
    'droideka.tst.smarttv.yandex.net',
    443,
    [
        StubberEndpoint('/api/v7/categories', ['GET']),
        StubberEndpoint('/api/v7/carousels', ['GET']),
        StubberEndpoint('/api/v7/carousel', ['GET']),
    ],
    scheme='https',
    stubs_subdir='backend_stubs',
)


@pytest.fixture(scope="module")
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.fixture(scope="function")
def srcrwr_params(droideka_backend_stubber):
    return {
        'HOLLYWOOD_DROIDEKA_PROXY': f'http://localhost:{droideka_backend_stubber.port}:10000',
    }


@pytest.mark.scenario(name='TvHome', handle=SCENARIO_HANDLE)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestTvHome:
    def get_first_directive(self, payload, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        return directives[0]

    def test_get_categories_semantic_frame(self, alice):
        payload = {
            'typed_semantic_frame': {
                'get_smart_tv_categories_semantic_frame': {}
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_smarttv_categories'
            }
        }

        directive = self.get_first_directive(payload, alice).SetSmartTvCategoriesDirective
        assert len(directive.Categories) > 0, 'empty categories!'
        for item in directive.Categories:
            assert item.CategoryId
            assert item.Title
            assert item.Icon
            assert item.Rank > 0

    def check_carousel(self, carousel, requested_limit):
        assert len(carousel.Includes) == requested_limit

        for item in carousel.Includes:
            assert item.ContentId
            assert item.Title

    def test_get_smart_tv_carousel_semantic_frame(self, alice):
        items_limit = 3
        payload = {
            'typed_semantic_frame': {
                'get_smart_tv_carousel_semantic_frame': {
                    'carousel_id': {
                        'string_value': 'FRONTEND_CATEG_PROMO_MIXED'
                    },
                    'limit': {
                        'num_value': items_limit
                    },
                    'offset': {
                        'num_value': 0
                    },
                    'restriction_age': {
                        'num_value': 18
                    },
                    'kid_mode': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_smart_tv_carousel'
            }
        }

        directive = self.get_first_directive(payload, alice).TvSetSingleCarouselDirective
        self.check_carousel(directive, items_limit)

    def test_get_smart_tv_carousels_semantic_frame(self, alice):
        carousels_limit = 1
        items_limit = 2
        payload = {
            'typed_semantic_frame': {
                'get_smart_tv_carousels_semantic_frame': {
                    'category_id': {
                        'string_value': 'main'
                    },
                    'limit': {
                        'num_value': carousels_limit
                    },
                    'offset': {
                        'num_value': 0
                    },
                    'max_items_count': {
                        'num_value': items_limit
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_smart_tv_carousels'
            }
        }

        directive = self.get_first_directive(payload, alice).TvSetCarouselsDirective
        assert len(directive.Carousels) == carousels_limit
        for carousel in directive.Carousels:
            assert carousel.CarouselId
            assert carousel.Title

            self.check_carousel(carousel, items_limit)
