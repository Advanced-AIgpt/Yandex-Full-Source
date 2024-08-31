import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    get_callback,
    get_first_track_id,
    prepare_server_action_data,
    EXPERIMENTS,
)
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from hamcrest import assert_that, has_entries, empty, is_not, contains, contains_string


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.station])
class TestsSaveProgress:

    @pytest.mark.parametrize('command, albumType, callback_name, short_name', [
        pytest.param('включи подушки шоу', 'podcast', 'music_thin_client_on_stopped', 'on_stopped', id='podcast_on_stopped'),
        pytest.param('включи подушки шоу', 'podcast', 'music_thin_client_on_finished', 'on_finished', id='podcast_on_finished'),
        # TODO(amullanurov): fix test "default_podcast_on_finished"
        # pytest.param('включи подкаст лайфхакер', 'podcast', 'music_thin_client_on_finished', 'on_finished', id='default_podcast_on_finished'),
        pytest.param('включи аудиокнигу дюна', 'audiobook', 'music_thin_client_on_finished', 'on_finished', id='audiobook_on_finished'),
        pytest.param('включи альбом золотые русские сказки. Часть 2', 'fairy-tale', 'music_thin_client_on_finished', 'on_finished', id='fairy_tale_on_finished',
                     marks=[pytest.mark.experiments('hw_music_thin_client_fairy_tale_ondemand', 'hw_music_fairy_tales_enable_ondemand',
                                                    'hw_music_thin_client_use_save_progress_fairy-tale')]),
    ])
    def test_save_progress(self, alice, command, albumType, callback_name, short_name):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            short_name: has_only_entries({
                'name': callback_name,
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-on_demand-catalogue_-album',
                            'trackId': contains_string(first_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'albumType': albumType,
                            'context': 'album',
                            'contextItem': is_not(empty()),
                            'should_save_progress': True,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, callback_name))
        output_speech = r.continue_response.ResponseBody.Layout.OutputSpeech
        assert output_speech.startswith('Продолжаю')
        if albumType == 'fairy-tale' or albumType == 'audiobook' or albumType == 'podcast':
            assert output_speech.endswith('"Алиса, включи сначала".')
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

    @pytest.mark.parametrize('command, albumType, callback_name, short_name', [
        pytest.param('включи аудиокнигу дюна сначала', 'audiobook', 'music_thin_client_on_finished', 'on_finished', id='audiobook_on_finished'),
    ])
    def test_save_progress_with_beginning_offset(self, alice, command, albumType, callback_name, short_name):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            short_name: has_only_entries({
                'name': callback_name,
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-on_demand-catalogue_-album',
                            'trackId': contains_string(first_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'albumType': albumType,
                            'context': 'album',
                            'contextItem': is_not(empty()),
                            'should_save_progress': True,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, callback_name))
        assert r.continue_response.ResponseBody.Layout.OutputSpeech.startswith('Включаю')
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}

    @pytest.mark.experiments('hw_music_change_track_number')
    @pytest.mark.parametrize('command, player_command, albumType', [
        pytest.param('включи аудиокнигу дюна', 'включи свежий выпуск', 'audiobook', id='newest'),
        pytest.param('включи аудиокнигу дюна', 'включи сначала', 'audiobook', id='from_the_beginning'),
        pytest.param('включи аудиокнигу дюна', 'включи новую серию подкаста', 'audiobook', id='new'),
        pytest.param('включи аудиокнигу дюна', 'включи последний подкаст', 'audiobook', id='last'),
    ])
    def test_save_progress_with_change_track_number_command(self, alice, command, player_command, albumType):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_finished': has_only_entries({
                'name': 'music_thin_client_on_finished',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-on_demand-catalogue_-album',
                            'trackId': contains_string(first_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'albumType': albumType,
                            'context': 'album',
                            'contextItem': is_not(empty()),
                            'should_save_progress': True,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        assert r.continue_response.ResponseBody.Layout.OutputSpeech.startswith('Продолжаю')

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        r = alice(voice(player_command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert next_track_id != get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        assert not r.continue_response.ResponseBody.Layout.OutputSpeech

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_finished': has_only_entries({
                'name': 'music_thin_client_on_finished',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-on_demand-catalogue_-album',
                            'trackId': '91286781',
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'albumType': albumType,
                            'context': 'album',
                            'contextItem': is_not(empty()),
                            'should_save_progress': True,
                        })
                    }))
                })
            })
        }))

    @pytest.mark.experiments('hw_music_change_track_number')
    @pytest.mark.parametrize('command', [
        pytest.param('включи альбом последний герой', id='album'),
    ])
    def test_not_save_progress_with_change_track_number_command(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        r = alice(voice('включи свежий выпуск'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_irrelevant()
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Не могу это сделать.'

    def test_podcast_save_progress_bad_device_state(self, alice):
        r = alice(voice('включи подушки шоу'))
        assert r.scenario_stages() == {'run', 'continue'}

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_finished': has_only_entries({
                'name': 'music_thin_client_on_finished',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-on_demand-catalogue_-album',
                            'trackId': contains_string(first_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'albumType': 'podcast',
                            'context': 'album',
                            'contextItem': is_not(empty()),
                            'should_save_progress': True,
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, 'music_thin_client_on_finished'))
        alice.update_audio_player_duration_ms(duration_ms=0)
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}
