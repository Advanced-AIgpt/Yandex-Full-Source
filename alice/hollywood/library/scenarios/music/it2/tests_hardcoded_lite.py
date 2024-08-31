import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['hardcoded_music']


EXPERIMENTS = [
    'bg_fresh_granet'
]


@pytest.fixture(scope="function")  # This overrides srcrwr_params fixture from conftest.py
def srcrwr_params(bass_stubber):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}'
    }


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodHardcodedMusic', handle='hardcoded_music')
@pytest.mark.experiments(*EXPERIMENTS)
class TestHardcodedLitePlaylists:

    @pytest.mark.parametrize('surface', surface.all_station_lite)
    def test_alice_lite_hardcoded_playlist(self, alice):
        r = alice(voice('включи свою любимую музыку'))
        return str(r)

    @pytest.mark.parametrize('surface', [surface.station_lite_beige])
    @pytest.mark.experiments('hw_music_thin_client')
    def test_thin_client_alice_lite_hardcoded_playlist(self, alice):
        r = alice(voice('включи свою любимую музыку'))
        return str(r)
