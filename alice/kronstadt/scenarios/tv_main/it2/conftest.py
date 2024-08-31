import pytest

from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint


vh_stubber = create_stubber_fixture(
    'frontend.vh.yandex.ru',
    443,
    [
        StubberEndpoint('/player/{vh_uuid}.json', ['GET']),
        StubberEndpoint('/v23/carousel_videohub.json', ['GET']),
        StubberEndpoint('/v23/feed.json', ['GET']),
        StubberEndpoint('/v23/series_seasons.json', ['GET']),
        StubberEndpoint('/v23/series_episodes.json', ['GET']),
    ],
    scheme="https",
    stubs_subdir='player_stubs',
    hash_pattern=True
)
ott_stubber = create_stubber_fixture(
    'api.ott.yandex.net',
    443,
    [
        StubberEndpoint('/prestable/v12/selections/', ['GET']),
    ],
    scheme="https",
    stubs_subdir='ott_stubs',
    hash_pattern=True,
)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'tv_main'


@pytest.fixture(scope='function')
def srcrwr_params(vh_stubber, kronstadt_grpc_port, ott_stubber):
    return {
        'VIDEO_HOSTING_FEED_PROXY': f'http://localhost:{vh_stubber.port}:10000',
        'VIDEO_HOSTING_CAROUSEL_PROXY': f'http://localhost:{vh_stubber.port}:10000',
        'KINOPOISK_SELECTIONS': f'http://localhost:{ott_stubber.port}:10000',
        'TV_MAIN': f'localhost:{kronstadt_grpc_port}',
    }


@pytest.mark.scenario(name='TvMain', handle='tv_main')
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.additional_options(bass_options={
    'user_agent': 'com.yandex.io.sdk/2.96.20.5292 (Yandex YandexModule2-00001; Android 9)'
})
class TvMainScenarioTestingPreset:
    pass
