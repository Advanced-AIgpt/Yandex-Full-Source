import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import server_action, voice, Scenario
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture, HttpResponseStub
from alice.hollywood.library.python.testing.it2.hamcrest import has_only_entries
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    get_callback,
    prepare_server_action_data,
)
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TContentId
from hamcrest import assert_that, has_entries, empty, is_not, contains
from conftest import get_scenario_state


logger = logging.getLogger(__name__)
bass_stubber = create_localhost_bass_stubber_fixture()


EXPERIMENTS_FM_RADIO = [
    'bg_fresh_granet',
    'hw_music_thin_client',
    'hw_music_thin_client_fm_radio',
    'radio_play_in_quasar',
]


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS_FM_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsBase:
    pass


@pytest.mark.parametrize('', [
    pytest.param(id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
    pytest.param(id='plus', marks=pytest.mark.oauth(auth.YandexPlus)),
])
class TestsFmRadio(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('включи русское радио', id='russian_radio'),
        pytest.param('включи радио 105.7', id='russian_radio_freq'),
    ])
    def test_play_fm_radio(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'
        assert re.match(r'.*Русское радио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        return str(r)

    def test_play_fm_radio_general(self, alice):
        r = alice(voice('включи радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*(Включаю|Окей|Хорошо).*(р|Р)адио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id
        return str(r)

    @pytest.mark.device_state(radio={
        'currently_playing': {
            'radioId': 'rusradio',
            'radioTitle': 'Русское Радио'
        }
    })
    def test_play_fm_radio_general_with_currently_playing_device_state(self, alice):
        r = alice(voice('включи радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*(Включаю|Окей|Хорошо).*(р|Р)адио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

    def test_play_fm_radio_general_with_currently_playing_scenario_state(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Русское радио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

        r = alice(voice('включи радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Русское радио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/radio-stream/ranked/list': [
            HttpResponseStub(200, 'freeze_stubs/fm_radio_chanson_inactive.json'),
        ],
    })
    def test_play_fm_radio_inactive(self, alice):
        '''
        Просим включить "радио шансон", оно НЕАКТИВНО (поле active = false, см. стаб)
        Включаем другое доступное радио
        '''
        r = alice(voice('включи радио шансон'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Я ещё не.*(волну|станцию).*(М|м)огу (предложить|включить) вам.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id
        return str(r)

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/radio-stream/ranked/list': [
            HttpResponseStub(200, 'freeze_stubs/fm_radio_chanson_unavailable.json'),
        ],
    })
    def test_play_fm_radio_unavailable(self, alice):
        '''
        Просим включить "радио шансон", оно НЕДОСТУПНО (поле available = false, см. стаб)
        Включаем другое доступное радио
        '''
        r = alice(voice('включи радио шансон'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Эта станция временно недоступна.*(М|м)огу (предложить|включить) вам.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id
        return str(r)

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/radio-stream/ranked/list': [
            HttpResponseStub(200, 'freeze_stubs/fm_radio_chanson_missing.json'),
        ],
    })
    def test_play_fm_radio_missing(self, alice):
        '''
        Просим включить "радио шансон", но его просто нет в списке радио (см. стаб)
        Ведем так, как будто радио НЕАКТИВНО и включаем другое доступное радио
        '''
        r = alice(voice('включи радио шансон'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Я ещё не.*(волну|станцию).*(М|м)огу (предложить|включить) вам.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id
        return str(r)

    # TODO(mike88): Add unrecognized test

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_play_fm_radio_child_restricted(self, alice):
        r = alice(voice('включи радио европа плюс'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == 'Лучше слушать эту станцию вместе с родителями.'
        assert not r.continue_response.ResponseBody.Layout.Directives
        return str(r)

    @pytest.mark.experiments('yamusic_audiobranding_score=0')
    def test_play_radio_after_fm_radio(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'
        assert re.match(r'.*Русское радио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)

        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        if layout.OutputSpeech.startswith('Включаю'):
            assert_audio_play_directive(layout.Directives)
        else:
            assert layout.OutputSpeech in [
                'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
                'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
                'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
            ]
        return str(r)


@pytest.mark.no_oauth
class TestsFmRadioUnauthorized(TestsBase):

    def test_play_fm_radio_unauthorized(self, alice):
        r = alice(voice('включи радио'))
        assert r.scenario_stages() == {'run'}
        assert re.match(r'.*(Кажется, вы не авторизованы|Чтобы слушать радио).*ойдите в свой аккаунт на Яндексе.*', r.run_response.ResponseBody.Layout.OutputSpeech)
        assert not r.run_response.ResponseBody.Layout.Directives
        return str(r)


@pytest.mark.parametrize('', [
    pytest.param(id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
    pytest.param(id='plus', marks=pytest.mark.oauth(auth.YandexPlus)),
])
class TestsFmRadioPlayerCommand(TestsBase):

    def test_play_fm_radio_continue(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert re.match(r'.*Русское радио.*', r.continue_response.ResponseBody.Layout.OutputSpeech)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}

        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

    def test_play_fm_radio_what_is_playing(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)

        r = alice(voice('что сейчас играет'))
        assert r.scenario_stages() == {'run'}
        assert re.match(r'.*Сейчас играет \"Русское радио\".*', r.run_response.ResponseBody.Layout.OutputSpeech)
        assert not r.run_response.ResponseBody.Layout.Directives

        return str(r)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_play_fm_radio_next_station_in_list(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

        r = alice(voice('следующее'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        # the next radio is alphabetically bigger
        # example: 'silver_rain' > 'rusradio' or 'sevastopol_fm' > 'rusradio'
        assert state.Queue.PlaybackContext.ContentId.Id > 'rusradio'

        return str(r)

    def test_play_fm_radio_next_prev_track(self, alice):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)

        r = alice(voice('включи радио маяк'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'rusradio'

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'mayak'

    @pytest.mark.parametrize('command, text_re', [
        pytest.param('перемотай назад на 30 секунд', r'.*прямой эфир.*', id='rewind_backward'),
        pytest.param('поставь на повтор', r'.*прямой эфир.*', id='repeat'),
        pytest.param('включи заново', r'.*прямой эфир.*', id='replay'),
        pytest.param('перемешай музыку', r'Пока я умею такое только в Яндекс.Музыке.', id='shuffle'),
        pytest.param('поставь лайк', r'Пока я умею такое только в Яндекс.Музыке.', id='like'),
        pytest.param('поставь дизлайк', r'Пока я умею такое только в Яндекс.Музыке.', id='dislike'),
    ])
    def test_play_fm_radio_unsupported_commands(self, alice, command, text_re):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}
        layout = r.continue_response.ResponseBody.Layout
        assert_audio_play_directive(layout.Directives, is_hls=True)

        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert re.match(text_re, layout.OutputSpeech)
        return str(r)  # XXX(sparkle): added for HOLLYWOOD-948


@pytest.mark.oauth(auth.YandexPlus)
class TestsFmRadioPlayerCommandWithOndemand(TestsBase):

    def test_play_fm_radio_next_prev_track_with_ondemand(self, alice):
        r = alice(voice('включи клава кока'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('включи радио маяк'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)

        r = alice(voice('включи песню краш'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'mayak'

        r = alice(voice('предыдущий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type != TContentId.FmRadio

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives, is_hls=True)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.FmRadio
        assert state.Queue.PlaybackContext.ContentId.Id == 'mayak'

        r = alice(voice('следующий трек'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert_audio_play_directive(r.continue_response.ResponseBody.Layout.Directives)
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Track
        assert state.Queue.PlaybackContext.ContentId.Id == '66869588'


@pytest.mark.parametrize('', [
    pytest.param(id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
    pytest.param(id='plus', marks=pytest.mark.oauth(auth.YandexPlus)),
])
class TestsFmRadioCallbacks(TestsBase):

    @pytest.mark.parametrize('callback_name, short_name', [
        pytest.param('music_thin_client_on_started', 'on_started', id='on_started'),
        pytest.param('music_thin_client_on_stopped', 'on_stopped', id='on_stopped'),
        pytest.param('music_thin_client_on_failed', 'on_failed', id='on_failed'),
    ])
    def test_lifecycle_callback(self, alice, callback_name, short_name):
        r = alice(voice('включи русское радио'))
        assert r.scenario_stages() == {'run', 'continue'}

        callbacks = r.continue_response_pyobj['response']['layout']['directives'][0]['audio_play_directive']['callbacks']
        assert_that(callbacks, has_entries({
            short_name: has_only_entries({
                'name': callback_name,
                'ignore_answer': True,
                'payload': has_only_entries({
                    'events': contains(has_only_entries({
                        'playAudioEvent': has_only_entries({
                            'uid': is_not(empty()),
                            'from': 'alice-on_demand-fm_radio-fm_radio',
                            'trackId': 'rusradio',
                            'playId': is_not(empty()),
                            'context': 'fm_radio',
                            'contextItem': 'rusradio',
                        })
                    }))
                })
            })
        }))

        directives = r.continue_response.ResponseBody.Layout.Directives
        audio_play = assert_audio_play_directive(directives, is_hls=True)
        server_action_data = prepare_server_action_data(get_callback(audio_play.Callbacks, callback_name))
        r = alice(server_action(server_action_data['name'], server_action_data['payload']))
        assert r.scenario_stages() == {'run', 'commit'}
