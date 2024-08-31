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
]

DEFAULT_EXPERIMENTS = ['hw_music_thin_client']

DEFAULT_SUPPORTED_FEATURES = ['audio_client']

DEFAULT_DEVICE_STATE = {
    'audio_player': {
        'player_state': 'Playing'
    },
}

TESTS_DATA = {
    'audio_player_stop': {
        'input_dialog': [
            voice('стоп')
        ],
    },
    'audio_player_stop_without_both_players_states': {
        'input_dialog': [
            voice('стоп', device_state={})
        ],
    },
    'audio_player_stop_with_music_player_state': {
        'input_dialog': [
            voice('стоп', device_state={
                "music": {
                    "player": {
                        "pause": False
                    }
                },
            })
        ],
    },
    'audio_player_stop_with_both_players_states': {
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
    },
    'audio_player_stop_with_explicit_flag': {
        'input_dialog': [
            voice('стоп', device_state={
                'music': {
                    'player': {
                        'pause': True
                    }
                },
                'audio_player': {
                    'player_state': 'Stopped'
                },
            })
        ],
        'experiments': ['hw_audio_pause_on_common_pause'],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
