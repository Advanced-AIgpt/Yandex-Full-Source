import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_bass_stubber_fixture

from conftest import videobass_request_content_hasher


SCENARIO_NAME = 'Video'
SCENARIO_HANDLE = 'video'
bass_stubber = create_bass_stubber_fixture(custom_request_content_hasher=videobass_request_content_hasher)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.fixture(scope='function')
def srcrwr_params(bass_stubber):
    return {'ALICE__SCENARIO_VIDEO_NEW_PROXY': f'localhost:{bass_stubber.port}'}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.device_state(
    {
        'is_tv_plugged_in': True,
        'video': {
            'current_screen': 'main',
        },
    }
)
class TestStationCommands:
    def test_search(self, alice):
        r = alice(voice('Включи фильмы'))
        return str(r)

    def test_play(self, alice):
        r = alice(voice('Найди Моана'))
        return str(r)
