import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_static_stubber_fixture


logger = logging.getLogger(__name__)


EXPERIMENTS = [
    'music_check_plus_promo',
    'hw_music_thin_client',
]


billing_stubber = create_static_stubber_fixture(  # This overrides billing_stubber fixture from conftest.py
    '/billing/requestPlus',
    'alice/hollywood/library/scenarios/music/it2/billing_data/promo_incorrect_status.json'
)


@pytest.fixture(scope='function')  # This overrides srcrwr_params fixture from conftest.py
def srcrwr_params(bass_stubber, music_back_stubber, storage_mds_stubber, billing_stubber, apphost):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
        'HOLLYWOOD_MUSIC_MDS_PROXY': f'localhost:{storage_mds_stubber.port}',
        'MUSIC_SCENARIO_BILLING_PROXY': f'localhost:{billing_stubber.port}',

        # We need this additional srcrwr because 'stop/pause' command lives in 'fast_command' (aka Commands) scenario
        'Commands': f'localhost:{apphost.http_adapter_port}',
    }


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class TestsPromoIncorrectStatus:

    output_speech_thin = [
        'Чтобы слушать музыку, вам нужно оформить подписку Яндекс Плюс.'
        'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
        'Простите, я бы с радостью, но у вас нет подписки на Плюс.'
    ]
    output_speech_thick = 'Чтобы слушать музыку, вам нужно оформить подписку Яндекс Плюс.'

    def test_play_artist(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech in self.output_speech_thin
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 0
