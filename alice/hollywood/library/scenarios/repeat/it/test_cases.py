from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice

from alice.megamind.protos.common.data_source_type_pb2 import RESPONSE_HISTORY
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest

from google.protobuf import text_format


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/repeat/it/data/'

SCENARIO_NAME = 'Repeat'
SCENARIO_HANDLE = 'repeat'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = [
    'quasar', 'launcher',
]

DEFAULT_EXPERIMENTS = []


DEVICE_STATE_WITH_PLAYER = {
    'is_tv_plugged_in': False,
    'mics_muted': False,
    'music': {
        'currently_playing': {
            'last_play_timestamp': 1601192720211,
            'track_id': '32773082',
            'track_info': {
                'albums': [
                    {
                        'artists': [
                            {
                                'composer': False,
                                'cover': {
                                    'prefix': 'c1def05d.a.3995724-2',
                                    'type': 'from-album-cover',
                                    'uri': 'avatars.yandex.net/get-music-content/2808981/c1def05d.a.3995724-2/%%'
                                },
                                'genres': [],
                                'id': 2941392,
                                'name': 'Маша и медведь',
                                'various': False
                            }
                        ],
                        'available': True,
                        'availableForMobile': True,
                        'availableForPremiumUsers': True,
                        'availablePartially': False,
                        'bests': [
                            32773061,
                            32773062,
                            32773064
                        ],
                        'buy': [],
                        'coverUri': 'avatars.yandex.net/get-music-content/2357076/508aa40e.a.3995727-2/%%',
                        'genre': 'animated',
                        'id': 3995727,
                        'labels': [
                            {
                                'id': 915148,
                                'name': 'Animaccord LTD 2008-2016'
                            }
                        ],
                        'metaType': 'music',
                        'ogImage': 'avatars.yandex.net/get-music-content/2357076/508aa40e.a.3995727-2/%%',
                        'recent': False,
                        'releaseDate': '2017-01-01T00:00:00+03:00',
                        'title': '«Маша и Медведь». Часть 1',
                        'trackCount': 28,
                        'trackPosition': {
                            'index': 22,
                            'volume': 1
                        },
                        'version': 'саундтрек к мультфильму',
                        'veryImportant': False,
                        'year': 2017
                    }
                ],
                'artists': [
                    {
                        'composer': False,
                        'cover': {
                            'prefix': 'c1def05d.a.3995724-2',
                            'type': 'from-album-cover',
                            'uri': 'avatars.yandex.net/get-music-content/2808981/c1def05d.a.3995724-2/%%'
                        },
                        'genres': [],
                        'id': 2941392,
                        'name': 'Маша и медведь',
                        'various': False
                    }
                ],
                'available': True,
                'availableForPremiumUsers': True,
                'availableFullWithoutPermission': False,
                'batchId': 'sync-7a626d94-36ea-4a12-9a54-41f7b883f064',
                'batchInfo': {
                    'albumId': '3995727',
                    'durationMs': 56530,
                    'general': False,
                    'itemType': 'track',
                    'rid': '2b1ee033-cde0-4e9f-8c90-fe94b8e6cd5a',
                    'type': 'dynamic'
                },
                'coverUri': 'avatars.yandex.net/get-music-content/2357076/508aa40e.a.3995727-2/%%',
                'durationMs': 56530,
                'fileSize': 0,
                'id': '32773082',
                'lyricsAvailable': False,
                'major': {
                    'id': 123,
                    'name': 'IRICOM'
                },
                'normalization': {
                    'gain': -2.49,
                    'peak': 28573
                },
                'ogImage': 'avatars.yandex.net/get-music-content/2357076/508aa40e.a.3995727-2/%%',
                'previewDurationMs': 30000,
                'realId': '32773082',
                'rememberPosition': False,
                'storageDir': '',
                'title': 'День варенья',
                'type': 'music'
            }
        },
        'last_play_timestamp': 1601192720211,
        'player': {
            'pause': False
        },
        'playlist_owner': '',
        'session_id': 'y1E1RBDd'
    },
    'radio': {
        'currently_playing': {
            'last_play_timestamp': 1601192700201,
            'radioId': 'retro_fm',
            'radioTitle': 'Ретро FM'
        },
        'last_play_timestamp': 1601192700201,
        'player': {
            'pause': True,
            'timestamp': 1601192720
        },
        'playlist_owner': ''
    },
    'smart_activation': True,
    'sound_level': 6,
    'sound_muted': False,
    'timers': {
        'active_timers': []
    },
    'tof': True,
    'video': {
        'current_screen': 'music_player'
    }
}


def _include_response_history(run_request):
    run_request_proto = TScenarioRunRequest()
    text_format.Merge(run_request, run_request_proto)

    run_request_proto.DataSources[RESPONSE_HISTORY].ResponseHistory.PrevResponse.Layout.OutputSpeech = 'foobar'

    return text_format.MessageToString(run_request_proto, as_utf8=True)


TESTS_DATA = {
    'repeat_with_context': {
        'input_dialog': [
            voice('повтори', request_patcher=_include_response_history)
        ],
    },
    'repeat_with_context_and_player_playing': {
        'input_dialog': [
            voice('повтори', request_patcher=_include_response_history, device_state=DEVICE_STATE_WITH_PLAYER)
        ],
    },
    'repeat_without_context': {
        'input_dialog': [
            voice('повтори')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
