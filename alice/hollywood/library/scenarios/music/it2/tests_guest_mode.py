import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.python.testing.it2.input import server_action, voice, callback, ClientBiometry
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    get_callback_from_reset_add_effect,
    assert_audio_play_directive,
    get_first_track_id,
    prepare_server_action_data,
    EXPERIMENTS,
    EXPERIMENTS_RADIO,
    EXPERIMENTS_PLAYLIST,
)
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TContentId  # noqa
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from conftest import get_scenario_state
from hamcrest import assert_that, has_entries, empty, is_not, contains, contains_string


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)

DEFAULT_GUEST_USER = auth.RobotMultiroom
DEFAULT_GUEST_USER_UID = '1035351314'
DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX = 'Wht4'

SOURCE_DEVICE_QUASAR_DEVICE_ID = 'quasar-device-id'

BIO_CAPABILITY_ENV_STATE = {
    'endpoints': [{
        'id': SOURCE_DEVICE_QUASAR_DEVICE_ID,
        'capabilities': [
            {
                '@type': 'type.googleapis.com/NAlice.TBioCapability',
            },
        ],
    }]
}


def _assert_guest_credentials(response, is_guest=True):
    guest_credentials = response.run_response_pyobj['continue_arguments']['scenario_args']['guest_credentials']
    if not is_guest:
        return guest_credentials is None
    assert guest_credentials, 'Guest credentials are expected to be passed to next stage via MusicArguments'
    assert guest_credentials['uid'] == DEFAULT_GUEST_USER_UID
    assert guest_credentials['oauth_token_encrypted']


def _make_client_biometry(is_owner_enrolled=True, matched_user=DEFAULT_GUEST_USER):
    return ClientBiometry(matched_user, is_owner_enrolled)


