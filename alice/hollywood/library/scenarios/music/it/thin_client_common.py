from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import SERVER_TIME_MS_20_JAN_2020_054431
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'
DEFAULT_APP_PRESETS = ['quasar']
DEFAULT_EXPERIMENTS = [
    'hw_music_thin_client',
    'hw_music_thin_client_playlist',
    'mm_enable_stack_engine',
    'new_music_radio_nlg',
]
DEFAULT_SUPPORTED_FEATURES = ['audio_client', 'music_player_allow_shots']


def make_device_state_idle():
    return {
        'audio_player': {
            'player_state': 'Idle',
        }
    }


def make_device_state_playing(stream_id='EXV4PTox8VuJ',  # Must match PlayId in the Queue. Manually fix it if it's not after it/generator call
                               played_ms=15000, offset_ms=15000, duration_ms=240000, owner='music',
                               last_play_timestamp=SERVER_TIME_MS_20_JAN_2020_054431 - 60000):
    return {
        'audio_player': {
            'player_state': 'Playing',
            'played_ms': played_ms,
            'offset_ms': offset_ms,
            'duration_ms': duration_ms,
            'current_stream': {
                'stream_id': stream_id,
            },
            'scenario_meta': {
                'owner': owner,
            },
            'last_play_timestamp': last_play_timestamp,
        }
    }


def make_device_state_stopped(stream_id='EXV4PTox8VuJ',  # Must match PlayId in the Queue. Manually fix it if it's not after it/generator call
                               played_ms=15000, offset_ms=15000, duration_ms=240000, owner='music',
                               last_play_timestamp=SERVER_TIME_MS_20_JAN_2020_054431 - 60000):
    return {
        'audio_player': {
            'player_state': 'Stopped',
            'played_ms': played_ms,
            'offset_ms': offset_ms,
            'duration_ms': duration_ms,
            'current_stream': {
                'stream_id': stream_id,
            },
            'scenario_meta': {
                'owner': owner,
            },
            'last_play_timestamp': last_play_timestamp,
        }
    }


def make_device_state_finished(stream_id='EXV4PTox8VuJ',  # Must match PlayId in the Queue. Manually fix it if it's not after it/generator call
                                played_ms=240000, offset_ms=240000, duration_ms=240000, owner='music',
                                last_play_timestamp=SERVER_TIME_MS_20_JAN_2020_054431 - 60000):
    return {
        'audio_player': {
            'player_state': 'Finished',
            'played_ms': played_ms,
            'offset_ms': offset_ms,
            'duration_ms': duration_ms,
            'current_stream': {
                'stream_id': stream_id,
            },
            'scenario_meta': {
                'owner': owner,
            },
            'last_play_timestamp': last_play_timestamp,
        }
    }


def make_lifecycle_callback_payload(track_id='1710810', play_id='EXV4PTox8VuJ'):
    return {
        'events': [
            {
                'playAudioEvent': {
                    'trackId': track_id,
                    'from': 'hollywood',
                    'playId': play_id,
                    'uid': '1000000042',  # This should be either owner uid or "biometry guest uid"
                }
            },
            # More events could go here... e.g. radioFeedback
        ],
        '@scenario_name': 'HollywoodMusic'
    }


def make_lifecycle_callback_payload_on_started_radio():
    return {
        'events': [
            _make_play_audio_event(),
            _make_radio_feedback_event(type_='TrackStarted', track_id='100001'),
        ],
        '@scenario_name': 'HollywoodMusic'
    }


def make_lifecycle_callback_payload_on_started_radio_after_skip():
    return {
        'events': [
            _make_play_audio_event(track_id='100002'),
            _make_radio_feedback_event(type_='TrackStarted', track_id='100002',
                                       batch_id='aaaaaaaa-bbbb-cccc-dddd-ffffffffffff'),
            _make_radio_feedback_event('Skip', track_id='100001', batch_id='aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee'),
        ],
        '@scenario_name': 'HollywoodMusic'
    }


