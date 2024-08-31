import pytest
from alice.hollywood.library.framework.proto.framework_state_pb2 import TProtoHwFramework
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_localhost_bass_stubber_fixture, \
    create_stubber_fixture
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse


pytest.register_assert_rewrite(
    'alice.hollywood.library.scenarios.music.it2.thin_client_helpers',
)


@pytest.fixture(scope='module')
def enabled_scenarios():
    # We need 'fast_command' because 'stop/pause' command lives there in (Commands scenario)
    return ['music', 'fast_command']


bass_stubber = create_localhost_bass_stubber_fixture()

music_back_stubber = create_stubber_fixture(
    'music-web-ext.music.yandex.net',
    80,
    [
        StubberEndpoint('/internal-api/artists/{artist_id}/tracks', ['GET']),
        StubberEndpoint('/internal-api/artists/{artist_id}/track-ids', ['GET']),
        StubberEndpoint('/internal-api/artists/{artist_id}/direct-albums', ['GET']),
        StubberEndpoint('/internal-api/albums/{album_id}/with-tracks', ['GET']),
        StubberEndpoint('/internal-api/tracks/{track_id}', ['GET']),
        StubberEndpoint('/internal-api/tracks/{track_id}/full-info', ['GET']),
        StubberEndpoint('/internal-api/playlists/personal/{playlist_id}', ['GET']),
        StubberEndpoint('/internal-api/search', ['GET']),
        StubberEndpoint('/internal-api/users/{user_id}/playlists/{playlist_id}', ['GET']),
        StubberEndpoint('/internal-api/plays', ['POST']),
        StubberEndpoint('/internal-api/streams/progress/save-current', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/albums/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/artists/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/dislikes/artists/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/genres/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/tracks', ['GET']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/tracks/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/tracks/{track_id}/remove', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/dislikes/tracks', ['GET']),
        StubberEndpoint('/internal-api/users/{user_id}/dislikes/tracks/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/dislikes/tracks/{track_id}/remove', ['POST']),
        StubberEndpoint('/internal-api/tracks/{track_id}/download-info', ['GET'], idempotent=False, cgi_filter_regexps=['sign']),
        StubberEndpoint('/internal-api/after-track', ['GET'], idempotent=False),
        StubberEndpoint('/internal-api/shots/feedback', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/likes/shots/add', ['POST']),
        StubberEndpoint('/internal-api/users/{user_id}/dislikes/shots/add', ['POST']),
        StubberEndpoint('/internal-api/infinite-feed', ['GET']),
        StubberEndpoint('/internal-api/radio-stream/{fm_radio_id}', ['GET']),
        StubberEndpoint('/internal-api/radio-stream/available/list', ['GET']),
        StubberEndpoint('/internal-api/radio-stream/ranked/list', ['GET']),
        StubberEndpoint('/internal-api/children-landing/catalogue', ['GET']),
        StubberEndpoint('/external-rotor/session/new', ['POST'], idempotent=False),
        StubberEndpoint('/external-rotor/session/{radio_session_id}/feedback', ['POST']),
        StubberEndpoint('/external-rotor/session/{radio_session_id}/tracks', ['POST'], idempotent=False),
        StubberEndpoint('/external-rotor/station/{station_id}/feedback', ['POST']),
        StubberEndpoint('/external-rotor/station/{station_id}/stream', ['GET'], idempotent=False),
        StubberEndpoint('/external-rotor/station/{station_id}/tracks', ['GET'], idempotent=False),
    ],
    stubs_subdir='music_back',
    header_filter_regexps=['x-yandex-music-client', 'x-yandex-music-device'],
)

storage_mds_stubber = create_stubber_fixture(
    'storage.mds.yandex.net',
    80,  # NOTE: Stubber (and requests library under the hood of it) cuts the default ports
    # so this works fine with MDS, see MDSSUPPORT-605
    [
        StubberEndpoint('/file-download-info/{id1}/{id2}', ['GET']),
        StubberEndpoint('/file-download-info/{id1}/{id2}/{id3}', ['GET']),
    ],
    stubs_subdir='storage_mds',
)

billing_stubber = create_stubber_fixture(
    'paskills-common-testing.alice.yandex.net',
    80,
    [
        StubberEndpoint('/billing/requestPlus', ['POST']),
    ],
    scheme='http',
    stubs_subdir='billing',
)


div_render_graph_stubber = create_stubber_fixture(
    ('sas', 'div-renderer-prestable.div-renderer'),
    10000,
    [
        StubberEndpoint('/render', ['POST']),
    ],
    stubs_subdir='div_render_back',
    type_to_proto={
        'render_data': TDivRenderData,
        'render_result': TRenderResponse,
    },
    pseudo_grpc=True,
    header_filter_regexps=['content-length'],
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


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, music_back_stubber, storage_mds_stubber, billing_stubber, div_render_graph_stubber,
                  avatars_back_stubber, apphost):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_INFINITE_FEED_PROXY': f'localhost:{music_back_stubber.port}',
        'HOLLYWOOD_MUSIC_MDS_PROXY': f'localhost:{storage_mds_stubber.port}',
        'MUSIC_SCENARIO_BILLING_PROXY': f'localhost:{billing_stubber.port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
        'ALICE__AVATARS_MDS_PROXY': f'localhost:{avatars_back_stubber.port}',

        # We need this additional srcrwr because 'stop/pause' command lives in 'fast_command' (aka Commands) scenario
        'Commands': f'localhost:{apphost.http_adapter_port}',
    }


def get_scenario_state(response):
    state = TScenarioState()
    if not response.ResponseBody.State.Unpack(state):
        hw_framework_proto = TProtoHwFramework()
        if response.ResponseBody.State.Unpack(hw_framework_proto):
            hw_framework_proto.ScenarioState.Unpack(state)
    return state
