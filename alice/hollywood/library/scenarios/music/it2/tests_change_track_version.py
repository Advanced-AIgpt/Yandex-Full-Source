import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice, server_action
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    get_first_track_id,
    EXPERIMENTS,
)
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_music_change_track_version', 'bg_fresh_granet')
class TestsChangeTrackVersion:

    def test_change_track_version(self, alice):
        r = alice(voice('включи we will rock you'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)

        r = alice(voice('включи другую версию'))
        assert r.scenario_stages() == {'run', 'continue'}
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)

        assert second_track_id != first_track_id

        r = alice(voice('включи другую версию'))
        assert r.scenario_stages() == {'run', 'continue'}
        third_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)

        assert third_track_id != first_track_id and third_track_id != second_track_id

        return str(r)

    def test_change_track_version_to_original(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '44092997'
                    },
                    'object_type': {
                        'enum_value': 'Track'
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)

        r = alice(voice('включи оригинальную версию'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert second_track_id != first_track_id
        assert second_track_id == '1710811'
