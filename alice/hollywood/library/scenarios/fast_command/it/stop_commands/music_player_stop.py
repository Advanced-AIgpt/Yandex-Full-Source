from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/fast_command/it/data/'

SCENARIO_NAME = 'Commands'
SCENARIO_HANDLE = 'fast_command'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = [
    'quasar',
    'yandexmini',
    'small_smart_speakers'
]

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {
    "music": {
        "player": {
            "pause": False
        }
    }
}

TESTS_DATA = {
    'music_player_stop': {
        'input_dialog': [
            voice('стоп')
        ],
    },
    'music_player_stop_without_both_players_states': {
        'input_dialog': [
            voice('стоп', device_state={})
        ],
    },
    'music_player_stop_with_audio_player_state': {
        'input_dialog': [
            voice('стоп', device_state={
                'audio_player': {
                    'player_state': 'Playing'
                },
            })
        ],
        'app_presets': {
            'only': ['quasar', 'search_app_prod']
        }
    },
    'music_player_stop_with_both_players_states': {
        'input_dialog': [
            voice('стоп', device_state={
                "music": {
                    "player": {
                        "pause": False
                    }
                },
                'audio_player': {
                    'player_state': 'Playing'
                },
            })
        ],
        'app_presets': {
            'only': ['quasar', 'search_app_prod']
        }
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