@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.environment_state(BIO_CAPABILITY_ENV_STATE)
@pytest.mark.device_state(device_id=SOURCE_DEVICE_QUASAR_DEVICE_ID)
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.station])
class TestsOnDemandGuestMode(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', id='track'),
        pytest.param('включи альбом dark side of the moon', id='album'),
        pytest.param('включи книгу дюна', id='album_audiobook'),
        pytest.param('включи книгу остров сокровищ', id='album_audiobook_fairy_tale'),
        pytest.param('включи ну что ты за ребенок', id='album_podcast'),
    ])
    def test_play_on_demand(self, alice, command):
        r = alice(voice(command, client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        _assert_guest_credentials(r)

        http_request = r.sources_dump.get_http_request
        content_req = http_request('MUSIC_SCENARIO_THIN_CONTENT_PROXY') or http_request('MUSIC_SCENARIO_THIN_ALBUM_CONTENT_PROXY') or http_request('MUSIC_SCENARIO_THIN_ARTIST_CONTENT_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in content_req.path, 'Request to music backend should be made with guest\'s credentials'

        mp3_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_MP3_GET_ALICE_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in mp3_req.path, 'Request to music backend should be made with guest\'s credentials'


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsRadioGuestMode(TestsBase):

    @pytest.mark.parametrize('command, output_speech_re', [
        pytest.param('включи грустную музыку', r'.*(груст).*', id='mood'),
        pytest.param('включи рок', r'.*(рок|Рок).*', id='genre'),
        pytest.param('включи музыку для тренировки', r'.*(тренир).*', id='activity'),
        pytest.param('включи музыку восьмидесятых', r'.*(восьмидеся).*', id='epoch'),
    ])
    def test_play_radio(self, alice, command, output_speech_re):
        r = alice(voice(command, client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(output_speech_re, layout.OutputSpeech)
        assert_audio_play_directive(layout.Directives)
        _assert_guest_credentials(r)

        radio_new_session_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY')
        assert re.match(r'/external-rotor/session/new', radio_new_session_req.path)
        assert radio_new_session_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Request to music backend should be made with guest\'s credentials'


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsRadioCallbacksGuestMode(TestsBase):

    def test_play_mood_next_callback(self, alice):
        '''
        См. описание аналогичного теста в tests_thin_client.py. Здесь дополнительно проверяем, что
        новая пачка треков будет запрошена от имени гостя, который включил радио.
        '''
        r = alice(voice('включи грустную музыку', client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        _assert_guest_credentials(r)

        radio_new_session_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY')
        assert re.match(r'/external-rotor/session/new', radio_new_session_req.path)
        assert radio_new_session_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Request to music backend should be made with guest\'s credentials'

        state = get_scenario_state(r.continue_response)
        queue_size = len(state.Queue.Queue)
        for i in range(queue_size + 1):
            if i == 0:
                assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
            else:
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
            callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

            r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
            assert r.scenario_stages() == {'run', 'continue'}
            assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        radio_session_tracks_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY')
        assert re.match(r'/external-rotor/session/\S+/tracks', radio_session_tracks_req.path)
        assert radio_session_tracks_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Request to music backend should be made with guest\'s credentials'

    def test_play_mood_on_started(self, alice):
        '''
        См. описание аналогичного теста в tests_thin_client.py. Здесь дополнительно проверяем, что в фидбеки
        добавляются credentials гостя, который включил радио.
        '''
        r = alice(voice('включи грустную музыку', client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_guest_credentials(r)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        radio_session_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        guest_oauth_token_encrypted = r.continue_response_pyobj['response']['state']['queue']['playback_context']['biometry_options']['guest_oauth_token_encrypted']
        assert guest_oauth_token_encrypted

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_entries({
                            'uid': DEFAULT_GUEST_USER_UID,
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(first_track_id),
                            'albumId': is_not(empty()),
                            'playId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'context': 'radio',
                            'contextItem': 'mood:sad',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'RadioStarted',
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(first_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

        play_audio_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_PLAYS_PROXY')
        assert re.match(rf'/internal-api/plays\?__uid={DEFAULT_GUEST_USER_UID}', play_audio_feedback_req.path)

        radio_track_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_TRACK_STARTED_FEEDBACK_PROXY')
        assert radio_track_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

        radio_session_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_FEEDBACK_RADIO_STARTED_PROXY')
        assert radio_session_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

    def test_play_mood_next_track_on_started(self, alice):
        '''
        См. описание аналогичного теста в tests_thin_client.py. Здесь дополнительно проверяем, что в фидбеки
        добавляются credentials гостя, который включил радио.
        '''
        r = alice(voice('включи грустную музыку', client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_guest_credentials(r)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        guest_oauth_token_encrypted = r.continue_response_pyobj['response']['state']['queue']['playback_context']['biometry_options']['guest_oauth_token_encrypted']
        assert guest_oauth_token_encrypted

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        radio_session_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        directives = r.continue_response_pyobj['response']['layout']['directives']
        callbacks = directives[0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_entries({
                            'uid': DEFAULT_GUEST_USER_UID,
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(next_track_id),
                            'albumId': is_not(empty()),
                            'playId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'context': 'radio',
                            'contextItem': 'mood:sad',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'Skip',
                            'trackId': contains_string(first_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(next_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }))
                })
            }),
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

        play_audio_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_PLAYS_PROXY')
        assert re.match(rf'/internal-api/plays\?__uid={DEFAULT_GUEST_USER_UID}', play_audio_feedback_req.path)

        radio_track_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_TRACK_STARTED_FEEDBACK_PROXY')
        assert radio_track_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

        radio_session_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_FEEDBACK_SKIP_PROXY')
        assert radio_session_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

    @pytest.mark.parametrize('dislike_requester', [
        # pytest.param(auth.YandexPlus, id='owner_dislike'), TODO(klim-roma): add secrets to yav
        pytest.param(DEFAULT_GUEST_USER, id='guest_dislike'),
    ])
    def test_play_mood_dislike_on_started_guest_requester(self, alice, dislike_requester):
        '''
        См. описание аналогичного теста в tests_thin_client.py. Здесь дополнительно проверяем, что в фидбеки
        добавляются credentials гостя, который включил радио, а сам лайк отправляется от имени запросившего его юзера.
        '''
        client_biometry=_make_client_biometry(matched_user=dislike_requester)

        r = alice(voice('включи грустную музыку', client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_guest_credentials(r)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        guest_oauth_token_encrypted = r.continue_response_pyobj['response']['state']['queue']['playback_context']['biometry_options']['guest_oauth_token_encrypted']
        assert guest_oauth_token_encrypted

        r = alice(voice('дизлайк', client_biometry=client_biometry))
        assert r.scenario_stages() == {'run', 'apply'}

        dislike_req_uid = DEFAULT_GUEST_USER_UID if dislike_requester == DEFAULT_GUEST_USER else '1083955728'

        dislike_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_DISLIKE_PROXY')
        assert f'__uid={dislike_req_uid}' in dislike_req.path, 'Dislike request should be made with dislike requester uid'

        layout = r.apply_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        radio_session_id = r.apply_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        directives = r.apply_response_pyobj['response']['layout']['directives']
        callbacks = directives[0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_entries({
                            'uid': DEFAULT_GUEST_USER_UID,
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(next_track_id),
                            'albumId': is_not(empty()),
                            'playId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'batchId': is_not(empty()),
                            'context': 'radio',
                            'contextItem': 'mood:sad',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'Dislike',
                            'trackId': contains_string(first_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(next_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }))
                })
            }),
        }))

        directives = r.apply_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

        play_audio_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_PLAYS_PROXY')
        assert re.match(rf'/internal-api/plays\?__uid={DEFAULT_GUEST_USER_UID}', play_audio_feedback_req.path)

        radio_track_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_TRACK_STARTED_FEEDBACK_PROXY')
        assert radio_track_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

        radio_session_started_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_FEEDBACK_DISLIKE_PROXY')
        assert radio_session_started_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'

    def test_play_mood_on_finished(self, alice):
        '''
        См. описание аналогичного теста в tests_thin_client.py. Здесь дополнительно проверяем, что в фидбеки
        добавляются credentials гостя, который включил радио.
        '''
        r = alice(voice('включи грустную музыку', client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}
        _assert_guest_credentials(r)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        guest_oauth_token_encrypted = r.continue_response_pyobj['response']['state']['queue']['playback_context']['biometry_options']['guest_oauth_token_encrypted']
        assert guest_oauth_token_encrypted

        radio_session_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_finished': has_only_entries({
                'name': 'music_thin_client_on_finished',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': DEFAULT_GUEST_USER_UID,
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(first_track_id),
                            'albumId': is_not(empty()),
                            'playId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'batchId': is_not(empty()),
                            'context': 'radio',
                            'contextItem': 'mood:sad',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackFinished',
                            'trackId': contains_string(first_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                            'guestOAuthTokenEncrypted': guest_oauth_token_encrypted,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(audio_play.Callbacks.OnPlayFinishedCallback)

        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

        play_audio_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_PLAYS_PROXY')
        assert re.match(rf'/internal-api/plays\?__uid={DEFAULT_GUEST_USER_UID}', play_audio_feedback_req.path)

        radio_track_finished_feedback_req = r.sources_dump.get_http_request('MUSIC_COMMIT_RADIO_TRACK_FINISHED_FEEDBACK_PROXY')
        assert radio_track_finished_feedback_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Feedback request to music backend should be made with guest\'s credentials'


@pytest.mark.experiments(
    *EXPERIMENTS_PLAYLIST,
)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsPlaylistsGuestMode(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи мою музыку', id='my_music'),
        pytest.param('включи мою любимую музыку', id='my_favorite_music'),
    ])
    def test_playlist_liked(self, alice, command):
        r = alice(voice(command, client_biometry=_make_client_biometry()))
        assert r.scenario_stages() == {'run', 'continue'}

        guest_oauth_token_encrypted = r.continue_response_pyobj['response']['state']['queue']['playback_context']['biometry_options']['guest_oauth_token_encrypted']
        assert guest_oauth_token_encrypted

        assert re.search('любим[оы][ей]|любите|понравил[ои]сь', r.continue_response.ResponseBody.Layout.OutputSpeech)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        content_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_CONTENT_PROXY')
        assert content_req.path.startswith(f'/internal-api/users/{DEFAULT_GUEST_USER_UID}/playlists'), 'Guest user\'s liked playlist should be requested'
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in content_req.path, 'Request to music backend should be made with guest\'s credentials'


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsAutoflowGuestMode(TestsBase):

    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    @pytest.mark.parametrize('next_track_command_biometry', [
        pytest.param(None, id='incognito'),
        pytest.param(DEFAULT_GUEST_USER, id='same_guest'),
    ])
    def test_guest_autoflow_by_track_next_track_command(self, alice, is_owner_enrolled, next_track_command_biometry):
        r = alice(voice('включи seether песня fine again',
                        client_biometry=_make_client_biometry(is_owner_enrolled=is_owner_enrolled)))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        _assert_guest_credentials(r)

        http_request = r.sources_dump.get_http_request
        content_req = http_request('MUSIC_SCENARIO_THIN_CONTENT_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in content_req.path, 'Request to music backend should be made with guest\'s credentials'

        mp3_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_MP3_GET_ALICE_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in mp3_req.path, 'Request to music backend should be made with guest\'s credentials'

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек',
                        client_biometry=_make_client_biometry(is_owner_enrolled=is_owner_enrolled, matched_user=next_track_command_biometry)))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.EContentType.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == f'track:{first_track_id}'
        assert state.Queue.Queue[0].TrackId != first_track_id

        radio_session_tracks_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY')
        assert re.match(r'/external-rotor/session/new', radio_session_tracks_req.path)
        assert radio_session_tracks_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Request to music backend should be made with guest\'s credentials'

    @pytest.mark.parametrize('is_owner_enrolled', [
        pytest.param(True, id='owner_enrolled'),
        pytest.param(False, id='no_owner'),
    ])
    @pytest.mark.parametrize('next_track_command_biometry', [
        pytest.param(None, id='incognito'),
        pytest.param(DEFAULT_GUEST_USER, id='same_guest'),
    ])
    def test_guest_autoflow_by_track_get_next_callback(self, alice, is_owner_enrolled, next_track_command_biometry):
        r = alice(voice('включи seether песня fine again',
                        client_biometry=_make_client_biometry(is_owner_enrolled=is_owner_enrolled)))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        _assert_guest_credentials(r)

        http_request = r.sources_dump.get_http_request
        content_req = http_request('MUSIC_SCENARIO_THIN_CONTENT_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in content_req.path, 'Request to music backend should be made with guest\'s credentials'

        mp3_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_MP3_GET_ALICE_PROXY')
        assert f'__uid={DEFAULT_GUEST_USER_UID}' in mp3_req.path, 'Request to music backend should be made with guest\'s credentials'

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.EContentType.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == f'track:{first_track_id}'
        assert state.Queue.Queue[0].TrackId != first_track_id

        radio_session_tracks_req = r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY')
        assert re.match(r'/external-rotor/session/new', radio_session_tracks_req.path)
        assert radio_session_tracks_req.headers['authorization'].endswith(DEFAULT_GUEST_USER_OAUTH_TOKEN_SUFFIX), \
            'Request to music backend should be made with guest\'s credentials'
