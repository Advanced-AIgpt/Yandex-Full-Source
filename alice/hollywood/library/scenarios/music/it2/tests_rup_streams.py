import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import assert_audio_play_directive
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TContentId
from conftest import get_scenario_state


logger = logging.getLogger(__name__)
bass_stubber = create_localhost_bass_stubber_fixture()


# Это значение для 'Scenarios' параметра конфига Голливуда шарда 'all/test'
@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['music']


@pytest.mark.experiments('bg_fresh_granet', 'internal_music_player', 'station_promo_score=0',)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class TestRupStreams:

    @pytest.mark.parametrize('query', [
        pytest.param('включи мою волну', id='my-wave'),
        pytest.param('включи поток коллекция', id='collection'),
        pytest.param('включи поток хиты', id='hits'),
    ])
    @pytest.mark.parametrize('surface', [surface.station])
    def test_rup_streams(self, alice, query):
        r = alice(voice(query))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю поток \"' in layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')
        return str(r)

    @pytest.mark.parametrize('query, contentId', [
        pytest.param('включи мою волну', 'user:onyourwave', id='my-wave'),
        pytest.param('включи мою волну', 'user:onyourwave', id='my-wave-disabled-ichwill',
                     marks=pytest.mark.experiments('hw_music_thin_client_disable_ichwill_mywave')),
        pytest.param('включи поток коллекция', 'personal:collection', id='collection'),
        pytest.param('включи поток хиты', 'personal:hits', id='hits'),
    ])
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.experiments('hw_music_thin_client', 'mm_enable_stack_engine', 'new_music_radio_nlg')
    def test_rup_streams_thin(self, alice, query, contentId):
        r = alice(voice(query))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech
        assert_audio_play_directive(layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == contentId

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.experiments('hw_music_thin_client', 'mm_enable_stack_engine', 'new_music_radio_nlg')
    def test_rup_streams_thin_tsf(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'user:onyourwave',
                    },
                    'object_type': {
                        'enum_value': 'Radio',
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'from': {
                        'string_value': 'pult'
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == "user:onyourwave"

    @pytest.mark.parametrize('query, tag', [
        pytest.param('включи мою волну', 'onyourwave', id='my-wave'),
        pytest.param('включи поток коллекция', 'collection', id='collection'),
        pytest.param('включи поток хиты', 'hits', id='hits'),
    ])
    @pytest.mark.experiments('hw_music_sdk_client_navigator')  # TODO(sparkle): remove flag after release
    @pytest.mark.parametrize('surface', [surface.navi])
    def test_rup_streams_navi(self, alice, query, tag):
        r = alice(voice(query))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('OpenUriDirective')
        assert 'tag={}'.format(tag) in layout.Directives[0].OpenUriDirective.Uri
        return str(r)
