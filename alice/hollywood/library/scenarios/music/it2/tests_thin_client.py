import logging
import re
import time
import json
import base64

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth, ALICE_START_TIME, region
from alice.hollywood.library.python.testing.it2.stubber import HttpResponseStub
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries, non_empty_dict
from alice.hollywood.library.python.testing.it2.input import server_action, voice, callback, Scenario, Biometry
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    assert_glagol_metadata,
    get_callback,
    get_callback_from_reset_add_effect,
    get_recovery_callback_from_reset_add,
    get_first_track_id,
    prepare_server_action_data,
    proto_to_dict,
    EXPERIMENTS,
    EXPERIMENTS_HLS,
    EXPERIMENTS_RADIO,
    EXPERIMENTS_PLAYLIST,
)
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState, ERepeatType, TContentId  # noqa
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from alice.megamind.protos.scenarios.directives_pb2 import TAudioPlayDirective, TPlayerRewindDirective  # noqa
from google.protobuf import text_format
from hamcrest import assert_that, has_entries, empty, is_not, matches_regexp, contains, contains_string, not_none
from conftest import get_scenario_state
from alice.protos.data.scenario.music.dj_request_data_pb2 import TDjMusicRequestData


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

logger = logging.getLogger(__name__)


def _assert_reask_response(response, reask_count):
    state = get_scenario_state(response)
    assert state.ReaskState.ReaskCount == reask_count
    layout = response.ResponseBody.Layout

    if reask_count == 1:
        assert layout.OutputSpeech in [
            'Какую <[ accented ]> песню?',
            'Какой <[ accented ]> альбом?',
            'Какого <[ accented ]> исполнителя?',
            'Не поняла. Какую <[ accented ]> песню?',
            'Не поняла. Какой <[ accented ]> альбом?',
            'Не поняла. Какого <[ accented ]> исполнителя?',
            'Кого <[ accented ]> включить?',
            'Что <[ accented ]> включить?',
        ]
    else:
        assert layout.OutputSpeech in [
            'Помедленее, я не поняла!',
            'Повторите почётче, пожалуйста!',
        ]
    assert not layout.Directives

    analytics_info = response.ResponseBody.AnalyticsInfo
    assert analytics_info.Intent == "music.reask"
    assert analytics_info.ProductScenarioName == "music"


_like_output_speech_re = r'.*(Буду включать такое чаще|что вам такое по душе|что вы оценили|лайк).*'


_dislike_output_speeches = [
    'Дизлайк принят.',
    'Хорошо, ставлю дизлайк.',
    'Окей, не буду такое ставить.',
    'Поняла. Больше не включу.',
    'Нет проблем, поставила дизлайк.',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.station])