def make_lifecycle_callback_payload_on_started_radio_after_dislike():
    return {
        'events': [
            _make_play_audio_event(track_id='100002'),
            _make_radio_feedback_event(type_='TrackStarted', track_id='100002',
                                       batch_id='aaaaaaaa-bbbb-cccc-dddd-ffffffffffff'),
            _make_radio_feedback_event('Dislike', track_id='100001', batch_id='aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee'),
        ],
        '@scenario_name': 'HollywoodMusic'
    }


def make_lifecycle_callback_payload_on_finished_radio():
    return {
        'events': [
            _make_play_audio_event(),
            _make_radio_feedback_event(type_='TrackFinished', track_id='100001'),
        ],
        '@scenario_name': 'HollywoodMusic'
    }


def _make_play_audio_event(track_id='1710810', play_id='EXV4PTox8VuJ'):
    return {
        'playAudioEvent': {
            'trackId': track_id,
            'from': 'hollywood',
            'playId': play_id,
            'uid': '1000000042',  # This should be either owner uid or "biometry guest uid"
        }
    }


def _make_radio_feedback_event(type_, track_id='1710810', batch_id='c8277792-0d2c-57d5-b465-aaaaaaaaaaaa'):
    result = {
        'radioFeedbackEvent': {
            'type': type_,
            'stationId': 'mood:sad',
            'uid': '1000000042',  # This should be either owner uid or "biometry guest uid"
        }
    }
    if track_id:
        result['radioFeedbackEvent']['trackId'] = track_id
    if batch_id:
        result['radioFeedbackEvent']['batchId'] = batch_id
    return result


# NOTE: This is old forrmat. Use new format with 'events' list
def make_next_track_callback_payload(track_id='1710810', play_id='EXV4PTox8VuJ'):
    return {
        'trackId': track_id,
        'playId': play_id,
        '@scenario_name': 'HollywoodMusic'
    }


def make_multiroom_device_state():
    return {
        'multiroom': {
            'multiroom_session_id': '11223344'
        }
    }


def make_content_settings_device_state(contentSettings):
    return {
        'device_config': {
            'content_settings': contentSettings
        }
    }


def merge_device_states(one, two):
    one.update(two)
    return one


def create_music_back_stubber_fixture(tests_data_path):
    return create_stubber_fixture(
        tests_data_path,
        'music-web-ext.music.yandex.net',
        80,
        [
            StubberEndpoint('/internal-api/artists/{artist_id}/tracks', ['GET']),
            StubberEndpoint('/internal-api/albums/{album_id}/with-tracks', ['GET']),
            StubberEndpoint('/internal-api/account/status', ['GET']),
            StubberEndpoint('/internal-api/tracks/{track_id}', ['GET']),
            StubberEndpoint('/internal-api/playlists/personal/{playlist_id}', ['GET']),
            StubberEndpoint('/internal-api/search', ['GET']),
            StubberEndpoint('/internal-api/users/{user_id}/playlists/{playlist_id}', ['GET']),
            StubberEndpoint('/internal-api/plays', ['POST']),
            StubberEndpoint('/internal-api/users/{user_id}/likes/tracks/add', ['POST']),
            StubberEndpoint('/internal-api/users/{user_id}/dislikes/tracks/add', ['POST']),
            StubberEndpoint('/external-rotor/station/{station_id}/feedback', ['POST']),
            StubberEndpoint('/external-rotor/station/{station_id}/tracks', ['GET'], idempotent=False),
            StubberEndpoint('/internal-api/after-track', ['GET']),
        ],
        stubs_subdir='music_back',
    )


def create_music_back_dl_info_stubber_fixture(tests_data_path):
    return create_stubber_fixture(
        tests_data_path,
        'music-web-ext.music.yandex.net',
        80,
        [
            StubberEndpoint('/internal-api/tracks/{track_id}/download-info', ['GET'], idempotent=False),
        ],
        stubs_subdir='music_dl_info_back',
    )


def create_music_mds_stubber_fixture(tests_data_path):
    return create_stubber_fixture(
        tests_data_path,
        'storage.mds.yandex.net',
        80,  # NOTE: Stubber (and requests library under the hood of it) cuts the default ports
        # so this works fine with MDS, see MDSSUPPORT-605
        [
            StubberEndpoint('/file-download-info/{id1}/{id2}', ['GET']),
        ],
        stubs_subdir='music_mds_back',
    )
