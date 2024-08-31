import pytest
import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import alice.tests.library.scenario as scenario
import alice.tests.library.directives as directives

VIDEO_URI = 'https://frontend.vh.yandex.ru/player/13991786694980472246?autoplay=1&amp;service=ya-video&amp;from=yavideo'
VIDEO_UID = '48ec62883cb1bfc8a65154fcd3749b72'
PLAY_ACTION = 'play'
OPEN_ACTION = 'open'

CONDITIONAL_ACTIONS_OPEN_VH_VIDEO = {
    'screen_conditional_action': {
        'some_screen': {
            'conditional_actions': [
                {
                    'conditional_semantic_frame': {
                        'player_continue_semantic_frame': {
                        }
                    },
                    'effect_frame_request_data': {
                        "analytics": {
                            "origin": "Scenario",
                            "product_scenario": "Video",
                            "purpose": "select_video_from_gallery"
                        },
                        'origin': {},
                        'typed_semantic_frame': {
                            'gallery_video_select_semantic_frame': {
                                'action': {
                                    'string_value': OPEN_ACTION
                                },
                                'provider_item_id': {
                                    'string_value': VIDEO_UID
                                }
                            }
                        }
                    }
                },
            ]
        },
    },
}


CONDITIONAL_ACTIONS_PLAY_VIDEO = {
    'screen_conditional_action': {
        'some_screen': {
            'conditional_actions': [
                {
                    'conditional_semantic_frame': {
                        'player_continue_semantic_frame': {
                        }
                    },
                    'effect_frame_request_data': {
                        "analytics": {
                            "origin": "Scenario",
                            "product_scenario": "Video",
                            "purpose": "select_video_from_gallery"
                        },
                        'origin': {},
                        'typed_semantic_frame': {
                            'gallery_video_select_semantic_frame': {
                                'action': {
                                    'string_value': PLAY_ACTION
                                },
                                'embed_uri': {
                                    'string_value': VIDEO_URI
                                }
                            }
                        }
                    }
                },
            ]
        },
    },
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('gallery_video_select', 'video_use_pure_hw')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestSelectItem(object):
    owners = ('nikitakeba', 'abc:smarttv')

    @pytest.mark.device_state(active_actions=CONDITIONAL_ACTIONS_OPEN_VH_VIDEO)
    def test_open_vh(self, alice):
        r = alice('продолжи')
        assert r.scenario == scenario.Video
        assert len(r.directives) == 1
        assert r.directives[0].name == directives.names.TvOpenDetailsScreenDirective

    @pytest.mark.device_state(active_actions=CONDITIONAL_ACTIONS_PLAY_VIDEO)
    def test_play(self, alice):
        r = alice('продолжи')
        assert r.scenario == scenario.Video
        assert len(r.directives) == 1
        assert r.directives[0].name == directives.names.VideoPlayDirective