class TestsOnDemand(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', id='track'),
        pytest.param('включи альбом dark side of the moon', id='album'),
        pytest.param('включи книгу дюна', id='album_audiobook'),
        pytest.param('включи книгу остров сокровищ', id='album_audiobook_fairy_tale'),
        pytest.param('включи ну что ты за ребенок', id='album_podcast'),
    ])
    def test_play_on_demand(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        return str(r)

    def test_play_track_with_double_artist(self, alice):
        '''
        При генерации NLG должны быть перечислены все артисты (авторы) трека.
        '''
        r = alice(voice('включи el problema'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        return str(r)

    @pytest.mark.parametrize('command, content_type, output_speech_re', [
        pytest.param('включи последний альбом земфиры', TContentId.Album, r'.*(Земфира).*', id='album'),
        pytest.param('включи что-нибудь новое из beatles', TContentId.Album, r'.*(Beatles).*', id='artist'),
        pytest.param('поставь последнюю песню моргенштерна', TContentId.Album, r'.*(MORGENSHTERN).*', id='track'),
    ])
    def test_play_on_demand_with_novelty_slot(self, alice, command, content_type, output_speech_re):
        '''
        Комбинация слотов search_text и novelty должна приводить к включению последнего альбома данного артиста. Даже если просят "последнюю песню" этого артиста, все равно включаем альбом.
        '''
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert re.match(output_speech_re, r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == content_type
        return str(r)

    @pytest.mark.parametrize('command, content_type, output_speech_re', [
        # Тут мы имеем слоты novelty+search_text, однако artist={id=171, is_various=true} как бы не существует,
        # т.к. поиск находит альбом-сборник. Поэтому найти последний альбом данного "артиста" невозможно (код 404)
        pytest.param('включи классику современную', TContentId.Album, r'.*(классика).*', id='artist_is_various',
                     marks=pytest.mark.skip(reason="Does not work on trunk also, should be checked"))
    ])
    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/test_play_on_demand_with_novelty_slot_bass_post_megamind-prepare.json'),
        ],
    })
    def test_play_on_demand_with_novelty_slot2(self, alice, command, content_type, output_speech_re):
        '''
        Комбинация слотов search_text и novelty должна приводить к включению последнего альбома данного артиста. Даже если просят "последнюю песню" этого артиста, все равно включаем альбом.
        '''
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert re.match(output_speech_re, r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == content_type
        return str(r)

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', [
        pytest.param('включи queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', id='track'),
        pytest.param('включи альбом dark side of the moon', id='album'),
    ])
    def test_play_on_demand_no_plus(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert not r.continue_response.ResponseBody.Layout.Directives
        return str(r)

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/artists/{artist_id}/tracks': [
            HttpResponseStub(200, 'freeze_stubs/get_internal-api-artists-233945-tracks_play_artist_with_no_tracks.json'),
        ],
    })
    def test_play_artist_with_no_tracks(self, alice):
        r = alice(voice('включи ласковый май'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.Directives
        assert layout.OutputSpeech == 'Я не нашла музыки по вашему запросу, попробуйте ещё.'

    @pytest.mark.parametrize('object_id, object_type', [
        pytest.param('15578107', 'Album', id='album'),
        pytest.param('30', 'Artist', id='artist'),
    ])
    def test_play_empty_content(self, alice, object_id, object_type):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id
                    },
                    'object_type': {
                        'enum_value': object_type
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'К сожалению, у меня нет такой музыки.',
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.',
            'Как назло, именно этой музыки у меня нет.',
            'У меня нет такой музыки, попробуйте что-нибудь другое.',
            'Я не нашла музыки по вашему запросу, попробуйте ещё.',
        ]

    @pytest.mark.experiments(*EXPERIMENTS_HLS)
    @pytest.mark.additional_options(server_time_ms=round(time.time()*1000))
    @pytest.mark.supported_features('audio_client_hls')
    @pytest.mark.parametrize('', [
        pytest.param(id='one'),
    ])
    def test_hls_on_demand(self, alice):
        r = alice(voice('включи квин'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, is_hls=True)
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.Url.find('master.m3u8') != -1
        assert audio_play.Stream.Url.count('?') <= 1

    @pytest.mark.experiments(*EXPERIMENTS_HLS)
    @pytest.mark.additional_options(server_time_ms=round(time.time()*1000))
    @pytest.mark.supported_features(audio_client_hls=None)
    def test_hls_on_demand_without_supported_feature(self, alice):
        r = alice(voice('включи квин'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.Url.find('master.m3u8') == -1

    @pytest.mark.experiments('hw_music_thin_client_page_size=3')
    @pytest.mark.parametrize('object_id, object_type', [
        pytest.param('7019820', 'Album', id='album'),
        pytest.param('41052', 'Artist', id='artist'),
    ])
    def test_play_on_demand_next_page(self, alice, object_id, object_type):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Id == object_id

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Id == object_id

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Id == object_id

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Id == object_id

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_play_on_demand_ugc(self, alice):
        '''
        Запускаем 1 ugc трек, скипаем 1 раз, получаем радио user:onyourwave
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'ec5f857b-75f3-4065-a587-a62e46a2f7c7',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == 'user:onyourwave'

    @pytest.mark.experiments('hw_music_thin_client_page_size=3')
    def test_play_album_page_change(self, alice):
        def _get_music_monitoring_event(analytics_info):
            events = filter(lambda event: event.HasField('MusicMonitoringEvent'), analytics_info.Events)
            event = next(events, None)
            if not event:
                return None
            return event.MusicMonitoringEvent

        # pageSize==3, 1-st track is playing (in the history back), 2 are in the queue after this command
        r = alice(voice('включи альбом dark side of the moon'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Speak To Me')
        event = _get_music_monitoring_event(r.continue_response.ResponseBody.AnalyticsInfo)
        assert event.BatchOfTracksRequested

        # 2 tracks are in the history, 1 track is in the queue after this command
        r = alice(voice('следующий трек'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Breathe (In The Air)')
        event = _get_music_monitoring_event(r.continue_response.ResponseBody.AnalyticsInfo)
        assert not event

        # 3 tracks are in the history, queue is empty after this command
        r = alice(voice('следующий трек'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='On The Run')
        event = _get_music_monitoring_event(r.continue_response.ResponseBody.AnalyticsInfo)
        assert not event

        # 4 tracks are in the history, 2 in the queue after this command
        r = alice(voice('следующий трек'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Time')
        event = _get_music_monitoring_event(r.continue_response.ResponseBody.AnalyticsInfo)
        assert event.BatchOfTracksRequested


@pytest.mark.parametrize('surface', [surface.station])
class TestsOnDemandCallbacks(TestsBase):

    @pytest.mark.parametrize('callback_name, short_name', [
        pytest.param('music_thin_client_on_started', 'on_started', id='on_started'),
        pytest.param('music_thin_client_on_stopped', 'on_stopped', id='on_stopped'),
        pytest.param('music_thin_client_on_finished', 'on_finished', id='on_finished'),
        pytest.param('music_thin_client_on_failed', 'on_failed', id='on_failed'),
    ])
    def test_lifecycle_callback(self, alice, callback_name, short_name):
        r = alice(voice('включи клаву коку'))
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
                            'from': 'alice-on_demand-catalogue_-artist',
                            'trackId': contains_string(first_track_id),
                            'albumType': is_not(empty()),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'context': 'artist',
                            'contextItem': is_not(empty()),
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, callback_name))
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}


@pytest.mark.parametrize('surface', [surface.station])
class TestsChildVoice(TestsBase):

    child_mode_responses = {
        'Я не могу поставить эту музыку в детском режиме.',
        'Знаю такое, но не могу поставить в детском режиме.',
        'В детском режиме такое включить не получится.',
        'Не могу. Знаете почему? У вас включён детский режим.',
        'Я бы и рада, но у вас включён детский режим поиска.',
        'Не выйдет. У вас включён детский режим, а это не для детских ушей.'
    }

    def test_adult_content_with_child_voice(self, alice):
        r = alice(voice('включи моргерштерна', biometry=Biometry(classification={"child": True})))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech in self.child_mode_responses

    def test_adult_content_then_child_unpause(self, alice):
        r = alice(voice('включи моргерштерна'))
        assert r.scenario_stages() == {'run', 'continue'}
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        r = alice(voice('продолжи', biometry=Biometry(classification={"child": True})))
        assert r.scenario_stages() == {'run', 'continue'}
        assert (r.continue_response.ResponseBody.Layout.OutputSpeech in self.child_mode_responses or
               r.continue_response.ResponseBody.Layout.OutputSpeech == 'Лучше послушай эту музыку вместе с родителями.')

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={
        'content_settings': 'medium',
        'child_content_settings': 'safe',
    })
    def test_play_rock_safe_with_attention(self, alice):
        '''
        Алиса не может включить genre:rock в Safe режиме, но предлагает включить genre:forchildren.
        '''
        r = alice(voice('включи рок', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Это лучше слушать вместе с родителями - попроси их включить. А пока для тебя - детская музыка.'
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_forchildren_safe_without_attention(self, alice):
        expected_output_texts = [
            'Поняла. Для вас - детская музыка.',
            'Легко. Для вас - детская музыка.',
            'Детская музыка - отличный выбор.',
        ]
        r = alice(voice('включи детскую музыку', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in expected_output_texts
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_music_safe_without_attention(self, alice):
        r = alice(voice('включи музыку', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю.'
        assert_audio_play_directive(layout.Directives)


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS_RADIO)
class TestsPodcastsInSafeMode(TestsBase):

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_adult_podcast_in_safe_mode(self, alice):
        r = alice(voice('включи подкаст как жить'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == \
               'Лучше всего послушать этот подкаст вместе с родителями.'
        assert not r.continue_response.ResponseBody.Layout.Directives

    @pytest.mark.device_state(device_config={
        'content_settings': 'medium',
        'child_content_settings': 'safe',
    })
    def test_play_adult_podcast_in_safe_mode_then_child_unpause(self, alice):
        r = alice(voice('включи подкаст как жить'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.is_run_relevant()

        r = alice(voice('продолжи', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == \
               'Лучше всего послушать этот подкаст вместе с родителями.'
        assert not r.continue_response.ResponseBody.Layout.Directives

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_child_podcast_in_safe_mode(self, alice):
        r = alice(voice('включи подкаст зубочистки'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert (r.continue_response.ResponseBody.Layout.OutputSpeech ==
                'Продолжаю подкаст "Зубочистки". Чтобы включить самый последний выпуск, скажите: "Алиса, включи сначала".')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)


@pytest.mark.parametrize('surface', [surface.station])
class TestsNormalization(TestsBase):

    def test_r128_normalization(self, alice):
        r = alice(voice('включи песню Tuff Chat группы Colly C'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.HasField('Normalization')
        assert audio_play.Stream.Normalization.IntegratedLoudness != 0
        assert audio_play.Stream.Normalization.TruePeak != 0


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsRadio(TestsBase):

    @pytest.mark.parametrize('command, output_speech_re', [
        pytest.param('включи грустную музыку', r'.*(груст).*', id='mood'),
        pytest.param('включи рок', r'.*(рок).*', id='genre'),
        pytest.param('включи музыку для тренировки', r'.*(тренир).*', id='activity'),
        pytest.param('включи музыку восьмидесятых', r'.*(восьмидеся).*', id='epoch'),
    ])
    def test_play_radio(self, alice, command, output_speech_re):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(output_speech_re, layout.OutputSpeech)
        assert_audio_play_directive(layout.Directives)
        return str(r)

    @pytest.mark.parametrize('command, output_speech_re', [
        pytest.param('включи музыку', r'Включаю.*', id='my'),
        pytest.param('включи инструментальную музыку', r'Включаю.*', id='vocal'),
        pytest.param('включи музыку на французском языке', r'Включаю.*', id='language'),
    ])
    def test_play_radio_todo_support_thin_client(self, alice, command, output_speech_re):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(output_speech_re, layout.OutputSpeech)
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('AudioPlayDirective')
        return str(r)

    # TODO(sparkle): support multiple metatags for radio queries
    def test_music_play_semantic_frame_radio(self, alice):
        object_type = 'Radio'
        radio_station_id = 'mood:beautiful'
        start_from_track_id = '10892'
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': radio_station_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': start_from_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_radio'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives, title='What A Wonderful World', track_id=start_from_track_id)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, radio_station_id, shuffled=None)

        analytics_info_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert analytics_info_track_id == start_from_track_id

        radio_session_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']

        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(start_from_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'batchId': is_not(empty()),
                            'context': 'radio',
                            'contextItem': 'mood:beautiful',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'RadioStarted',
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:beautiful',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(start_from_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:beautiful',
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

    @pytest.mark.parametrize('command', [
        pytest.param('включи итальянскую музыку', id='italian'),
        pytest.param('включи зарубежную мелодичную музыку', id='non-russian'),
        pytest.param('включи индастриал на немецком языке', id='german'),
        pytest.param('включи русскую музыку девяностых годов', id='russian'),
        pytest.param('включи музыку на украинском', id='ukrainian'),
    ])
    def test_play_language(self, alice, command):
        '''
        Тут проверяем, что начали работать запросы с language
        Объекты песен - канонизируются, канонизированные песни нужно смотреть глазами!
        '''
        r = alice(voice(command))
        state = get_scenario_state(r.continue_response)

        # one track in history, four in queue
        tracks = [track for track in state.Queue.History] + [track for track in state.Queue.Queue]
        return str(tracks)

    @pytest.mark.oauth(auth.Yandex)
    def test_play_radio_no_plus(self, alice):
        r = alice(voice('включи веселую музыку'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert not r.continue_response.ResponseBody.Layout.Directives
        assert r.continue_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.music_play'
        assert r.continue_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'music'

    def test_without_action_request(self, alice):
        r = alice(voice('музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

    @pytest.mark.experiments('hw_music_thin_client_force_ichwill_onyourwave')
    def test_play_onyourwave_with_ichwill_recommendations(self, alice):
        r = alice(voice('включи музыку'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю.'
        assert_audio_play_directive(layout.Directives)

    def test_send_next_track_dj_info(self, alice):
        r = alice(voice("включи музыку"))
        r = alice(voice("следующий трек"))
        req = TDjMusicRequestData()
        track = get_scenario_state(r.continue_response).Queue.History[-1].TrackId
        r = alice(voice("следующий трек"))
        assert type(json.loads(r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY').content).get("djData")) == str
        req = TDjMusicRequestData()
        req = req.FromString(base64.b64decode(json.loads(r.sources_dump.get_http_request('MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY').content).get("djData")))
        assert str(req.SkippedTrackId) == track


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsRadioCallbacks(TestsBase):

    # TODO(vitvkv): Add even more tests:
    # - test_play_mood_pause_on_stopped
    # - test_play_mood_error_on_failed
    # - test_play_mood_next_callback_on_started

    def test_play_mood_next_callback(self, alice):
        '''
        Тут мы проверяем две вещи. 1) что коллбек автоматически переключающий треки радио работает
        2) что когда пачка треков заканчивается, мы не падаем, а запрашиваем след. пачку
        NOTE: Мы не можем использовать тут команду "следующий трек", т.к. в радио там совершенно другая логика
        (фибек Skip с дискардом очереди треков и т.п.).
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        acc = Accumulator()
        acc.add(r)

        state = get_scenario_state(r.continue_response)
        queue_size = len(state.Queue.Queue)
        assert queue_size == 4
        for i in range(queue_size + 1):  # +1 because we want to get a new batch on the last iteration
            if i == 0:
                assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
            else:
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
            callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

            r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
            assert r.scenario_stages() == {'run', 'continue'}
            assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
            acc.add(r)
        return str(acc)

    def test_play_mood_on_started(self, alice):
        '''
        Когда включается трек радио, то отправляется play-audio репорт и радио-фидбек trackStarted.
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        radio_session_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['session_id']
        assert radio_session_id

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
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
                            'type': 'RadioStarted',
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(first_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
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
        return str(r)

    def test_play_mood_next_track_on_started(self, alice):
        '''
        После команды "следующий трек", когда следующий трек включится, отправляется дополнительный фидбек
        skip с trackId предыдущего трека (который скипнули).
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

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
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'alice-discovery-radio-mood',
                            'trackId': contains_string(next_track_id),
                            'albumId': is_not(empty()),
                            'albumType': is_not(empty()),
                            'playId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'batchId': is_not(empty()),
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
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(next_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
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
        return str(r)

    def test_play_mood_dislike_on_started(self, alice):
        '''
        После команды "дизлайк", когда следующий трек включится, отправляется дополнительный фидбек
        dislike с trackId предыдущего трека (который дизлайкнули).
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
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
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
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
                        })
                    }), has_only_entries({
                        'radioFeedbackEvent': has_only_entries({
                            'type': 'TrackStarted',
                            'trackId': contains_string(next_track_id),
                            'batchId': is_not(empty()),
                            'radioSessionId': radio_session_id,
                            'stationId': 'mood:sad',
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
        return str(r)

    def test_play_mood_on_finished(self, alice):
        '''
        Когда трек радио доиграл до конца, то отправляется play-audio репорт и радио-фидбек trackFinished.
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

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
                            'uid': '1083955728',
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
        return str(r)

    @pytest.mark.experiments('hw_music_thin_client_radio_fresh_queue_size=2')
    def test_play_mood_next_callback_with_parameterized_fresh_queue_size(self, alice):
        '''
        Проверяем логику очищения очереди в случае с параметризацией количества треков в
        свежей радийной очереди. Если несколько раз подряд от клиента приходит колбек на
        переключение трека, то при достаточно маленькой очереди мы должны ее очистить,
        чтобы перезапросить очередную пачку треков у музбека.
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        acc = Accumulator()
        acc.add(r)

        state = get_scenario_state(r.continue_response)
        queue_size = len(state.Queue.Queue)
        assert queue_size == 4  # we don't truncate radio batch

        fresh_queue_size = 2
        for i in range(fresh_queue_size):
            if i == 0:
                assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
            else:
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
            callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

            r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
            assert r.scenario_stages() == {'run', 'continue'}
            assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
            acc.add(r)

        state = get_scenario_state(r.continue_response)
        queue_size = len(state.Queue.Queue)
        assert queue_size == 4  # we want to get a new batch when queue becomes too small

        return str(acc)

    @pytest.mark.experiments('hw_music_thin_client_radio_fresh_queue_size=1')
    def test_play_mood_next_callback_and_clear_queue_per_track(self, alice):
        '''
        То же самое, что и предыдущий тест, но очищаем историю после каждого трека.
        Ожидаем, что после каждого нового трека мы будем получать новую пачку радио.
        '''
        r = alice(voice('включи веселую музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        state = get_scenario_state(r.continue_response)
        queue_size = len(state.Queue.Queue)
        assert queue_size == 4  # we don't truncate radio batch

        radio_batch_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['batchId']
        assert radio_batch_id

        for i in range(3):
            if i == 0:
                assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
            else:
                reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
            callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

            r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
            assert r.scenario_stages() == {'run', 'continue'}
            assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

            prev_radio_batch_id = radio_batch_id
            radio_batch_id = r.continue_response_pyobj['response']['state']['queue']['current_content_loading_state']['radio']['batchId']
            assert radio_batch_id
            assert radio_batch_id != prev_radio_batch_id

            state = get_scenario_state(r.continue_response)
            queue_size = len(state.Queue.Queue)
            assert queue_size == 4


@pytest.mark.parametrize('surface', [surface.station])
class TestsRecoveryCallback(TestsBase):

    # TODO(vitvlkv): Add similar test for playlist (including case of loosing scState in the middle of a shot)
    def test_play_on_demand_recovery_callback(self, alice):
        r = alice(voice('включи benny greb'))
        assert r.scenario_stages() == {'run', 'continue'}
        audio_play = assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        first_track_title = audio_play.AudioPlayMetadata.Title

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        audio_play = assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != second_track_id
        recovery_callback_pyobj = r.continue_response_pyobj['response']['stack_engine']['actions'][0]['reset_add']['recovery_action']['callback']
        assert_that(recovery_callback_pyobj, has_only_entries({
            'name': 'music_thin_client_recovery',
            'payload': has_only_entries({
                'playback_context': has_only_entries({
                    'content_id': has_only_entries({
                        'type': 'Artist',
                        'id': '3760756',
                    }),
                    'content_info': has_only_entries({
                        'name': 'Benny Greb',
                    }),
                    'biometry_options': has_only_entries({
                        'user_id': '1083955728',
                    }),
                }),
                'paged': empty(),
            })
        }))
        recovery_callback = get_recovery_callback_from_reset_add(
            r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
            callback_name='music_thin_client_recovery'
        )

        alice.clear_session()

        r = alice(callback(name=recovery_callback['name'], payload=recovery_callback['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        audio_play = assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert not r.continue_response.ResponseBody.Layout.OutputSpeech
        assert first_track_title == audio_play.AudioPlayMetadata.Title

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_radio_recovery_callback(self, alice):
        r = alice(voice('включи грустную музыку девяностых'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != second_track_id
        recovery_callback_pyobj = r.continue_response_pyobj['response']['stack_engine']['actions'][0]['reset_add']['recovery_action']['callback']
        assert_that(recovery_callback_pyobj, has_only_entries({
            'name': 'music_thin_client_recovery',
            'payload': has_only_entries({
                'playback_context': has_only_entries({
                    'content_id': has_only_entries({
                        'type': 'Radio',
                        'ids': ['epoch:nineties', 'mood:sad'],
                        'id': 'mood:sad',
                    }),
                    'biometry_options': has_only_entries({
                        'user_id': '1083955728',
                    }),
                }),
                'radio': has_only_entries({
                    'session_id': is_not(empty()),
                    'batch_id': is_not(empty()),
                }),
            })
        }))
        recovery_callback = get_recovery_callback_from_reset_add(
            r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
            callback_name='music_thin_client_recovery'
        )

        alice.clear_session()

        r = alice(callback(name=recovery_callback['name'], payload=recovery_callback['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert not r.continue_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.experiments(
    *EXPERIMENTS_PLAYLIST,
)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsPlaylists(TestsBase):

    @pytest.mark.experiments('hw_music_thin_client_page_size=3')
    def test_play_playlist_next_page(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'yndx.epislon:1044',  # playlist with 5000+ tracks
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='California Dreamin\'')

        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '178190693:1044'

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='When I Call Your Name')

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='My Favorite Things')

        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Африка')

    @pytest.mark.parametrize('track_offset, track_name', [
        pytest.param(0, 'California Dreamin\'', id='first_track'),
        pytest.param(20, 'Курьер', id='middle_track'),
    ])
    def test_play_playlist_huge_size(self, alice, track_offset, track_name):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'yndx.epislon:1044',  # playlist with 5000+ tracks
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'play_single_track': {
                        'bool_value': False
                    },
                    'disable_autoflow': {
                        'bool_value': False
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                    'track_offset_index': {
                        'num_value': track_offset
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title=track_name, has_callbacks=False)

        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '178190693:1044'
        assert len(directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata.MusicMetadata.NextTrackInfo.Id) != 0

        return str(r)

    @pytest.mark.parametrize('command', [
        pytest.param('включи шум дождя', id='sound_of_rain'),
        pytest.param('включи громкие новинки', id='novelty_new'),
        pytest.param('включи популярную музыку', id='chart'),
    ])
    def test_common_special(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        return str(r)

    @pytest.mark.parametrize('command', [
        pytest.param('включи мою музыку', id='my_music'),
        pytest.param('включи мою любимую музыку', id='my_favorite_music'),
        pytest.param('включи мою музыку перемен', id='my_music_suffix'),  # https://st.yandex-team.ru/HOLLYWOOD-304
    ])
    def test_playlist_liked(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert re.search('любим[оы][ей]|любите|понравил[ои]сь', r.continue_response.ResponseBody.Layout.OutputSpeech)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        return str(r)

    def test_playlist_liked_with_podcast(self, alice):
        r = alice(voice('включи подкаст лайфхакера'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        response_body = r.run_response.CommitCandidate.ResponseBody
        assert re.match(r'.*(Буду включать такое чаще|что вам такое по душе|что вы оценили|лайк).*', response_body.Layout.OutputSpeech)
        assert not response_body.Layout.Directives

        r = alice(voice('включи мою музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert track_id != first_track_id

    @pytest.mark.parametrize('command', [
        pytest.param('включи мою музыку', id='my_music'),
        pytest.param('включи твой плейлист', id='playlist_of_alice'),
        pytest.param('включи плейлист дня', id='playlist_of_the_day'),
        # unable to regenerate
        # pytest.param('включи шум дождя', id='sound_of_rain'),
        pytest.param('включи громкие новинки', id='novelty_new'),
        pytest.param('включи популярную музыку', id='chart'),
    ])
    @pytest.mark.parametrize('mode, test_re', [
        pytest.param(' в перемешку', 'вперемешку', id='shuffle'),
        pytest.param(' на повторе', 'повтор', id='repeat'),
    ])
    def test_playlists_special_modes(self, alice, command, mode, test_re):
        r = alice(voice(command+mode))
        assert r.scenario_stages() == {'run', 'continue'}
        # Disabled due to https://st.yandex-team.ru/HOLLYWOOD-574
        # assert re.search(test_re, r.continue_response.ResponseBody.Layout.OutputSpeech)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.xfail(reason='Тыква перестала возвращаться, включается плейлист...')
    def test_playlist_of_the_day_guest_user(self, alice):
        '''
        Здесь гость хочет запустить плейлист дня, но для него этого плейлиста не существует,
        музбек возвращает тыкву из 0 треков. Мы опеределяем, что это тыква по полю "ready" = false,
        и делаем фоллбек на роторное радио user:onyourwave
        '''
        r = alice(voice('включи плейлист дежавю', biometry=Biometry(is_known=False)))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio

    @pytest.mark.xfail(reason='Тыква перестала возвращаться, включается плейлист...')
    def test_playlist_of_the_day_guest_user_bass_radio(self, alice):
        '''
        Аналогичный тесту выше, но фоллбек нельзя сделать сразу, потому что включено
        только "толстое" радио. Мы его так просто запустить не можем, поэтому делаем
        callback на его запуск "с нуля". Такой же callback используется в ряде других случаев.
        '''
        r = alice(voice('включи плейлист дежавю', biometry=Biometry(is_known=False)))
        assert r.scenario_stages() == {'run', 'continue'}
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_turn_on_radio')
        assert_that(callback_next['payload'], has_entries({
            'content_type': 'user',
            'content_id': 'onyourwave',
            'type': 'Original',
        }))

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_that(r.run_response_pyobj, has_entries({
            'continue_arguments': has_entries({
                'execution_flow_type': 'BassRadio',
                'radio_station_id': 'user:onyourwave',
            }),
        }))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')  # because this is old client

        # check that nothing breaks in next tracks
        r = alice(voice('следующий'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')  # because this is old client

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/search': [
            HttpResponseStub(200, 'freeze_stubs/get_internal-api-search_empty_playlist.json'),
        ],
    })
    def test_empty_playlist(self, alice):
        r = alice(voice('включи жара набор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech in {
            'К сожалению, у меня нет такой музыки.',
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.',
            'Как назло, именно этой музыки у меня нет.',
            'У меня нет такой музыки, попробуйте что-нибудь другое.',
            'Я не нашла музыки по вашему запросу, попробуйте ещё.',
        }

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_playlist_with_alice_shots(self, alice):
        r = alice(voice('включи плейлист с твоими шотами'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert re.match('.*Включаю Плейлист с Алисой.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective

        has_shot = False
        for i in range(20):
            if i >= 1 and audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot:
                # Stop
                alice.skip(2)
                r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
                assert r.scenario_stages() == {'run'}

                # Continue
                r = alice(voice('продолжи'))
                assert r.scenario_stages() == {'run', 'continue'}
                directives = r.continue_response.ResponseBody.Layout.Directives
                assert_audio_play_directive(directives)
                audio_play = directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot
                assert audio_play.Stream.OffsetMs >= 2000, "Проигрывание продолжается"
                callback_next = get_callback_from_reset_add_effect(r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd,
                                                                   callback_name='music_thin_client_next')

                # Track after shot
                r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
                assert r.scenario_stages() == {'run', 'continue'}
                directives = r.continue_response.ResponseBody.Layout.Directives
                assert_audio_play_directive(directives)
                audio_play = directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Track

                # Command previous track should not play shot
                r = alice(voice('следующий трек'))
                assert r.scenario_stages() == {'run', 'continue'}
                assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

                r = alice(voice('предыдущий трек'))
                assert r.scenario_stages() == {'run', 'continue'}
                directives = r.continue_response.ResponseBody.Layout.Directives
                assert_audio_play_directive(directives)
                audio_play = directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Track

                # After previous track shot of current track should be played
                r = alice(voice('предыдущий трек'))
                assert r.scenario_stages() == {'run', 'continue'}
                assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

                r = alice(voice('следующий трек'))
                assert r.scenario_stages() == {'run', 'continue'}
                directives = r.continue_response.ResponseBody.Layout.Directives
                assert_audio_play_directive(directives)
                audio_play = directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot
                assert audio_play.Stream.OffsetMs == 0

                # Save Callbacks for test
                on_started_callback = prepare_server_action_data(audio_play.Callbacks.OnPlayStartedCallback)
                on_stopped_callback = prepare_server_action_data(audio_play.Callbacks.OnPlayStoppedCallback)
                on_finished_callback = prepare_server_action_data(audio_play.Callbacks.OnPlayFinishedCallback)

                # What is playing command
                r = alice(voice('что сейчас играет'))
                assert r.scenario_stages() == {'run'}
                assert re.match('.*шот.*', r.run_response.ResponseBody.Layout.OutputSpeech)
                assert not r.run_response.ResponseBody.Layout.Directives

                r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
                assert r.scenario_stages() == {'run'}

                # Like command
                r = alice(voice('лайк'))
                assert r.scenario_stages() == {'run', 'commit'}
                assert r.run_response.CommitCandidate.ResponseBody.Layout.OutputSpeech == 'Поставила лайк.'
                assert not r.run_response.CommitCandidate.ResponseBody.Layout.Directives

                # Dislike command on stopped player
                r = alice(voice('дизлайк'))
                assert r.scenario_stages() == {'run', 'apply'}
                assert r.apply_response.ResponseBody.Layout.OutputSpeech == 'Поставила дизлайк.'
                assert not r.apply_response.ResponseBody.Layout.Directives

                # Dislike command on playing player
                r = alice(voice('продолжи'))
                assert r.scenario_stages() == {'run', 'continue'}
                audio_play = r.continue_response.ResponseBody.Layout.Directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot

                r = alice(voice('дизлайк'))
                assert r.scenario_stages() == {'run', 'apply'}
                assert r.apply_response.ResponseBody.Layout.OutputSpeech == 'Поставила дизлайк.'
                assert_audio_play_directive(r.apply_response.ResponseBody.Layout.Directives)

                # Callbacks
                r = alice(server_action(on_started_callback['name'], on_started_callback['payload']))
                assert r.scenario_stages() == {'run'}
                assert r.is_run_irrelevant()

                r = alice(server_action(on_stopped_callback['name'], on_stopped_callback['payload']))
                assert r.scenario_stages() == {'run'}
                assert r.is_run_irrelevant()

                r = alice(server_action(on_finished_callback['name'], on_finished_callback['payload']))
                assert r.is_run_relevant_with_second_scenario_stage('commit')
                assert not r.run_response.ResponseBody.Layout.OutputSpeech

                has_shot = True
                break

            r = alice(voice('следующий трек'))
            assert r.scenario_stages() == {'run', 'continue'}
            directives = r.continue_response.ResponseBody.Layout.Directives
            assert_audio_play_directive(directives)
            audio_play = directives[0].AudioPlayDirective

        # If shot was not found
        assert has_shot, "Шот не был найден"

    def test_play_personal_playlist(self, alice):
        '''
        Плейлист который должен включиться https://music.yandex.ru/users/robot-alice-hw-tests-plus/playlists/1000
        '''
        r = alice(voice('включи плейлист хрючень брудень и элекок'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '1035351314:1000'

    def test_playlist_verification_personal(self, alice):
        r = alice(voice('включи плейлист абацаба'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '1083955728:1001'
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech

    def test_playlist_verfication_verified_user(self, alice):
        r = alice(voice('включи 100 суперхитов'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '103372440:1101'
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech

    def test_playlist_verification_unverified_user(self, alice):
        r = alice(voice('включи отстойную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '691042205:1003'
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю \"самый отстойный плейлист\".',
            'Вот что я нашла среди плейлистов других пользователей. Включаю \"самый отстойный плейлист\".',
        ]

    def test_play_playlist_by_search(self, alice):
        '''
        Должно включаться https://music.yandex.ru/users/music-blog/playlists/2137
        '''
        r = alice(voice('включи песни из фильмов тарантино'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '103372440:2137'

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_playlist_ugc_only(self, alice):
        '''
        Запускаем плейлист из 2 ugc треков, скипаем 2 раза, запускаем радио по плейлисту
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1004',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == 'playlist:1035351314_1004'

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_playlist_normal_with_ugc_tracks(self, alice):
        '''
        Запускаем плейлист из 1 ugc трека и 1 нормального, скипаем 2 раза, запускаем радио по плейлисту
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1003',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == 'playlist:1035351314_1003'


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsPlaylistsNextTrack(TestsBase):

    @pytest.mark.experiments(
        *EXPERIMENTS_PLAYLIST,
    )
    def test_playlist_next_track(self, alice):
        '''
        Запускаем плейлист из 2 трэков, скипаем 2 раз, получаем радио
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1002',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'play_single_track': {
                        'bool_value': False
                    },
                    'disable_autoflow': {
                        'bool_value': False
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio

    @pytest.mark.experiments(
        *EXPERIMENTS_PLAYLIST,
        *EXPERIMENTS_RADIO
    )
    def test_playlist_next_callback(self, alice):
        '''
        Запускаем плейлист из 2 трэков, скипаем 1 раз, "ждём" штатного окончания второго, получаем радио
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1002',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'play_single_track': {
                        'bool_value': False
                    },
                    'disable_autoflow': {
                        'bool_value': False
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsOldMusicPlaylists(TestsBase):

    def test_play_my_music_suffix(self, alice):
        '''
        Тут будут два "конкурирующих" слота Novelty и SearchText, но должно уйти в старую музыку
        '''
        r = alice(voice('включи мою музыку перемен'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')
        return str(r)

    def test_novelty_web_answer(self, alice):
        '''
        Тут тоже будет слот Novelty и будет WebAnswer с плейлистом, должно уйти в старую музыку
        '''
        r = alice(voice('включи новинки из фильмов тарантино'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')
        return str(r)


@pytest.mark.experiments(
    'bg_enable_music_play_experimental',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestsReask(TestsBase):

    def test_state_negative(self, alice):
        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 2)

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.ReaskState.ReaskCount == 0
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

    def test_state_positive(self, alice):
        r = alice(voice('включи группу'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('linkin park'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.ReaskState.ReaskCount == 0
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        r = alice(voice('включи группу'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

    def test_reset_state(self, alice):
        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)

        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant

        r = alice(voice('включи песню'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response, 1)


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandNextTrack(TestsBase):

    def test_play_artist_next_video(self, alice):
        r = alice(voice('включи queen'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        r = alice(voice('следующий фильм'))
        assert r.is_run_irrelevant()
        irrel_message = 'Не могу включить следующее видео.'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == irrel_message
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == irrel_message

    def test_play_album_next_track(self, alice):
        '''
        Команда "следующий" включает следующий трек.
        '''
        r = alice(voice('включи альбом the dark side of the moon'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Speak To Me', sub_title='Pink Floyd')

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Breathe (In The Air)', sub_title='Pink Floyd')
        return str(r)

    def test_play_track_next_track(self, alice):
        '''
        Команда "следующий" на последнем треке on-demand контента включает радио с похожими треками.
        '''
        r = alice(voice('включи песню let it be'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')

        track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}

        # check radio request
        assert r.run_response_pyobj['continue_arguments']['scenario_args']['radio_request'] == {
            'station_ids': [f'track:{track_id}']
        }

        # check directive
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective'), 'Радио должно включиться на тонком плеере'

        return str(r)

    # TODO(vitvlkv): Add similar test_play_track_on_repeat_next_callback
    def test_play_track_on_repeat_next_track(self, alice):
        '''
        Команда "следующий" на последнем треке on-demand контента с включенным режимом повтора трека
        включает радио с похожими треками.
        '''
        r = alice(voice('включи песню let it be на повторе'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.run_response_pyobj['continue_arguments']['scenario_args']['radio_request'] == {
            'station_ids': [f'track:{track_id}']
        }
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_next_track(self, alice):
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id
        return str(r)

    def test_play_album_on_repeat_next_track(self, alice):
        '''
        Команда "следующий трек" НЕ выводит плеер из режима повторения альбома.
        '''
        r = alice(voice('включи альбом beyond magnetic металлики на повтор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll

        queue_size = len(state.Queue.Queue)
        assert queue_size == 3
        for i in range(queue_size + 1):  # +1 because we want to fell out of album on the last iteration
            r = alice(voice('следующий трек'))
            assert r.scenario_stages() == {'run', 'continue'}, f'FYI: Iteration i={i}'
            assert r.is_run_relevant()
            assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
            next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
            if i == queue_size:
                assert next_track_id == first_track_id, 'Вернулись к первому треку'
            else:
                assert next_track_id != first_track_id, 'Трек переключился на следующий'
            state = get_scenario_state(r.continue_response)
            assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll

    def test_play_album_on_repeat_next_track_prev_track(self, alice):
        '''
        Команда "предыдущий трек" при включенном режиме повтора альбома включает предыдущий трек, если
        таковой имеется в истории проигрывания.
        '''
        r = alice(voice('включи альбом beyond magnetic металлики на повтор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll, 'Режим повтора включен'

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll, 'Режим повтора включен'

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        third_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert third_track_id == first_track_id, 'Должен включиться самый первый трек'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll, 'Режим повтора включен'

    @pytest.mark.parametrize('directive_name', [
        pytest.param('MusicPlayDirective', id='music_player_radio',
                     marks=pytest.mark.xfail(reason='Test is dead after recanonization')),
        pytest.param('AudioPlayDirective', marks=pytest.mark.experiments(*EXPERIMENTS_RADIO), id='thin_client_radio'),
    ])
    def test_next_track_when_no_scenario_state(self, alice, directive_name):
        '''
        Обработка плеерной команды, когда нет стейта сценария должна включать радио пользователя
        (то же, что включается на "включи музыку").
        '''
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField(directive_name)

    def test_play_artist_pause_next_track(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        alice.skip(seconds=10)

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()

        alice.skip(seconds=10)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause >= 20
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

    # This test may flaps if music backend returns the same tracks time after time it checks clearing the queue during next_track command
    @pytest.mark.experiments(*EXPERIMENTS_RADIO, "music_radio_slow_next_track")
    def test_slow_radio_next_track(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        queue_before = [el.TrackId for el in get_scenario_state(r.continue_response).Queue.Queue]
        r = alice(voice('следующий трек'))
        history_after = [el.TrackId for el in get_scenario_state(r.continue_response).Queue.History]
        assert history_after[-1] != queue_before[0]


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandPrevTrack(TestsBase):

    def test_play_artist_prev_video(self, alice):
        r = alice(voice('включи queen'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        r = alice(voice('предыдущий фильм'))
        assert r.is_run_irrelevant()
        irrel_message = 'Не могу включить предыдущее видео.'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == irrel_message
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == irrel_message

    def test_play_artist_next_track_prev_track(self, alice):
        '''
        Команда "предыдущий трек" включает предыдущий трек.
        '''
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo) != first_track_id

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo) == first_track_id
        return str(r)

    def test_play_track_next_track_prev_track(self, alice):
        '''
        Включаем трек. "Cледующий трек" включает radio autoflow. "Предыдущий трек" включает первый трек.
        '''
        r = alice(voice('включи seether песня fine again'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.EContentType.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == f'track:{first_track_id}'
        assert state.Queue.Queue[0].TrackId != first_track_id

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo) == first_track_id

    def test_play_artist1_artist2_prev_track(self, alice):
        '''
        Проверяем, что "предыдущий трек" запоминает историю даже при смене on-demand контент-id.
        '''
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('включи мадонну'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert second_track_id != first_track_id

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        third_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert third_track_id == first_track_id, 'Должен включиться тот же самый трек queen'

    def test_play_album_prev_track(self, alice):
        '''
        Команда "предыдущий", когда в истории (в стейте) следующего трека нет.
        '''
        r = alice(voice('включи альбом the dark side of the moon'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Speak To Me', sub_title='Pink Floyd')

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run'}
        assert_that(r.run_response_pyobj, has_entries({
            'response_body': has_entries({
                'layout': has_only_entries({
                    'cards': is_not(empty()),
                    'output_speech': matches_regexp(r'не запомнила|забыла|не знаю, что играло'),
                }),
                'state': non_empty_dict(),
            }),
        }))

    def test_play_album_on_repeat_prev_track(self, alice):
        '''
        Команда "предыдущий трек" НЕ выводит плеер из режима повторения альбома.
        '''
        r = alice(voice('включи альбом beyond magnetic металлики на повтор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run'}
        assert_that(r.run_response_pyobj, has_entries({
            'response_body': has_entries({
                'layout': has_only_entries({
                    'cards': is_not(empty()),
                    'output_speech': matches_regexp(r'не запомнила|забыла|не знаю, что играло'),
                }),
                'state': has_entries({
                    'queue': has_entries({
                        'playback_context': has_entries({
                            'repeat_type': 'RepeatAll'
                        })
                    })
                }),
            }),
        }))


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)
class TestsCommandLike(TestsBase):

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_like(self, alice):
        '''
        Команда "лайк" лайкает текущий трек радио.
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        response_body = r.run_response.CommitCandidate.ResponseBody
        assert re.match(_like_output_speech_re, response_body.Layout.OutputSpeech)
        assert not response_body.Layout.Directives
        assert response_body.HasField('State')

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_like_after_some_time(self, alice):
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        alice.skip(seconds=70)

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        response_body = r.run_response.CommitCandidate.ResponseBody
        assert re.match(_like_output_speech_re, response_body.Layout.OutputSpeech)
        assert not response_body.Layout.Directives

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_like_on_short_long_pause(self, alice):
        '''
        Команда "лайк" на короткой паузе поставит лайк, а на долгой - нет
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        alice.skip(seconds=50)

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        response_body = r.run_response.CommitCandidate.ResponseBody
        assert re.match(_like_output_speech_re, response_body.Layout.OutputSpeech)
        assert not response_body.Layout.Directives

        alice.skip(seconds=70)
        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Не могу понять какую песню лайкать.',
            'Я бы с радостью, но не знаю какую песню лайкать.',
        ]

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_play_like_not_music_screen(self, alice):
        '''
        Команда "лайк" на другом экране не должна ставить лайк
        Если мы потом включили музыку обратно, следующий трек по колбеку должен прийти без проблем
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective

        alice.update_current_screen('main')

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Не могу понять какую песню лайкать.',
            'Я бы с радостью, но не знаю какую песню лайкать.',
        ]

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_play_ugc_like(self, alice):
        '''
        Запускаем плейлист из 2 ugc треков, ставим лайк первому без проблем
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1004',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        response_body = r.run_response.CommitCandidate.ResponseBody
        assert re.match(_like_output_speech_re, response_body.Layout.OutputSpeech)
        assert not response_body.Layout.Directives


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)
class TestsCommandDislike(TestsBase):

    @pytest.mark.additional_options(server_time_ms=round(time.time()*1000))
    @pytest.mark.supported_features('audio_client_hls')
    @pytest.mark.parametrize('is_hls', [
        pytest.param(False, id='get_alice'),
        pytest.param(True, id='hls',
                     marks=[pytest.mark.xfail(reason='Test is dead after recanonization'),
                            pytest.mark.experiments(*EXPERIMENTS_HLS)]),
    ])
    def test_play_album_dislike(self, alice, is_hls):
        '''
        Команда "дизлайк" дизлайкает текущий трек и включает следующий трек.
        '''
        r = alice(voice('включи альбом recovery'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Cold Wind Blows', sub_title='Eminem', is_hls=is_hls)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert_audio_play_directive(layout.Directives, title='Talkin’ 2 Myself', sub_title='Eminem',
                                    is_hls=is_hls)
        return str(r)

    def test_play_track_dislike(self, alice):
        '''
        Команда "дизлайк" на последнем треке on-demand контента дизлайкает текущий трек и
        включает радио с похожими треками (т.е. следующего трека нет).
        '''
        r = alice(voice('включи песню let it be'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert len(layout.Directives) == 0

        reset_add = r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_dislike(self, alice):
        '''
        Команда "дизлайк" дизлайкает текущий трек и включает следующий.
        '''
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert r.apply_response.ResponseBody.HasField('State')

        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        return str(r)

    def test_play_album_on_repeat_dislike(self, alice):
        '''
        Команда "дизлайк" НЕ выводит плеер из режима повторения альбома.
        '''
        r = alice(voice('включи альбом beyond magnetic металлики на повтор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.apply_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.apply_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatAll

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_dislike_after_some_time(self, alice):
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        alice.skip(seconds=70)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_dislike_on_short_long_pause(self, alice):
        '''
        Команда "дизлайк" на короткой паузе поставит дизлайк, а на долгой - нет
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        alice.skip(seconds=30)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        alice.skip(seconds=50)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert len(layout.Directives) == 0
        assert len(r.apply_response.ResponseBody.AnalyticsInfo.Actions) == 1
        assert r.apply_response.ResponseBody.AnalyticsInfo.Actions[0].HumanReadable == 'Трек отмечается как непонравившийся'

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.OffsetMs >= 30000, "Проигрывание продолжается"
        assert first_track_id == get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        alice.skip(seconds=70)

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Не могу понять какую песню дизлайкать.',
            'Я бы с радостью, но не знаю какую песню дизлайкать.',
        ]

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_play_dislike_not_music_screen(self, alice):
        '''
        Команда "дизлайк" на другом экране не должна ставить дизлайк
        Если мы потом включили музыку обратно, следующий трек по колбеку должен прийти без проблем
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective

        alice.update_current_screen('main')

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Не могу понять какую песню дизлайкать.',
            'Я бы с радостью, но не знаю какую песню дизлайкать.',
        ]

    @pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_dislike_on_playlist_with_alice_shots(self, alice):
        r = alice(voice('включи плейлист с твоими шотами'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective

        has_shot = False
        for i in range(20):
            if i >= 1 and audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot:
                r = alice(voice('предыдущий трек'))
                assert r.scenario_stages() == {'run', 'continue'}
                assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

                # After Dislike command shot should be played
                r = alice(voice('дизлайк'))
                assert r.scenario_stages() == {'run', 'apply'}
                directives = r.apply_response.ResponseBody.Layout.Directives
                assert_audio_play_directive(directives)
                audio_play = directives[0].AudioPlayDirective
                assert audio_play.Stream.StreamType == TAudioPlayDirective.TStream.TStreamType.Shot

                has_shot = True
                break

            r = alice(voice('следующий трек'))
            assert r.scenario_stages() == {'run', 'continue'}
            directives = r.continue_response.ResponseBody.Layout.Directives
            assert_audio_play_directive(directives)
            audio_play = directives[0].AudioPlayDirective

        # If shot was not found
        assert has_shot, "Шот не был найден"

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_play_ugc_dislike(self, alice):
        '''
        Запускаем плейлист из 2 ugc треков, дизлайкаем первый, включается второй ugc трек
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1004',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_ugc_dislike_last_track(self, alice):
        '''
        Запускаем плейлист из 2 ugc треков, скипаем первый, дизлайкаем второй, включается радио onyourwave
        '''
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1004',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech in _dislike_output_speeches

        reset_add = r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == 'playlist:1035351314_1004'


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandShuffle(TestsBase):

    _music_shuffle_responses = {
        'Сделала. После этого трека всё будет вперемешку.',
        'Пожалуйста. После этого трека - всё вперемешку.',
        'Готово. После этого трека включаю полный винегрет.',
    }

    _radio_shuffle_responses = {
        'А я уже.',
        'Да тут и так все вперемешку.',
        'Ок, еще раз пропустила через блендер.',
    }

    _irrelevant_shuffle_responses = {
        'Перемешаю для вас тишину, потому что ничего не играет.',
        'Сейчас ведь ничего не играет.',
        'Честно говоря, сейчас ничего не играет.',
        'Не знаю, как быть - ничего ведь не играет.',
    }

    def test_play_album_shuffled(self, alice):
        r = alice(voice('включи альбом dark side of the moon вперемешку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        return str(r)

    def test_play_album_shuffle_next_track(self, alice):
        # Первый трек 'Speak To Me', второй трек 'Breathe'
        r = alice(voice('включи альбом dark side of the moon'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in self._music_shuffle_responses
        glagol_metadata = layout.Directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, 'Album', '297567', shuffled=True)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        audio_play = r.continue_response.ResponseBody.Layout.Directives[0].AudioPlayDirective
        assert 'Breathe' not in audio_play.AudioPlayMetadata.Title, 'Очередь не была перемешана'

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_shuffle(self, alice):
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        alice.skip(seconds=60)

        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in self._radio_shuffle_responses
        assert not layout.Directives
        return str(r)

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_shuffle_on_short_pause(self, alice):
        '''
        Команда "перемешай" на короткой паузе перемешивает, а на долгой - нет
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        alice.skip(seconds=30)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        alice.skip(seconds=50)

        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in self._music_shuffle_responses
        glagol_metadata = layout.Directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, 'Artist', '4331814', shuffled=True)

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        audio_play = directives[0].AudioPlayDirective
        assert audio_play.Stream.OffsetMs >= 30000, "Проигрывание продолжается"
        assert first_track_id == get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_shuffle_on_long_pause(self, alice):
        '''
        Команда "перемешай" на короткой паузе перемешивает, а на долгой - нет
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        alice.skip(seconds=30)
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        alice.skip(seconds=70)

        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in self._irrelevant_shuffle_responses

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_play_shuffle_not_music_screen(self, alice):
        '''
        Команда "дизлайк" на другом экране не должна ставить дизлайк
        Если мы потом включили музыку обратно, следующий трек по колбеку должен прийти без проблем
        '''
        r = alice(voice('включи Клаву Коку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective

        alice.update_current_screen('main')

        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in self._irrelevant_shuffle_responses


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandContinue(TestsBase):

    @pytest.mark.parametrize('command, irrel_message', [
        pytest.param('продолжи фильм', 'Не могу продолжить видео.', id='player_type_video'),
        pytest.param('продолжить смотреть', 'Не могу продолжить видео.', id='player_action_type_watch'),
    ])
    def test_play_artist_pause_continue_video(self, alice, command, irrel_message):
        r = alice(voice('включи queen'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        r = alice(voice('пауза', scenario=(Scenario('Commands', 'fast_command'))))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice(command))
        assert r.is_run_irrelevant()
        assert r.run_response.ResponseBody.Layout.OutputSpeech == irrel_message
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == irrel_message

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_pause_continue(self, alice):
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech.startswith(('Знаю подходящую музыку для грустного настроения.',
                                               'Есть отличная музыка для грустного настроения.',
                                               'Вот,.sil<[600]> отлично подойдёт под грустное настроение.',
                                               'Вот,.sil<[600]> самое то для грустного настроения.'))
        audio_play = layout.Directives[0].AudioPlayDirective
        assert len(layout.Directives) == 1
        title = audio_play.AudioPlayMetadata.Title
        sub_title = audio_play.AudioPlayMetadata.SubTitle
        assert audio_play.Stream.OffsetMs == 0, "Проигрывание начинается с начала"

        alice.skip(seconds=60)

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        audio_play = r.continue_response.ResponseBody.Layout.Directives[0].AudioPlayDirective
        assert audio_play.AudioPlayMetadata.Title == title, 'Это должен быть тот же трек'
        assert audio_play.AudioPlayMetadata.SubTitle == sub_title, 'Это должен быть тот же трек'
        assert audio_play.Stream.OffsetMs >= 60000, "Проигрывание продолжается с момента остановки (примерно)"
        return str(r)

    def test_play_artist_continue(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}

        alice.skip(seconds=10)

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause >= 10
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.EContentType.Artist
        assert len(r.run_response.ResponseBody.Layout.Directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in ['Хорошо.', 'Окей.', 'Принято.', 'Продолжаю.',
                                                                   'Готово.', 'Продолжаем.', ]

        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.Intent == 'personal_assistant.scenarios.player_continue'
        assert analytics_info.ProductScenarioName == 'player_commands'
        assert analytics_info.Actions[0].HumanReadable == 'Включается музыка после паузы (или просто продолжается воспроизведение)'

    def test_play_artist_pause_continue(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        audio_play = layout.Directives[0].AudioPlayDirective
        title = audio_play.AudioPlayMetadata.Title
        sub_title = audio_play.AudioPlayMetadata.SubTitle

        alice.skip(seconds=10)

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        audio_play = r.continue_response.ResponseBody.Layout.Directives[0].AudioPlayDirective
        assert audio_play.AudioPlayMetadata.Title == title, 'Это должен быть тот же трек'
        assert audio_play.AudioPlayMetadata.SubTitle == sub_title, 'Это должен быть тот же трек'
        assert audio_play.Stream.OffsetMs >= 10000, "Проигрывание продолжается с момента остановки (примерно)"
        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandRepeat(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи beatles', id='artist'),
        pytest.param('включи альбом dark side of the moon', id='album'),
        pytest.param('включи yesterday', id='track'),
        pytest.param('включи веселую музыку', id='mood',
                     marks=pytest.mark.experiments(*EXPERIMENTS_RADIO)),
    ])
    def test_play_music_repeat(self, alice, command):
        '''
        Команда "повторяй этот трек" бесконечно повторяет один и тот же трек при автоматическом переключении треков.
        '''
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        first_track_title = audio_play.AudioPlayMetadata.Title

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        alice.skip(seconds=25)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        glagol_metadata = r.run_response.ResponseBody.Layout.Directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert glagol_metadata.MusicMetadata.RepeatMode == TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        assert audio_play.AudioPlayMetadata.Title == first_track_title

    def test_play_track_repeat_next_track(self, alice):
        '''
        Команда "следующий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи yesterday'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.EContentType.Radio
        assert state.Queue.PlaybackContext.ContentId.Id == f'track:{first_track_id}'
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    def test_play_artist_repeat_next_track(self, alice):
        '''
        Команда "следующий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи beatles'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_repeat_next_track(self, alice):
        '''
        Команда "следующий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи веселую музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    def test_play_track_repeat_prev_track(self, alice):
        '''
        Команда "предыдущий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи yesterday'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        assert re.match(r'.*(не запомнила|забыла|не знаю, что играло).*',
                        r.run_response.ResponseBody.Layout.OutputSpeech)
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    def test_play_artist_next_track_repeat_prev_track(self, alice):
        '''
        Команда "предыдущий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи beatles'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert second_track_id != first_track_id

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        prev_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert prev_track_id == first_track_id, 'Трек переключился на предыдущий'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_repeat_prev_track(self, alice):
        '''
        Команда "предыдущий трек" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи веселую музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        assert re.match(r'.*(не запомнила|забыла|не знаю, что играло).*',
                        r.run_response.ResponseBody.Layout.OutputSpeech)
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    def test_play_track_repeat_dislike(self, alice):
        '''
        Команда "дизлайк" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи yesterday'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.is_run_relevant()
        reset_add = r.apply_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_pb = reset_add.Effects[0].Callback
        assert callback_pb.Name == 'music_thin_client_next', 'Включается радио с похожими треками'
        # TODO(vitvlkv): Тут кстати интересный момент, включится радио с треками, похожими на дизлайкнутый, кек...
        # Скорее всего надо включать что-то вроде "моей музыки"
        state = get_scenario_state(r.apply_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    def test_play_artist_repeat_dislike(self, alice):
        '''
        Команда "дизлайк" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи beatles'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.apply_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.apply_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_mood_repeat_dislike(self, alice):
        '''
        Команда "дизлайк" выводит плеер из режима повторения трека.
        '''
        r = alice(voice('включи веселую музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        r = alice(voice('повторяй этот трек'))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        state = get_scenario_state(r.run_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatTrack

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.apply_response.ResponseBody.Layout.Directives)
        next_track_id = get_first_track_id(r.apply_response.ResponseBody.AnalyticsInfo)
        assert first_track_id != next_track_id, 'Трек переключился на следующий'
        state = get_scenario_state(r.apply_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandReplay(TestsBase):

    def test_play_track_replay(self, alice):
        r = alice(voice('включи песню sia cheap thrills'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)

        alice.skip(seconds=60)

        r = alice(voice('включи трек с начала'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, offset_ms=0)

        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id == first_track_id, 'Трек начал играть заново'
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.RepeatType == ERepeatType.RepeatNone

        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandRewind(TestsBase):

    def _assert_audio_rewind_directive(self, directives, type_, amount_ms=0):
        assert len(directives) == 1

        rewind = directives[0].AudioRewindDirective
        assert rewind.Type == type_
        assert rewind.AmountMs == amount_ms

    @pytest.mark.parametrize('command, type_, amount_ms', [
        pytest.param('перемотай вперед на тридцать секунд', TPlayerRewindDirective.EType.Forward,
                     30000, id='rewind_fwd_30s'),
        pytest.param('перемотай назад на полминуты', TPlayerRewindDirective.EType.Backward,
                     30000, id='rewind_bwd_half_minute'),
        pytest.param('перемотай на десятую секунду', TPlayerRewindDirective.EType.Absolute,
                     10000, id='rewind_to_10s'),
        pytest.param('перемотай на один час пять минут и тридцать секунд', TPlayerRewindDirective.EType.Absolute,
                     3930000, id='rewind_to_1h_5m_30s'),
        pytest.param('перемотай немного вперед', TPlayerRewindDirective.EType.Forward,
                     10000, id='rewind_fwd_little'),
        pytest.param('перемотай немного назад', TPlayerRewindDirective.EType.Backward,
                     10000, id='rewind_bwd_little'),
    ])
    def test_play_track_rewind(self, alice, command, type_, amount_ms):
        r = alice(voice('включи песню sia cheap thrills'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        skip_ms = 60000
        alice.skip(seconds=skip_ms / 1000)

        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()
        self._assert_audio_rewind_directive(r.run_response.ResponseBody.Layout.Directives,
                                            type_=type_, amount_ms=amount_ms)
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-951

    def test_rewind_loses_to_previous_track(self, alice):
        '''
        Запрос "Перемотай назад" матчится на два фрейма
        "personal_assistant.scenarios.player.previous_track"
        "personal_assistant.scenarios.player.rewind"
        Первый фрейм более "приоритетный" и он должен выиграть
        '''
        alice(voice('включи музыку'))
        r = alice(voice('перемотай назад'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.AnalyticsInfo.Intent
        assert not r.run_response.ResponseBody.Layout.Directives
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Простите, я отвлеклась и не запомнила, что играло до этого.',
            'Простите, я совершенно забыла, что включала до этого.',
            'Извините, я не запомнила, какой трек был предыдущим.',
            'Извините, но я выходила во время предыдущего трека и не знаю, что играло.',
        ]


@pytest.mark.parametrize('surface', [surface.station])
class TestsChildExplicitModes(TestsBase):

    @pytest.mark.device_state(device_config={'content_settings': 'medium'})
    def test_play_explicit_album_medium(self, alice):
        r = alice(voice('включи STRENGTH IN NUMB333RS'))  # В этом альбоме НЕ ВСЕ треки помечены explicit
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives, sub_title='FEVER 333')

        msg = 'FEVER 333, альбом "STRENGTH IN NUMB333RS".'
        assert layout.Cards[0].Text == 'Включаю: ' + msg
        assert layout.OutputSpeech == 'Включаю ' + msg + ' Осторожно! Детям лучше этого не слышать.'

    @pytest.mark.device_state(device_config={'content_settings': 'children'})
    def test_play_explicit_album_child(self, alice):
        r = alice(voice('включи STRENGTH IN NUMB333RS'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives, title='BURN IT', sub_title='FEVER 333')
        # BURN IT - это единственный трек в альбоме БЕЗ отметки explicit, он и включается

        msg = 'FEVER 333, альбом "STRENGTH IN NUMB333RS"'
        assert layout.Cards[0].Text == 'Включаю: ' + msg + '.'
        assert layout.OutputSpeech == 'Если что, у вас включён детский режим. Включаю ' + msg

    @pytest.mark.device_state(device_config={'content_settings': 'children'})
    def test_play_all_explicit_album_child(self, alice):
        r = alice(voice('включи альбом ЛЕГЕНДАРНАЯ ПЫЛЬ'))  # В этом альбоме ВСЕ треки помечены explicit
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.Directives

        msg = 'В детском режиме такое включить не получится.'
        assert layout.Cards[0].Text == msg
        assert layout.OutputSpeech == msg

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_explicit_album_safe(self, alice):
        r = alice(voice('включи STRENGTH IN NUMB333RS'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.Directives

        msg = 'Лучше послушай эту музыку вместе с родителями.'
        assert layout.Cards[0].Text == msg
        assert layout.OutputSpeech == msg

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_play_child_album_safe(self, alice):
        '''
        Трек https://music.yandex.ru/album/169022/track/1706440 про елочку хотя и детский, однако
        не помечен как isSuitableForChildren. Поэтому в safe режиме, мы его включить не можем, увы.
        '''
        r = alice(voice('включи альбом В лесу родилась ёлочка'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.is_run_relevant()

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.Directives

        msg = 'Лучше послушай эту музыку вместе с родителями.'
        assert layout.Cards[0].Text == msg
        assert layout.OutputSpeech == msg

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_rock_safe_with_attention(self, alice):
        '''
        Алиса не может включить genre:rock в Safe режиме, но предлагает включить genre:forchildren.
        '''
        r = alice(voice('включи рок'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Это лучше слушать вместе с родителями - попроси их включить. А пока для тебя - детская музыка.'
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_forchildren_safe_without_attention(self, alice):
        expected_output_texts = [
            'Поняла. Для вас - детская музыка.',
            'Легко. Для вас - детская музыка.',
            'Детская музыка - отличный выбор.',
        ]
        r = alice(voice('включи детскую музыку'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in expected_output_texts
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_music_safe_without_attention(self, alice):
        r = alice(voice('включи музыку', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю.'
        assert_audio_play_directive(layout.Directives)

    @pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_RADIO)
    @pytest.mark.device_state(device_config={
        'content_settings': 'medium',
        'child_content_settings': 'safe',
    })
    def test_play_radio_then_dislike_with_child_voice(self, alice):
        '''
        После моргенштерна включится радио по треку. Когда ребенок скажет дизлайк, то музыка попытается переключить
        трек на следущий. Скорее всего следующий трек не будет пригоден для ребенка (содержать флаги разрешающие
        играть трек в safe режиме).
        '''
        r = alice(voice('Включи моргенштерна cadillac'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        layout = r.continue_response.ResponseBody.Layout
        assert 'Включаю' in layout.OutputSpeech and 'Cadillac' in layout.OutputSpeech

        r = alice(voice('Следующий трек'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio
        assert state.Queue.PlaybackContext.ContentId.Id.startswith('track:')

        r = alice(voice('Поставь дизлайк', biometry=Biometry(classification={"child": True})))
        assert r.is_run_relevant_with_second_scenario_stage('apply')
        layout = r.apply_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Лучше послушай эту музыку вместе с родителями.'
        assert not layout.Directives


@pytest.mark.parametrize('surface', [surface.station])
class TestsCommandWhatIsPlaying(TestsBase):

    def _assert_dont_know_what_is_playing(self, response):
        output_speech_re = '|'.join([
            r'Мне в детстве медведь на ухо наступил. Не узнала мелодию.',
            r'Не нашла, что это за мелодия. Включите погромче.',
            r'Давайте еще раз попробуем.',
        ])
        assert re.match(output_speech_re, response.run_response.ResponseBody.Layout.OutputSpeech)
        assert not response.run_response.ResponseBody.Layout.Directives

    def test_play_artist_what_is_playing(self, alice):
        r = alice(voice('включи queen'))
        assert r.is_run_relevant()

        alice.skip(seconds=10)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause >= 10
        assert re.match(r'.*Queen.*песня ".+"', r.run_response.ResponseBody.Layout.OutputSpeech)
        assert not r.run_response.ResponseBody.Layout.Directives
        return str(r)

    def test_play_track_with_double_artist_what_is_playing(self, alice):
        r = alice(voice('включи el problema'))
        assert r.is_run_relevant()

        alice.skip(seconds=10)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause >= 10
        assert re.match(r'.*MORGENSHTERN.*Тимати.*песня ".+"', r.run_response.ResponseBody.Layout.OutputSpeech)
        assert not r.run_response.ResponseBody.Layout.Directives
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_no_state_what_is_playing(self, alice):
        '''
        Нет стейта сценария, и queue_item не указан в device state'е
        '''
        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        self._assert_dont_know_what_is_playing(r)
        assert r.is_run_relevant()

        assert not r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause == 0

        return str(r)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_play_artist_pause_what_is_playing(self, alice):
        r = alice(voice('включи queen'))
        assert r.is_run_relevant()

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.run_response.ResponseBody.Layout.Directives[0].ClearQueueDirective
        assert r.scenario_stages() == {'run'}

        alice.skip(seconds=10)

        r = alice(voice('что сейчас играет'))
        self._assert_dont_know_what_is_playing(r)
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_play_podcast(self, alice):
        r = alice(voice('включи подкаст деньги пришли'))
        assert r.is_run_relevant()

        alice.skip(seconds=10)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech.startswith("<[domain music]> Сейчас играет выпуск")

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_play_ugc(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'ec5f857b-75f3-4065-a587-a62e46a2f7c7',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        alice.skip(seconds=10)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert re.search(r'(Это|Сейчас играет) песня "Первый трек\.mp3"', layout.OutputSpeech)
        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
class TestsCallbackNext(TestsBase):

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_play_track_next_callback(self, alice):
        '''
        После завершения on-demand контента должно включаться радио с похожими треками.
        '''
        r = alice(voice('включи песню let it be'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')
        return str(r)

    def test_play_album_next_callback(self, alice):
        '''
        По завершении проигрывания трека автоматически включается следующий.
        '''
        r = alice(voice('включи альбом the dark side of the moon'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Speak To Me', sub_title='Pink Floyd')

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Breathe (In The Air)', sub_title='Pink Floyd')

        assert r.continue_response.ResponseBody.AnalyticsInfo.Objects
        return str(r)

    @pytest.mark.experiments('hw_music_get_nexts_use_apply')
    def test_play_album_next_callback_in_apply(self, alice):
        '''
        По завершении проигрывания трека автоматически включается следующий (используется схема run+apply).
        '''
        r = alice(voice('включи альбом the dark side of the moon'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Speak To Me', sub_title='Pink Floyd')

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'apply'}
        directives = r.apply_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Breathe (In The Air)', sub_title='Pink Floyd')
        return str(r)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO, *EXPERIMENTS_PLAYLIST)
    @pytest.mark.parametrize('data', [
        pytest.param({'command': 'включи музыку', 'id': 'user:onyourwave', 'type': 'Radio'}, id="music"),
        pytest.param({'command': 'включи веселую музыку для вождения', 'id': 'mood:happy', 'type': 'Radio'}, id="happy_driving"),
        pytest.param({'command': 'включи веселую музыку', 'id': 'mood:happy', 'type': 'Radio'}, id="happy"),
        pytest.param({'command': 'включи альбом the dark side of the moon', 'id': '297567', 'type': 'Album'}, id="album"),
        pytest.param({'command': 'включи плейлист хрючень брудень и элекок', 'id': '1035351314:1000', 'type': 'Playlist'}, id="bruden")
    ])
    def test_analitycs_next_play(self, alice, data):
        r = alice(voice(data['command']))
        assert r.scenario_stages() == {'run', 'continue'}

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        content_id = r.continue_response.ResponseBody.AnalyticsInfo.Objects[0].ContentId
        content_type_values = content_id.DESCRIPTOR.fields_by_name['Type'].enum_type.values_by_number

        assert content_type_values[content_id.Type].name == data['type']
        assert content_id.Id == data['id']

    # This test may flaps if music backend returns the same tracks time after time it checks clearing the queue during get_next callback
    @pytest.mark.experiments(*EXPERIMENTS_RADIO, "hw_music_thin_client_radio_fresh_queue_size=1")
    def test_slow_radio_next_play(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        queue_before = [el.TrackId for el in get_scenario_state(r.continue_response).Queue.Queue]
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        history_after = [el.TrackId for el in get_scenario_state(r.continue_response).Queue.History]
        assert history_after[-1] != queue_before[0]

    @pytest.mark.experiments(*EXPERIMENTS_RADIO, *EXPERIMENTS_PLAYLIST)
    def test_analytics_next_play_afterflow(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'robot-alice-hw-tests-plus:1002',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'play_single_track': {
                        'bool_value': False
                    },
                    'disable_autoflow': {
                        'bool_value': False
                    },
                    'disable_nlg': {
                        'bool_value': False
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        content_id = r.continue_response.ResponseBody.AnalyticsInfo.Objects[0].ContentId
        content_type_values = content_id.DESCRIPTOR.fields_by_name['Type'].enum_type.values_by_number

        assert content_type_values[content_id.Type].name == 'Playlist'
        assert content_id.Id == '1035351314:1002'
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        content_id = r.continue_response.ResponseBody.AnalyticsInfo.Objects[0].ContentId
        content_type_values = content_id.DESCRIPTOR.fields_by_name['Type'].enum_type.values_by_number
        assert content_type_values[content_id.Type].name == 'Radio'
        assert content_id.Id == 'playlist:1035351314_1002'

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        content_id = r.continue_response.ResponseBody.AnalyticsInfo.Objects[0].ContentId
        content_type_values = content_id.DESCRIPTOR.fields_by_name['Type'].enum_type.values_by_number
        assert content_type_values[content_id.Type].name == 'Radio'
        assert content_id.Id == 'playlist:1035351314_1002'


@pytest.mark.parametrize('surface', [surface.station])
class TestsMusicPlaySemanticFrame(TestsBase):

    def test_play_track_no_autoflow(self, alice):
        '''
        После завершения on-demand контента не должно включаться радио
        '''
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'search_text': {
                        'string_value': 'включи песню let it be',
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test',
            },
        }))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        assert not reset_add.Effects

    def test_play_track_dislike_no_autoflow(self, alice):
        '''
        После dislike не должно включаться радио
        '''
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'search_text': {
                        'string_value': 'включи песню let it be',
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test',
            },
        }))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Let It Be', sub_title='The Beatles')

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        assert not reset_add.Effects

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        response_body = r.apply_response.ResponseBody
        layout = response_body.Layout
        assert layout.OutputSpeech in _dislike_output_speeches
        assert len(layout.Directives) == 1
        assert len(response_body.AnalyticsInfo.Actions) == 1
        assert len(response_body.StackEngine.Actions) == 1
        assert layout.Directives[0].HasField('ClearQueueDirective')
        assert response_body.AnalyticsInfo.Actions[0].HumanReadable == 'Трек отмечается как непонравившийся'
        assert response_body.StackEngine.Actions[0].HasField('ResetAdd')

    def test_play_single_track_from_album_with_offset_no_nlg(self, alice):
        '''
        Играет один второй трек из альбома, без NLG.
        По завершении проигрывания трека не включается следующий.
        '''
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'search_text': {
                        'string_value': 'включи альбом the dark side of the moon',
                    },
                    'play_single_track': {
                        'bool_value': True
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                    'disable_history': {
                        'bool_value': True
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'track_offset_index': {
                        'num_value': 1
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test',
            },
        }))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert_audio_play_directive(directives, title='Breathe (In The Air)', sub_title='Pink Floyd')

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        assert not reset_add.Effects

        # disable_autoflow
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant

        # disable_history
        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant

    @pytest.mark.parametrize('object_id, object_type, offset,'
                             'track_offset_index, offset_sec, alarm_id, title, sub_title, is_hls', [
        pytest.param('38646012', 'Track', None, None, 0, None, 'Another Brick In The Wall, Pt. 2', 'Pink Floyd', False, id='track'),
        pytest.param('263065:3033', 'Playlist', None, None, 0, None, 'Love You Like A Love Song', 'Selena Gomez & The Scene', False, id='playlist',
                     marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)),
        pytest.param('211604', 'Album', None, 9, 0, None, 'Money', 'Pink Floyd', False, id='album_track_offset_index'),
        pytest.param('17277367', 'Album', 'saved_progress', 0, 0, None, None, None, False, id='podushki_show'),
        pytest.param('38646012', 'Track', None, None, 2.345, None, 'Another Brick In The Wall, Pt. 2', 'Pink Floyd', False, id='track_offset_sec'),
    ])
    def test_music_play_semantic_frame_with_object_id(self, alice, object_id, object_type, offset,
                                                      track_offset_index, offset_sec, alarm_id,
                                                      title, sub_title, is_hls):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'play_single_track': {
                        'bool_value': True
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        if offset is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['offset'] = {
                'offset_value': offset
            }
        if track_offset_index is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['track_offset_index'] = {
                'num_value': track_offset_index
            }
        if offset_sec != 0:
            payload['typed_semantic_frame']['music_play_semantic_frame']['offset_sec'] = {
                'double_value': str(offset_sec)
            }
        if alarm_id is not None:
            payload['typed_semantic_frame']['music_play_semantic_frame']['alarm_id'] = {
                'string_value': alarm_id
            }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives, title=title, sub_title=sub_title, offset_ms=offset_sec * 1000, is_hls=is_hls)

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        assert not reset_add.Effects

    def test_music_play_semantic_frame_with_object_id_alarm_id(self, alice):
        '''
        Old music works even with hw_music_thin_client if 'alarm_id' is set
        '''

        object_id = '38646012'
        alarm_id = 'sample_alarm_id'
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                    'play_single_track': {
                        'bool_value': True
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'alarm_id': {
                        'string_value': alarm_id
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        music_play = directives[0].MusicPlayDirective
        assert music_play.SessionId.strip()
        assert music_play.FirstTrackId == object_id
        assert music_play.Offset == 0
        assert music_play.AlarmId == alarm_id

    @pytest.mark.parametrize('object_id, object_type, start_from_track_id, title, prev_title', [
        # Включится нужный трек, на "предыдущий трек" включится предыдущий в альбоме
        pytest.param('10101', 'Album', '29238706', 'Дурак и молния', 'Холодное тело', id='album'),
        # Включится нужный трек, на "предыдущий трек" включится предыдущий в артисте
        pytest.param('41052', 'Artist', '50688531', 'Некромант', 'Хозяин леса', id='artist',
                     marks=pytest.mark.xfail(reason='Test is dead after recanonization')),
        # Включится нужный трек, на "предыдущий трек" включится предыдущий в плейлисте
        pytest.param('178190693:1044', 'Playlist', '734158', 'Курьер', 'Popcorn', id='playlist',
                     marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)),
        # Включится нужный трек, на "предыдущий трек" включится предыдущий в плейлисте (при этом first_page_size будет равен 20)
        pytest.param('178190693:1044', 'Playlist', '20681669', 'Tango - Composition', 'Символ веры', id='playlist_small_first_page_size',
                     marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)),
        # Не найдем нужный трек, включим первый
        pytest.param('178190693:1044', 'Playlist', '734158', 'California Dreamin\'', 'Краш', id='playlist_out_of_page_size',
                     marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST, 'hw_music_thin_client_find_track_idx_page_size=10')),
    ])
    def test_music_play_semantic_frame_with_object_id_start_from_track_id(self, alice, object_id, object_type, start_from_track_id, title, prev_title):

        r = alice(voice('включи краш'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'start_from_track_id': {
                        'string_value': start_from_track_id
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives, title=title, sub_title=None)

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title=prev_title)

    def test_music_play_semantic_frame_with_object_id_generative_without_exp(self, alice):
        '''
        Return irrelevant for generative without experiment flags
        '''

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'generative:lucky',
                    },
                    'object_type': {
                        'enum_value': 'Generative',
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech in {
            'К сожалению, у меня нет такой музыки.',
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.',
            'Как назло, именно этой музыки у меня нет.',
            'У меня нет такой музыки, попробуйте что-нибудь другое.',
            'Я не нашла музыки по вашему запросу, попробуйте ещё.',
        }

    def test_player_next_track_semantic_frame(self, alice):
        '''
        Проверяем переключение трека с помощью TSF с параметрами set_pause=true и set_pause=false
        '''
        r = alice(voice('включи альбом метеора'))
        assert r.scenario_stages() == {'run', 'continue'}

        payload = {
            'typed_semantic_frame': {
                'player_next_track_semantic_frame': {
                    'set_pause': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_next_track'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, set_pause=True)

    def test_player_prev_track_semantic_frame(self, alice):
        '''
        Проверяем переключение трека с помощью TSF с параметрами set_pause=true и set_pause=false
        '''
        r = alice(voice('включи альбом метеора'))
        assert r.scenario_stages() == {'run', 'continue'}

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}

        payload = {
            'typed_semantic_frame': {
                'player_prev_track_semantic_frame': {
                    'set_pause': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_prev_track'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, set_pause=True)

    def test_music_play_semantic_frame_with_from_field(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '211604',
                    },
                    'object_type': {
                        'enum_value': 'Album',
                    },
                    'disable_autoflow': {
                        'bool_value': True
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                    'from': {
                        'string_value': 'test_from'
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives, title='Astronomy Domine', sub_title='Pink Floyd')
        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'test_from',
                            'trackId': contains_string(first_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'context': 'album',
                            'contextItem': is_not(empty()),
                        })
                    }))
                })
            })
        }))

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        next_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert next_track_id != first_track_id

        directives = r.continue_response_pyobj['response']['layout']['directives']
        callbacks = directives[0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'test_from',
                            'trackId': contains_string(next_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'context': 'album',
                            'contextItem': is_not(empty()),
                        })
                    }))
                })
            }),
        }))

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives)
        prev_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert prev_track_id == first_track_id

        directives = r.continue_response_pyobj['response']['layout']['directives']
        callbacks = directives[0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            'on_started': has_only_entries({
                'name': 'music_thin_client_on_started',
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': '1083955728',
                            'from': 'test_from',
                            'trackId': contains_string(prev_track_id),
                            'playId': is_not(empty()),
                            'albumId': is_not(empty()),
                            'context': 'album',
                            'contextItem': is_not(empty()),
                        })
                    }))
                })
            }),
        }))

    @pytest.mark.experiments(
        *EXPERIMENTS_PLAYLIST,
    )
    @pytest.mark.parametrize('object_id, object_type, next_title', [
        pytest.param('298151', 'Album', 'Is There Anybody Out There?', id='album'),
        pytest.param('82939', 'Artist', 'Wish You Were Here', id='artist'),
        pytest.param('457553308:82939', 'Playlist', 'Wish You Were Here', id='playlist'),
    ])
    def test_unshuffle(self, alice, object_id, object_type, next_title):
        # Тест может падать при переканонизации из-за изменения порядка треков в используемых альбоме,
        # исполнителе, плейлисте
        # Включаем перемешанную сущность, проверяем, что в AudioPlayDirective установлен флаг shuffled в true.
        # Отключаем перемешивание. Воспроизведениен не прерывается, в ответ нет директивы AudioPlayDirective.
        # Включаем следующий трек, проверяем, что он соответствует неперемешанному порядку

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': '2774889'
                    },
                    'order': {
                        'order_value': 'shuffle'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        directives = layout.Directives
        assert_audio_play_directive(directives, title='Hey You', sub_title='Pink Floyd')
        assert not layout.OutputSpeech

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=True)

        payload = {
            'typed_semantic_frame': {
                'player_unshuffle_semantic_frame': {
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_unshuffle'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=False)

        r = alice(voice('следующий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title=next_title, sub_title='Pink Floyd')
        assert not layout.OutputSpeech

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=False)

    @pytest.mark.experiments(
        *EXPERIMENTS_PLAYLIST,
    )
    @pytest.mark.parametrize('object_id, object_type, unexpected_next_title', [
        pytest.param('298151', 'Album', 'Is There Anybody Out There?', id='album'),
        pytest.param('82939', 'Artist', 'Comfortably Numb', id='artist'),
        pytest.param('457553308:82939', 'Playlist', 'Comfortably Numb', id='playlist'),
    ])
    def test_shuffle(self, alice, object_id, object_type, unexpected_next_title):
        # Включаем перемешанную сущность, проверяем, что в AudioPlayDirective установлен флаг shuffled в true.
        # Отключаем перемешивание. Воспроизведениен не прерывается, в ответ нет директивы AudioPlayDirective.
        # Включаем следующий трек, проверяем, что он соответствует неперемешанному порядку
        # Тест может падать при переканонизации при изменении порядка треков в используемых альбоме,
        # исполнителе, плейлисте

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': '2774889'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        directives = layout.Directives
        assert_audio_play_directive(directives, title='Hey You', sub_title='Pink Floyd')
        assert not layout.OutputSpeech

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=False)

        payload = {
            'typed_semantic_frame': {
                'player_shuffle_semantic_frame': {
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_shuffle'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=True)

        r = alice(voice('следующий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert directives[0].AudioPlayDirective.AudioPlayMetadata.Title != unexpected_next_title
        assert_audio_play_directive(directives, sub_title='Pink Floyd')
        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=True)

    def test_repeat_one(self, alice):
        # Включаем альбом с определённого трека на повторе одного трека, проверяем, что в GlagolMetadata установлен режим повтора.
        # Вызываем get_next, проверяем, что включается тот же трек.
        # Отключаем повтор. Воспроизведениен не прерывается, в ответе есть только директива SetGlagolMetadata.
        # Вызываем get_next, проверяем, что включается следующий трек

        object_id = '298151'
        object_type = 'Album'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': '2774889'
                    },
                    'repeat': {
                        'repeat_value': 'One'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        directives = layout.Directives
        assert_audio_play_directive(directives, title='Hey You', sub_title='Pink Floyd')
        assert not layout.OutputSpeech

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(directives, title='Hey You', sub_title='Pink Floyd')
        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One)
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        payload = {
            'typed_semantic_frame': {
                'player_repeat_semantic_frame': {
                    'mode': {
                        'enum_value': 'None'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_repeat'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert len(r.run_response.ResponseBody.StackEngine.Actions) == 0

        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None"))

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Is There Anybody Out There?', sub_title='Pink Floyd')

    def test_repeat_all(self, alice):
        # Включаем альбом на повторе с послоеднего трека, проверяем, что в GlagolMetadata установлен режим повтора.
        # Вызываем get_next, проверяем, что включается первый трек альбома.
        # Отключаем повтор. Воспроизведениен не прерывается, в ответе есть только директива SetGlagolMetadata

        object_id = '298151'
        object_type = 'Album'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': '2774870'
                    },
                    'repeat': {
                        'repeat_value': 'All'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        directives = layout.Directives
        assert_audio_play_directive(directives, title='High Hopes', sub_title='Pink Floyd')
        assert not layout.OutputSpeech

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.All)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Astronomy Domine', sub_title='Pink Floyd')
        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.All)
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        payload = {
            'typed_semantic_frame': {
                'player_repeat_semantic_frame': {
                    'mode': {
                        'enum_value': 'None'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_repeat'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert len(r.run_response.ResponseBody.StackEngine.Actions) == 0

        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None"))

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_not_disabling_nlg_for_thick_player(self, alice):
        # Включаем трек semantic frame'ом с disable_nlg: true.
        # В Scenario state запоминается disable_nlg.
        # Проверяем, что не отключается при голосовом запросе радио,
        # который обрабатывается толстым плеером

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '18860',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        r = alice(voice('включи весёлую музыку'))
        layout = r.continue_response.ResponseBody.Layout

        assert layout.OutputSpeech

        directives = layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')

    def test_track_with_start_from_track_id(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '92175351',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                    'start_from_track_id': {
                        'string_value': '92175351'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)


@pytest.mark.parametrize('surface', [surface.station])
# surface.station has 'multiroom' feature in its app_preset by default
# We just want to declare it explicitly
@pytest.mark.supported_features('multiroom')
class TestsMultiroom(TestsBase):

    ROOM_ID_ALL = '__all__'

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_multiroom_artist(self, alice):
        r = alice(voice('включи queen везде'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'.*Queen.*', layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2

        start_multiroom = directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == TestsMultiroom.ROOM_ID_ALL

        music_play = directives[1].MusicPlayDirective
        assert music_play.SessionId
        assert music_play.RoomId == TestsMultiroom.ROOM_ID_ALL

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_multiroom_radio(self, alice):
        r = alice(voice('включи рок везде'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'.*[Рр]ок.*', layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2

        start_multiroom = directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == TestsMultiroom.ROOM_ID_ALL

        music_play = directives[1].MusicPlayDirective
        assert music_play.SessionId
        assert music_play.RoomId == TestsMultiroom.ROOM_ID_ALL


@pytest.mark.parametrize('surface', [surface.station])
class TestAudiobranding(TestsBase):

    @pytest.mark.experiments('hw_music_thin_client', 'yamusic_audiobranding_score=1')
    def test_audiobranding_general_playlist(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'.*Яндекс.Музык.*', layout.OutputSpeech)
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('AudioPlayDirective')
        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
class TestKeepScenarioState(TestsBase):

    @pytest.mark.device_state(device_config={
        'content_settings': 'children',
    })
    def test_children_mode_keeps_scenario_state(self, alice):
        r = alice(voice('включи песню фиксиков вентилятор'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('включи моргенштерна'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == \
            'Знаю такое, но не могу поставить в детском режиме.'
        state = get_scenario_state(r.continue_response)
        assert state.HasField('Queue')

    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/bass_megamind_prepare_music_found.json'),
            HttpResponseStub(200, 'freeze_stubs/bass_megamind_prepare_music_not_found.json'),
        ],
    })
    def test_music_not_found_keeps_scenario_state(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('включи enchantimals'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.'
        state = get_scenario_state(r.run_response)
        assert state.HasField('Queue')


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
class TestPlayerFeatures(TestsBase):

    @pytest.mark.device_state(audio_player={
        'player_state': 'Playing',
        'last_play_timestamp': (ALICE_START_TIME.timestamp() - 42) * 1000,
        'scenario_meta': {
            'owner': 'music',
        },
    })
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_return_player_features_when_no_scenario_state(self, alice):
        '''
        Тонкий плеер играет какую-то музыку. Однако по какой-то причине потерялся стейт сценария.
        Прилетает плеерная команда ("дальше"). Нужно, чтобы
            - вернулись плеерные фичи, как обычно для плеерных команд
            - включилось пользовательское радио ("включи музыку" на толстом плеере)
        '''
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.run_response.Features.HasField('PlayerFeatures')
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause == 42

        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField('MusicPlayDirective')


EXPECTED_ACTIONS = {
    'what_is_playing': [
        {
            'id': 'player_what_is_playing',
            'name': 'player what is playing',
            'human_readable': 'Говорит, какая песня играет',
        }
    ],
    'next': [
        {
            'id': 'player_next_track',
            'name': 'player next track',
            'human_readable': 'Включается следующий музыкальный трек'
        }
    ],
    'prev': [
        {
            'id': 'player_previous_track',
            'name': 'player previous track',
            'human_readable': 'Включается предыдущий музыкальный трек'
        }
    ],
    'like': [
        {
            'id': 'player_like',
            'name': 'player like',
            'human_readable': 'Трек отмечается как понравившийся'
        }
    ],
    'dislike': [
        {
            'id': 'player_dislike',
            'name': 'player dislike',
            'human_readable': 'Трек отмечается как непонравившийся'
        },
        {
            'id': 'player_next_track',
            'name': 'player next track',
            'human_readable': 'Включается следующий музыкальный трек'
        }
    ],
    'rewind': [
        {
            'id': 'player_rewind',
            'name': 'player rewind',
            'human_readable': 'Перематывает вперед на 30 секунд'
        }
    ],
    'shuffle': [
        {
            'id': 'player_shuffle',
            'name': 'player shuffle',
            'human_readable': 'Воспроизведение происходит в случайном порядке'
        }
    ],
    'continue': [
        {
            'id': 'player_continue',
            'name': 'player continue',
            'human_readable': 'Включается музыка после паузы (или просто продолжается воспроизведение)'
        }
    ],
    'repeat': [
        {
            'id': 'player_repeat',
            'name': 'player repeat',
            'human_readable': 'Включается режим повтора'
        }
    ],
    'replay': [
        {
            'id': 'player_replay',
            'name': 'player replay',
            'human_readable': 'Музыкальный трек проигрывается еще раз c начала'
        }
    ],
}


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('music_force_show_first_track')  # flag from
class TestsBadScenarioStateCommand(TestsBase):

    @pytest.mark.parametrize('command, expected_actions', [
        pytest.param('следующий', EXPECTED_ACTIONS['next'], id='next'),
        pytest.param('предыдущий', EXPECTED_ACTIONS['prev'], id='prev'),
        pytest.param('лайк', EXPECTED_ACTIONS['like'], id='like'),
        pytest.param('дизлайк', EXPECTED_ACTIONS['dislike'], id='dislike'),
        pytest.param('перемотай на 30 секунд вперёд', EXPECTED_ACTIONS['rewind'], id='rewind'),
        pytest.param('перемешай музыку', EXPECTED_ACTIONS['shuffle'], id='shuffle'),
        pytest.param('продолжай', EXPECTED_ACTIONS['continue'], id='continue'),
        pytest.param('поставь на повтор', EXPECTED_ACTIONS['repeat'], id='repeat'),
        pytest.param('включи заново', EXPECTED_ACTIONS['replay'], id='replay'),
    ])
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_fallback_radio(self, alice, command, expected_actions):
        '''
        Получаем команду с незаполненным стейтом сценария, в случае победы HollywoodMusic включаем персональное радио
        на толстом плеере, проверяем, что AnalyticsInfo.Actions заполнены в соответствии с командой
        '''
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        analytics_info = r.continue_response.ResponseBody.AnalyticsInfo

        assert len(analytics_info.Actions) == len(expected_actions)
        for action_proto, expected_action in zip(analytics_info.Actions, expected_actions):
            action = proto_to_dict(action_proto)
            assert action == expected_action

        assert len(analytics_info.Objects) == 0

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')

    @pytest.mark.device_state(audio_player={
        'player_state': 'Playing',
        'scenario_meta': {
            'owner': 'music',
            'queue_item': '''
CgcyNzU4MDA5GhNUaGUgU2hvdyBNdXN0IEdvIE9uMkNhdmF0YXJzLnlhbmRleC5uZXQvZ2V0LW11c2ljLWNvbnRlbnQvMTE3NTQ2L2M4YzY4ZjQzLmEuNTkwMTIy
MC0xLyUlOroBCrABaHR0cHM6Ly9zdG9yYWdlLm1kcy55YW5kZXgubmV0L2dldC1hbGljZS8xNzcwNjAzXzc5ZTAzMTQ2LjkxODkwNTkwLjEwLjI3NTgwMDkvMzIw
P3NpZ249NzJhMGMyZTU0ZTFjYTUxYTFmZjliZmU3YzRlNjZhNGM1MDA4ZDkyZGQ0NmQ0ZDk2MjY4MGI5MDFlMjY1ZmVjNCZ0cz02MjU5NjhiOSZvZmZzZXQ9NDI0
NDcQmrXZhvwtSgxlaGd0RmxUcFVhU0JQ7MwQggEFbXVzaWOaAZMBCgc1OTAxMjIwEhFCb2hlbWlhbiBSaGFwc29keRoFZmlsbXMqBTc5MjE1Mg4KBVF1ZWVuGgU3
OTIxNTgBQAFKQ2F2YXRhcnMueWFuZGV4Lm5ldC9nZXQtbXVzaWMtY29udGVudC8xMTc1NDYvYzhjNjhmNDMuYS41OTAxMjIwLTEvJSVSDgoFUXVlZW4aBTc5MjE1
qgEJCAISBTc5MjE1sgESCQrXo3A9iiPAESlcj8L1KMy/
''',
        },
    })
    def test_what_is_playing(self, alice):
        '''
        Получаем команду с незаполненным стейтом сценария и заполненным ответом на "что сейчас играет?"
        в device_state'е. Проверяем, что AnalyticsInfo.Actions не заполнены
        '''
        r = alice(voice('что сейчас играет?'))
        assert r.scenario_stages() == {'run'}
        response = r.run_response

        assert 'The Show Must Go On' in response.ResponseBody.Layout.OutputSpeech

        analytics_info = response.ResponseBody.AnalyticsInfo

        assert len(analytics_info.Actions) == 0
        assert len(analytics_info.Objects) == 0
        assert len(response.ResponseBody.Layout.Directives) == 0
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-935


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
class TestsBadScenarioStateCommandThinClientRadio(TestsBadScenarioStateCommand):

    @pytest.mark.parametrize('command, expected_actions', [
        pytest.param('следующий', EXPECTED_ACTIONS['next'], id='next'),
        pytest.param('предыдущий', EXPECTED_ACTIONS['prev'], id='prev'),
        pytest.param('лайк', EXPECTED_ACTIONS['like'], id='like'),
        pytest.param('дизлайк', EXPECTED_ACTIONS['dislike'], id='dislike'),
        pytest.param('перемотай на 30 секунд вперёд', EXPECTED_ACTIONS['rewind'], id='rewind'),
        pytest.param('перемешай музыку', EXPECTED_ACTIONS['shuffle'], id='shuffle'),
        pytest.param('продолжай', EXPECTED_ACTIONS['continue'], id='continue'),
        pytest.param('поставь на повтор', EXPECTED_ACTIONS['repeat'], id='repeat'),
        pytest.param('включи заново', EXPECTED_ACTIONS['replay'], id='replay'),
    ])
    def test_fallback_radio(self, alice, command, expected_actions):
        '''
        Получаем команду с незаполненным стейтом сценария, в случае победы HollywoodMusic включаем персональное радио
        на тонком плеере, проверяем, что AnalyticsInfo.Actions заполнены в соответствии с командой
        '''
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        analytics_info = r.continue_response.ResponseBody.AnalyticsInfo

        assert len(analytics_info.Actions) == len(expected_actions)
        for action_proto, expected_action in zip(analytics_info.Actions, expected_actions):
            action = proto_to_dict(action_proto)
            assert action == expected_action

        assert len(analytics_info.Objects) == 0

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)


@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.station])
class TestsBiometryIncognitoMode(TestsBase):

    OWNER_BIOMETRY = Biometry(is_known=True, known_user_id='1083955728')
    INCOGNITO_BIOMETRY = Biometry(is_known=False, known_user_id='1083955728')

    @pytest.mark.region(region.Madrid)
    @pytest.mark.parametrize('command', [
        pytest.param('включи песню seether fine again', id='track'),
        pytest.param('включи альбом the dark side of the moon', id='album'),
        pytest.param('включи depeche mode', id='artist'),
        pytest.param('включи плейлист хрючень брудень и элекок', marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST),
                     id='user_playlist'),
        pytest.param('включи плейлист дня', marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST),
                     id='special_playlist'),
    ])
    def test_play_music_incognito_from_abroad(self, alice, command):
        r = alice(voice(command, biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, incognito=True)

    @pytest.mark.region(region.Madrid)
    @pytest.mark.parametrize('command', [
        pytest.param('включи песню seether fine again', id='track'),
        pytest.param('включи альбом the dark side of the moon', id='album'),
        pytest.param('включи depeche mode', id='artist'),
        pytest.param('включи плейлист хрючень брудень и элекок', marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST),
                     id='user_playlist'),
        pytest.param('включи плейлист дня', marks=pytest.mark.experiments(*EXPERIMENTS_PLAYLIST),
                     id='special_playlist'),
    ])
    def test_play_music_owner_from_abroad(self, alice, command):
        r = alice(voice(command, biometry=TestsBiometryIncognitoMode.OWNER_BIOMETRY))
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

    def test_play_artist_like_biometry_known_user(self, alice):
        r = alice(voice('включи queen', biometry=TestsBiometryIncognitoMode.OWNER_BIOMETRY))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('поставь лайк', biometry=TestsBiometryIncognitoMode.OWNER_BIOMETRY))
        return str(r)

    def test_play_artist_like_biometry_incognito_user(self, alice):
        r = alice(voice('включи queen', biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, incognito=True)

        r = alice(voice('поставь лайк', biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        return str(r)

    def test_play_artist_dislike_biometry_known_user(self, alice):
        r = alice(voice('включи киркорова', biometry=TestsBiometryIncognitoMode.OWNER_BIOMETRY))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('поставь дизлайк', biometry=TestsBiometryIncognitoMode.OWNER_BIOMETRY))
        return str(r)

    def test_play_artist_dislike_biometry_incognito_user(self, alice):
        r = alice(voice('включи киркорова', biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, incognito=True)

        r = alice(voice('поставь дизлайк', biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        return str(r)

    def test_no_rights(self, alice):
        '''
        Этот трек https://music.yandex.ru/album/4769067/track/37586336 на момент создания теста
        имеет какие-то ограниченные права доступа. Поэтому если попытаться сделать на него URL
        от имени гостя (колонкиш аккаунта), то получаем в ответ 403 с сообщением no-rights.
        Надо ходить за URL-ом от имени владельца колонки.
        '''
        r = alice(voice('включи бежин луг', biometry=TestsBiometryIncognitoMode.INCOGNITO_BIOMETRY))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, incognito=True)


@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.station])
class TestsPumpkinRadio(TestsBase):

    def test_thin_to_thick_degradation(self, alice):
        '''
        Если у нас был запрос в Радио, то возвращается колбэк на feedback-запрос
        Когда радио отключится, нужно игнорировать эти колбэки, не выполнять их
        Это будет видно по стаб-файлам
        '''
        name = 'music_thin_client_on_finished'
        payload = {
            'events': [{
                'playAudioEvent': {
                    'uid': '1083955728',
                    'from': 'hollywood-radio',
                    'trackId': '12345',
                    'playId': '12345',
                    'radioSessionId': '12345',
                },
            }, {
                'radioFeedbackEvent': {
                    'type': 'TrackFinished',
                    'trackId': '12345',
                    'batchId': '12345',
                    'radioSessionId': '12345',
                },
            }],
            '@scenario_name': 'HollywoodMusic',
        }

        r = alice(server_action(name, payload))
        assert r.scenario_stages() == {'run', 'commit'}
        return str(r)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    def test_no_feedback(self, alice):
        '''
        Любые фидбек-запросы в тыкву должны игнорироваться
        Это будет видно по стаб-файлам
        '''
        name = 'music_thin_client_on_finished'
        payload = {
            'events': [{
                'playAudioEvent': {
                    'uid': '1083955728',
                    'from': 'hollywood-radio',
                    'trackId': '12345',
                    'playId': '12345',
                    'radioSessionId': '<PUMPKIN>',
                },
            }, {
                'radioFeedbackEvent': {
                    'type': 'TrackFinished',
                    'trackId': '12345',
                    'batchId': '12345',
                    'radioSessionId': '<PUMPKIN>',
                },
            }],
            '@scenario_name': 'HollywoodMusic',
        }

        r = alice(server_action(name, payload))
        assert r.scenario_stages() == {'run', 'commit'}
        return str(r)

    @pytest.mark.experiments(*EXPERIMENTS_RADIO)
    @pytest.mark.freeze_stubs(music_back_stubber={
        '/external-rotor/session/new': HttpResponseStub(200, 'freeze_stubs/radio_session_new_pumpkin.json'),
    })
    def test_play_music_but_pumpkin(self, alice):
        r = alice(voice('включи музыку'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        audio_play_directive = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']
        logger.info(f'audio_play_directive={audio_play_directive}')
        assert_that(audio_play_directive['callbacks'], has_entries({
            'on_started': has_entries({
                'name': 'music_thin_client_on_started',
                'payload': has_entries({
                    'events': contains(has_entries({
                        'playAudioEvent': has_entries({
                            'from': 'alice-discovery-radio-pumpkin',  # Хотим убедиться что from правильный для тыквы
                        })
                    }), not_none(), not_none())
                })
            })
        }))


@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsFixlist(TestsBase):
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_fixlist_playlist(self, alice):
        '''
        https://st.yandex-team.ru/ALICE-12337 Слово 'машины' разбиралось в слот activity:driving и
        на тонком плеере включалось радио... Очевидно, что фикслист должен иметь более высший приоритет над радио.
        '''
        r = alice(voice('включи машины истории'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == 'Включаю Машины сказки.'

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')


@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_PLAYLIST)
@pytest.mark.parametrize('surface', [surface.station])
class TestsScrollEntity(TestsBase):
    def test_listening_consequential_tracks(self, alice):
        object_id = '44612130:1000'
        object_type = 'Playlist'
        start_from_track_id = '46059463'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': start_from_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Something More')

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Revolution')

        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[0].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, title='Ark')


@pytest.mark.parametrize('surface', [surface.station_pro])
class TestDrawLedScreen(TestsBase):
    def test_commands(self, alice):
        r = alice(voice('включи eminem'))
        assert r.scenario_stages() == {'run', 'continue'}

        r = alice(voice('следующий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        draw_led_screen = directives[0].DrawLedScreenDirective

        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/next.gif'

        r = alice(voice('предыдущий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        draw_led_screen = directives[0].DrawLedScreenDirective
        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/previous.gif'

        r = alice(voice('пауза', scenario=(Scenario('Commands', 'fast_command'))))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        draw_led_screen = directives[0].DrawLedScreenDirective
        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/pause.gif'

        r = alice(voice('продолжи'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        draw_led_screen = directives[0].DrawLedScreenDirective
        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/resume.gif'

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        directives = r.run_response.CommitCandidate.ResponseBody.Layout.Directives
        assert len(directives) == 1
        draw_led_screen = directives[0].DrawLedScreenDirective
        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/like.gif'

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        directives = r.apply_response.ResponseBody.Layout.Directives
        draw_led_screen = directives[0].DrawLedScreenDirective
        assert draw_led_screen.DrawItem[0].FrontalLedImage == 'https://static-alice.s3.yandex.net/led-production/player/dislike.gif'


@pytest.mark.supported_features('scled_display')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestDrawScledScreen(TestsBase):
    def test_commands(self, alice):
        r = alice(voice('включи eminem'))
        assert r.scenario_stages() == {'run', 'continue'}

        r = alice(voice('следующий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        r = alice(voice('предыдущий'))
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        r = alice(voice('пауза', scenario=(Scenario('Commands', 'fast_command'))))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        r = alice(voice('продолжи'))

        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        directives = r.run_response.CommitCandidate.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        directives = r.apply_response.ResponseBody.Layout.Directives

        assert len(directives) == 3
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(
    *EXPERIMENTS,
    *EXPERIMENTS_PLAYLIST,
    *EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestMuzpult(object):

    owners = ('vitvlkv',)

    @pytest.mark.oauth(auth.Yandex)
    def test_audio_play_without_plus(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3284
        """
        payload = {
            "typed_semantic_frame": {
                "music_play_semantic_frame": {
                    "disable_nlg": {
                        "bool_value": True
                    },
                    "object_type": {
                        "enum_value": "Artist"
                    },
                    "start_from_track_id": {
                        "string_value": "50685846"
                    },
                    "object_id": {
                        "string_value": "41052"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "play_music"
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in [
            'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
            'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
            'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
        ]


@pytest.mark.experiments(
    *EXPERIMENTS_PLAYLIST,
)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.device_state(device_config={
    'content_settings': 'safe',
})
class TestsSFSafeContentSettings(TestsBase):
    def test_first_track_id(self, alice):
        first_track_id = '19781921'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': '44612130:1000',
                    },
                    'object_type': {
                        'enum_value': 'Playlist',
                    },
                    'start_from_track_id': {
                        'string_value': first_track_id
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Лучше послушай эту музыку вместе с родителями.'
        assert len(layout.Directives) == 0


@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_RADIO, *EXPERIMENTS_PLAYLIST)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_music_enable_prefetch_get_next_correction')
class TestsFixPrefetchBug(TestsBase):
    def test_ondemand_next_track(self, alice):
        r = alice(voice('включи beatles live at the bbc'))
        # In this album we have this tracks order: [42014773, 42014776, 42014777]
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id == '42014773'

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        # This simulates the bug: the scenario state is one track behind the device state
        alice.update_audio_player_current_stream_stream_id(stream_id='42014776')

        r = alice(voice('следующий трек'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        second_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert second_track_id == '42014777'

    def test_ondemand_next_track_callback(self, alice):
        r = alice(voice('включи beatles live at the bbc'))
        # In this album we have this tracks order: [42014773, 42014776, 42014777]
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id == '42014773'

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        # This simulates the bug: the scenario state is one track behind the device state
        alice.update_audio_player_current_stream_stream_id(stream_id='42014776')

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.is_run_relevant_with_second_scenario_stage('continue')
        directives = r.continue_response.ResponseBody.Layout.Directives
        second_track_id = directives[0].AudioPlayDirective.Stream.Id
        assert second_track_id == '42014777'

    def test_ondemand_dislike(self, alice):
        r = alice(voice('включи beatles live at the bbc'))
        # In this album we have this tracks order: [42014773, 42014776, 42014777]
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        first_track_id = get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo)
        assert first_track_id == '42014773'

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        # This simulates the bug: the scenario state is one track behind the device state
        alice.update_audio_player_current_stream_stream_id(stream_id='42014776')

        r = alice(voice('дизлайк'))
        assert r.is_run_relevant_with_second_scenario_stage('apply')
        directives = r.apply_response.ResponseBody.Layout.Directives
        second_track_id = directives[0].AudioPlayDirective.Stream.Id
        assert second_track_id == '42014777'


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsNeedSimilar(TestsBase):
    def test_need_similar_on_demand(self, alice):
        # Используется music_play_semantic_frame, аналогичный тому, который будет разобран бегемотом
        # после переобучения теггера для разбора слота need_similar
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'search_text': {
                        'string_value': 'eminem',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        # Artist's id is 1053
        assert_glagol_metadata(glagol_metadata, object_type='Radio', object_id='artist:1053')

        state = get_scenario_state(r.continue_response)
        radio_session_id = state.Queue.CurrentContentLoadingState.Radio.SessionId
        assert radio_session_id

        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'Включаю: .+\.', layout.Cards[0].Text)
        assert re.match(r'Включаю .+', layout.OutputSpeech)

    @pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_need_similar_playlist(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'playlist': {
                        'string_value': 'плейлист хрючень брудень и элекок',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        # Radio seed from music back is user:onyourwave for radio by playlist
        assert_glagol_metadata(glagol_metadata, object_type='Radio', object_id='user:onyourwave')

        state = get_scenario_state(r.continue_response)
        radio_session_id = state.Queue.CurrentContentLoadingState.Radio.SessionId
        assert radio_session_id

    def test_need_similar_after_on_demand(self, alice):
        r = alice(voice('включи eminem'))
        assert r.scenario_stages() == {'run', 'continue'}

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        # Artist's id is 1053
        assert_glagol_metadata(glagol_metadata, object_type='Radio', object_id='artist:1053')

        state = get_scenario_state(r.continue_response)
        radio_session_id = state.Queue.CurrentContentLoadingState.Radio.SessionId
        assert radio_session_id

        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'Включаю: .+\.', layout.Cards[0].Text)
        assert re.match(r'Включаю .+', layout.OutputSpeech)

    @pytest.mark.experiments(*EXPERIMENTS_PLAYLIST)
    def test_need_similar_after_playlist(self, alice):
        r = alice(voice('включи плейлист хрючень брудень и элекок'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '1035351314:1000'

        r = alice(voice('включи похожее'))
        assert r.scenario_stages() == {'run', 'continue'}
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Radio

        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'Включаю: .+\.', layout.Cards[0].Text)
        assert re.match(r'Включаю .+', layout.OutputSpeech)

    def test_need_similar_without_state(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type='Radio', object_id='user:onyourwave')

        state = get_scenario_state(r.continue_response)
        radio_session_id = state.Queue.CurrentContentLoadingState.Radio.SessionId
        assert radio_session_id

        layout = r.continue_response.ResponseBody.Layout
        assert layout.Cards[0].Text == 'Вот что я нашла для вас на Яндекс.Музыке. Надеюсь, понравится.'
        assert layout.OutputSpeech == 'Вот что я нашла для вас на Яндекс Музыке. Надеюсь, понравится.'

    def test_need_similar_radio(self, alice):
        # Включи похожее на рок
        # Проверяем, что включаем радио с seed = genre:orck
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'genre': {
                        'genre_value': 'rock',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert_audio_play_directive(layout.Directives)
        glagol_metadata = layout.Directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type='Radio', object_id='genre:rock')

        state = get_scenario_state(r.continue_response)
        radio_session_id = state.Queue.CurrentContentLoadingState.Radio.SessionId
        assert radio_session_id

        layout = r.continue_response.ResponseBody.Layout
        assert re.match(r'Включаю: .+\.', layout.Cards[0].Text)
        assert re.match(r'Включаю .+', layout.OutputSpeech)


@pytest.mark.parametrize('surface', [surface.station])
class TestsNeedSimilarThickRadio(TestsBase):
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_need_similar_on_demand(self, alice):
        # Проверяем без флагов EXPERIMENTS_RADIO, но с флагами EXPERIMENTS
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'search_text': {
                        'string_value': 'eminem',
                    },
                    'need_similar': {
                        'need_similar_value': 'need_similar'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')


@pytest.mark.experiments(*EXPERIMENTS, *EXPERIMENTS_PLAYLIST, *EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsSlotsCombo(TestsBase):

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_ondemand_with_radio_slot(self, alice):
        '''
        В таком случае должно просто включиться ac/dc.
        '''
        r = alice(voice('включи ac/dc для тренировки'))
        assert r.is_run_relevant_with_second_scenario_stage('continue')

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        event = next(filter(lambda event: event.HasField('MusicEvent'),
                     r.continue_response.ResponseBody.AnalyticsInfo.Events))
        assert event.MusicEvent.Id == '191175'
        assert event.MusicEvent.AnswerType == TMusicEvent.EAnswerType.Artist


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsTurnOnRadioCallbackAfterStateChange(TestsBase):
    def test_ondemand_next_track(self, alice):
        expected_object_type = 'Track'
        expected_object_id = '15675'

        r = alice(voice('включи abba money'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Money, Money, Money', sub_title='ABBA')

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(
            glagol_metadata,
            expected_object_type,
            expected_object_id,
            repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None")
        )

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        r = alice(voice('повторяй'))
        assert r.scenario_stages() == {'run'}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1

        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(
            glagol_metadata,
            expected_object_type,
            expected_object_id,
            repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One
        )

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Money, Money, Money', sub_title='ABBA')
        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(
            glagol_metadata,
            expected_object_type,
            expected_object_id,
            repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One
        )

    def test_repeat_last_track(self, alice):
        # Включаем альбом с последнего трека на повторе одного трека. Сохраняем get_next
        # Отключаем повтор
        # Вызываем get_next, проверяем, что включается радио по альбому

        object_id = '298151'  # Pink Floyd, альбом "The Discovery Boxset"
        object_type = 'Album'

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': object_type,
                    },
                    'start_from_track_id': {
                        'string_value': '2774870'  # Песня "High Hopes", последняя в альбоме
                    },
                    'repeat': {
                        'repeat_value': 'One'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        directives = layout.Directives
        assert_audio_play_directive(directives, title='High Hopes', sub_title='Pink Floyd')

        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode.One)

        assert r.continue_response.ResponseBody.StackEngine.Actions[0].NewSession
        reset_add = r.continue_response.ResponseBody.StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')

        payload = {
            'typed_semantic_frame': {
                'player_repeat_semantic_frame': {
                    'mode': {
                        'enum_value': 'None'
                    },
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_repeat'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        directives = layout.Directives
        assert len(directives) == 1
        assert len(r.run_response.ResponseBody.StackEngine.Actions) == 0

        glagol_metadata = directives[0].SetGlagolMetadataDirective.GlagolMetadata
        assert_glagol_metadata(glagol_metadata, object_type, object_id, repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None"))

        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives)
        glagol_metadata = directives[0].AudioPlayDirective.AudioPlayMetadata.GlagolMetadata
        # Content id refers to a single track while radio accepted set of tracks from album
        # https://st.yandex-team.ru/HOLLYWOOD-566
        # TODO(sparkle): it gives a new seed on each re-canonization, check with regex 'track:\d+'
        assert_glagol_metadata(glagol_metadata, 'Radio', 'track:2773127', repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None"))


@pytest.mark.experiments(*EXPERIMENTS_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestRepeatedSkip(TestsBase):

    def test_repeated_skip(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}

        # test next track
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 2

        # test continue
        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0

        # test dislike
        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert get_scenario_state(r.apply_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('дизлайк'))
        assert r.scenario_stages() == {'run', 'apply'}
        assert get_scenario_state(r.apply_response).RepeatedSkipState.SkipCount == 2

        # test like
        r = alice(voice('лайк'))
        assert r.scenario_stages() == {'run', 'commit'}
        assert get_scenario_state(r.run_response).RepeatedSkipState.SkipCount == 0

        # test prev track
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0

        # test what is playing
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        assert get_scenario_state(r.run_response).RepeatedSkipState.SkipCount == 0

        # test shuffle
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('перемешай'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0

        # test repeat
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('поставь на повтор'))
        assert r.scenario_stages() == {'run'}
        assert get_scenario_state(r.run_response).RepeatedSkipState.SkipCount == 0

        # test replay
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('включи заново'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0

        # test rewind
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        r = alice(voice('перемотай на 30 секунд вперёд'))
        assert r.scenario_stages() == {'run'}
        assert get_scenario_state(r.run_response).RepeatedSkipState.SkipCount == 0

        # test unshuffle
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        payload = {
            'typed_semantic_frame': {
                'player_unshuffle_semantic_frame': {
                    'disable_nlg': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'RemoteControl',
                'purpose': 'player_unshuffle'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0

        # test track finish
        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, 'music_thin_client_on_finished'))
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}
        assert get_scenario_state(r.run_response).RepeatedSkipState.SkipCount == 0

    @pytest.mark.experiments('hw_music_repeated_skip_threshold=2')
    def test_repeated_skip_render(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}

        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 1
        body = r.continue_response.ResponseBody
        assert not body.ExpectsRequest
        layout = body.Layout
        assert not layout.OutputSpeech
        assert_audio_play_directive(layout.Directives)

        r = alice(voice('дальше'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert get_scenario_state(r.continue_response).RepeatedSkipState.SkipCount == 0
        body = r.continue_response.ResponseBody
        assert not body.ExpectsRequest
        layout = body.Layout
        assert layout.OutputSpeech in (
            'Кажется, наши вкусы сегодня не совпадают. Что вы хотите послушать?',
            'Похоже, вам не нравится то, что я ставлю. Что прикажете?',
            'Что вам включить, чтобы вам понравилось?',
            'Что вам включить, чтобы вас порадовать?',
            'А давайте послушаем то, что вам нравится. Что вам включить?',
        )
        assert layout.Directives[0].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[1].HasField('ListenDirective')
        assert_audio_play_directive(layout.Directives[2:])


@pytest.mark.parametrize('surface', [surface.station])
class TestPlayOndemandAsAlbum(TestsBase):

    def test_play_on_demand_track_inside_album(self, alice):
        r = alice(voice('включи дюна 21 часть'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Фрэнк Герберт. «Дюна». Часть 21')
        state = get_scenario_state(r.continue_response)

        # Maybe later this should be changed to album
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Track
        assert state.Queue.PlaybackContext.ContentId.Id == '91288201'

    def test_play_ondemand_podcast_as_album(self, alice):
        response = alice(voice('Включи подкаст как выстраивать свое питание'))
        assert response.scenario_stages() == {'run', 'continue'}

        directives = response.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='54: Как выстраивать свое питание')
        state = get_scenario_state(response.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Album
        assert state.Queue.PlaybackContext.ContentId.Id == "6880227"
        assert state.Queue.PlaybackContext.StartFromTrackId == "94583725"


@pytest.mark.experiments(*EXPERIMENTS_RADIO, 'hw_music_announce')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.memento({
    "UserConfigs": {
        "MusicConfig": {
            "AnnounceTracks": True
        }
    }
})
class TestTrackAnnounce(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи музыку', id='radio'),
        pytest.param('включи queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', id='track'),
        pytest.param('включи альбом dark side of the moon', id='album'),
    ])
    def test_track_announce(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        out = text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        out += "\n# next track\n" + text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        return out

    def test_track_announce_song_next_prev(self, alice):
        bad_re = re.compile('песня.*(песня|песенка)', re.IGNORECASE)
        r = alice(voice('включи песенка верещагина'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert not bad_re.search(r.continue_response.ResponseBody.Layout.OutputSpeech)
        out = text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert not bad_re.search(r.continue_response.ResponseBody.Layout.OutputSpeech)
        out += "\n# prev track (after next track)\n" + text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        return out

    @pytest.mark.experiments('hw_music_announce_album')
    def test_track_announce_album(self, alice):
        r = alice(voice('включи вечные хиты'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        out = text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        out += "\n# next track\n" + text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        return out

    @pytest.mark.oauth(auth.RobotMultiroom)  # ugc tracks are available only for owner
    def test_play_ugc(self, alice):
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': 'ec5f857b-75f3-4065-a587-a62e46a2f7c7',
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        out = text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == '<[domain music]>Песня "Первый трек.mp3".<[/domain]>'
        out += "\n# prev track (after next track)\n" + text_format.MessageToString(r.continue_response.ResponseBody.Layout, as_utf8=True)
        return out


@pytest.mark.experiments(*EXPERIMENTS_RADIO, 'hw_music_send_song_text')
@pytest.mark.parametrize('surface', [surface.station])
class TestSendSongText(TestsBase):

    @pytest.mark.parametrize('what, is_missing', [
        pytest.param('кобзон', False, id='song'),
        pytest.param('чайковский', True, id='classics'),
        pytest.param('хрум', True, id='podcast'),
    ])
    def test_send_song_text(self, alice, what, is_missing):
        r = alice(voice('включи ' + what))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('пошли текст песни'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.send_song_text'

        check_directive = has_entries({
            'response_body': has_entries({
                'ServerDirectives': contains(has_entries({
                    'SendPushDirective': has_entries({
                        'push_tag': 'alice_music_song_text'
                    }),
                })),
            }),
        })
        if is_missing:
            check_directive = is_not(check_directive)
        assert_that(r.run_response_pyobj, check_directive)

        check_action = has_entries({
            'response_body': has_entries({
                'analytics_info': has_entries({
                    'actions': contains(has_entries({
                        'id': 'send_song_text'
                    })),
                }),
            }),
        })
        if is_missing:
            check_action = is_not(check_action)
        assert_that(r.run_response_pyobj, check_action)

        return str(r)

    def test_not_playing(self, alice):
        r = alice(voice('пошли текст песни'))
        assert 'Как только вы включите музыку, я смогу прислать текст.' == r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)


@pytest.mark.experiments(
    *EXPERIMENTS_RADIO,
    'hw_music_what_year_is_this_song',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestWhatYearIsThisSong(TestsBase):

    @pytest.mark.parametrize('what, is_missing', [
        pytest.param('кобзон', False, id='song'),
        pytest.param('чайковский', False, id='classics'),
        pytest.param('хрум', True, id='podcast'),
    ])
    def test_what_year_is_this_song(self, alice, what, is_missing):
        r = alice(voice('включи ' + what))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('какого года песня'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.what_year_is_this_song'
        if is_missing:
            assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Не смогу сказать год этой записи.'
        else:
            assert re.match(r'С альбома.*года.', r.run_response.ResponseBody.Layout.OutputSpeech)

        return str(r)

    def test_not_playing(self, alice):
        r = alice(voice('какого года песня'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.what_year_is_this_song'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Как только вы включите музыку, я отвечу.'
        return str(r)


@pytest.mark.experiments(
    *EXPERIMENTS_RADIO,
    'hw_music_what_album_is_this_song_from',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestWhatAlbumIsThisSongFrom(TestsBase):

    @pytest.mark.parametrize('what', [
        pytest.param('кобзон', id='song'),
        pytest.param('чайковский', id='classics'),
        pytest.param('хрум', id='podcast'),
    ])
    def test_what_album_is_this_song_from(self, alice, what):
        r = alice(voice('включи ' + what))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('с какого альбома эта песня'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.what_album_is_this_song_from'
        assert re.match(r'С альбома .*.', r.run_response.ResponseBody.Layout.OutputSpeech)

        return str(r)

    def test_not_playing(self, alice):
        r = alice(voice('с какого альбома эта песня'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.what_album_is_this_song_from'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Как только вы включите музыку, я отвечу.'
        return str(r)


@pytest.mark.experiments(
    *EXPERIMENTS_RADIO,
    'hw_music_songs_by_this_artist',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestsSongsByThisArtist(TestsBase):

    PAYLOAD = {
        'typed_semantic_frame': {
            'player_songs_by_this_artist_semantic_frame': {
            }
        },
        'analytics': {
            'origin': 'Scenario',
            'purpose': 'play_music'
        }
    }

    def test_songs_by_this_artist(self, alice):
        r = alice(voice('включи du hast'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(server_action(name='@@mm_semantic_frame', payload=self.PAYLOAD))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        assert 'Включаю Rammstein' in r.continue_response.ResponseBody.Layout.OutputSpeech

        # TODO(ardulat, sparkle): check that song is not the same

        return str(r)

    @pytest.mark.parametrize('what', [
        pytest.param('подкаст хрум', id='podcast'),
        pytest.param('аудиокнигу дюна', id='audiobook'),
    ])
    def test_cant_play_this_artist(self, alice, what):
        r = alice(voice('включи ' + what))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective') or directives[0].HasField('MusicPlayDirective')

        r = alice(server_action(name='@@mm_semantic_frame', payload=self.PAYLOAD))
        assert r.scenario_stages() == {'run'}
        assert 'Не смогу включить песни этого исполнителя.' in r.run_response.ResponseBody.Layout.OutputSpeech

        return str(r)

    def test_not_playing(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload=self.PAYLOAD))
        assert r.scenario_stages() == {'run'}
        assert 'Как только вы включите музыку.' in r.run_response.ResponseBody.Layout.OutputSpeech

        return str(r)


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.oauth(auth.YandexPlusForeign)
class TestsMusicForeignSubscription:

    def test_unavailable(self, alice):
        '''
        У пользователя сильно заграничный (испанский) Я.Плюс, поэтому почти вся музыка
        ему недоступна. В ответ на запрос нужно отвечать про недоступность из-за региона
        '''
        r = alice(voice('включи rammstein'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.Layout.Directives
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Простите, мне не разрешают включать эту музыку здесь.',
            'Ой,sil <[300]> кажется, я в стране, где этот трек недоступен.',
            'С радостью бы послушала вместе с вами, правда в этой стране трек пока недоступен.',
            'Похоже, мы с вами в стране, где эту музыку пока нельзя послушать.',
            'Ой,sil <[300]> кажется, мне не хватает прав, чтобы включать эту музыку здесь.',
        ]


@pytest.mark.experiments(
    *EXPERIMENTS_RADIO,
    'hw_music_what_is_this_song_about',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestsWhatIsThisSongAbout(TestsBase):

    # TODO(ardulat): change to text, when the grammar will be released
    PAYLOAD = {
        'typed_semantic_frame': {
            'player_what_is_this_song_about_semantic_frame': {
            }
        },
        'analytics': {
            'origin': 'Scenario',
            'purpose': 'play_music'
        }
    }

    @pytest.mark.parametrize('what, is_missing', [
        pytest.param('кобзон', False, id='song'),
        pytest.param('чайковский', True, id='classics'),
        pytest.param('хрум', True, id='podcast'),
    ])
    def test_what_is_this_song_about(self, alice, what, is_missing):
        r = alice(voice('включи ' + what))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(server_action(name='@@mm_semantic_frame', payload=self.PAYLOAD))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.what_is_this_song_about'

        check_directive = has_entries({
            'response_body': has_entries({
                'ServerDirectives': contains(has_entries({
                    'SendPushDirective': has_entries({
                        'push_tag': 'alice_music_song_text'
                    }),
                })),
            }),
        })
        if is_missing:
            check_directive = is_not(check_directive)
        assert_that(r.run_response_pyobj, check_directive)

        check_action = has_entries({
            'response_body': has_entries({
                'analytics_info': has_entries({
                    'actions': contains(has_entries({
                        'id': 'what_is_this_song_about'
                    })),
                }),
            }),
        })
        if is_missing:
            check_action = is_not(check_action)
        assert_that(r.run_response_pyobj, check_action)

        return str(r)

    def test_not_playing(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload=self.PAYLOAD))
        assert r.scenario_stages() == {'run'}
        assert 'Как только вы включите музыку, я попробую ответить.' == r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)
