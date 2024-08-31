import logging


from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import EXPERIMENTS, EXPERIMENTS_PLAYLIST

from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState, ERepeatType, TContentId  # noqa

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice

from conftest import get_scenario_state

import pytest


logger = logging.getLogger(__name__)

AMBIENT_SOUND_EXPERIMENTS = [
    'bg_enable_ambient_sounds_in_music',
    'hw_music_enable_ambient_sound',
    'mm_disable_ambient_sound_preferred_vins_intent',
    'vins_add_irrelevant_intents=personal_assistant.scenarios.music_ambient_sound',
]


@pytest.fixture(scope="function")
def srcrwr_params(music_back_stubber):
    return {
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
        # make sure, that current tagger has ambient_slot, when running in generator mode and delete the next line
        # otherwise start begemot with the right tagger to add new tests or recanonize the existing ones
        # 'ALICE__BEGEMOT_WORKER_MEGAMIND': 'localhost:12341:10000'
    }


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*AMBIENT_SOUND_EXPERIMENTS, *EXPERIMENTS, *EXPERIMENTS_PLAYLIST)
@pytest.mark.parametrize('surface', [surface.station])
class TestAmbientSounds:
    @pytest.mark.parametrize('command,expected_response_text,content_id', [
        pytest.param('включи шум моря', 'Включаю подборку "Шум моря"', '103372440:1902', id='sea'),
        pytest.param('включи спокойную мурчание', 'Включаю подборку "Звук мурчания"', '103372440:1908', id='mood-with-ambient-sound'),
    ])
    @pytest.mark.skip(reason='slot "ambient_sound" is not in frame anymore, https://st.yandex-team.ru/HOLLYWOOD-1042')
    def test_typed_slot(self, alice, command, expected_response_text, content_id):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run', 'continue'}
        assert response.continue_response.ResponseBody.Layout.OutputSpeech == expected_response_text

        state = get_scenario_state(response.continue_response)
        playback_context = state.Queue.PlaybackContext
        assert playback_context.RepeatType == ERepeatType.RepeatAll
        assert playback_context.Shuffle
        assert playback_context.ContentId.Type == TContentId.EContentType.Playlist
        assert playback_context.ContentId.Id == content_id

        assert response.continue_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'music_ambient_sound'
