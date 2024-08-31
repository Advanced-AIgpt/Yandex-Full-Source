import pytest
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_localhost_bass_stubber_fixture, \
    create_stubber_fixture
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState  # noqa


pytest.register_assert_rewrite(
    'alice.hollywood.library.scenarios.music.it2_music_client.music_sdk_helpers',
)


bass_stubber = create_localhost_bass_stubber_fixture()

music_back_stubber = create_stubber_fixture(
    'music-web-ext.music.yandex.net',
    80,
    [
        StubberEndpoint('/internal-api/artists/{artist_id}/brief-info', ['GET']),
        StubberEndpoint('/internal-api/artists/{artist_id}/tracks', ['GET']),
        StubberEndpoint('/internal-api/genre-overview', ['GET']),
        StubberEndpoint('/internal-api/playlists/personal/{playlist_id}', ['GET']),
        StubberEndpoint('/internal-api/search', ['GET']),
        StubberEndpoint('/internal-api/tracks/{track_id}', ['GET']),
        StubberEndpoint('/internal-api/users/{user_id}/playlists/{playlist_id}', ['GET']),
    ],
    stubs_subdir='music_back',
    header_filter_regexps=['x-yandex-music-client', 'x-yandex-music-device'],
)

avatars_back_stubber = create_stubber_fixture(
    'avatars-int.mds.yandex.net',
    13000,
    [
        StubberEndpoint('/getinfo-music-content/{group_id}/{image_name}/meta', ['GET']),
        StubberEndpoint('/getinfo-music-user-playlist/{group_id}/{image_name}/meta', ['GET']),
    ],
    stubs_subdir='avatars_back',
)


@pytest.fixture(scope="module")
def enabled_scenarios():
    # We need 'fast_command' because 'stop/pause' command lives there in (Commands scenario)
    return ['music', 'fast_command']


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, music_back_stubber, avatars_back_stubber):
    return {
        'ALICE__AVATARS_MDS_PROXY': f'localhost:{avatars_back_stubber.port}',
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
    }
