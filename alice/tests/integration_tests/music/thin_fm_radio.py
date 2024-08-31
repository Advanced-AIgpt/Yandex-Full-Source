import alice.megamind.protos.common.frame_pb2 as frame_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from music.thin_client import assert_audio_play_directive, assert_response


EMusicPlayType = frame_pb2.TMusicPlayObjectTypeSlot.EValue


EXPERIMENTS_FM_RADIO = [
    'bg_fresh_granet',
    'hw_music_thin_client',
    'hw_music_thin_client_fm_radio',
    'radio_play_in_quasar',
    'mm_fm_radio_prefer_hw_music_over_vins_on_smart_speakers',
    'mm_add_preclassifier_confident_frame_HollywoodMusic=alice.music.fm_radio_play',
]


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(*EXPERIMENTS_FM_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsThinFmRadio(object):

    owners = ('amullanurov', 'mike88',)

    def test_play_fm_radio_general(self, alice):
        response = alice('включи радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)

    @pytest.mark.device_state(radio={
        'currently_playing': {
            'radioId': 'rusradio',
            'radioTitle': 'Русское Радио'
        }
    })
    def test_play_fm_radio_general_with_currently_playing_device_state(self, alice):
        response = alice('включи радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

    def test_play_fm_radio_general_with_currently_playing_scenario_state(self, alice):
        response = alice('включи русское радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

        response = alice('включи радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

    @pytest.mark.parametrize('command', [
        pytest.param('включи русское радио', id='russian_radio'),
        pytest.param('включи радио 105.7', id='russian_radio_freq'),
    ])
    def test_play_fm_radio(self, alice, command):
        response = alice(command)
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

    def test_play_fm_radio_unavailable(self, alice):
        response = alice('включи радио пилот секрет')
        assert_response(response, text_re=r'Я ещё не (настроилась|поймала).*(М|м)огу (включить|предложить).*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id != 'pilot_secret'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(*EXPERIMENTS_FM_RADIO)
@pytest.mark.parametrize('surface', [surface.station])
class TestsThinFmRadioPlayerCommands(object):

    owners = ('amullanurov', 'mike88',)

    def test_play_fm_radio_continue(self, alice):
        response = alice('включи русское радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)

        response = alice('продолжи')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

    def test_play_fm_radio_what_is_playing(self, alice):
        response = alice('включи русское радио')
        assert_audio_play_directive(response.directive)

        response = alice('что сейчас играет')
        assert_response(response, text_re=r'Сейчас играет "Русское радио".', scenario=scenario.HollywoodMusic)
        assert not response.directive

    def test_play_fm_radio_next_station_in_list(self, alice):
        response = alice('включи русское радио')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Русское радио.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

        response = alice('следующий')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'silver_rain'

    def test_play_fm_radio_next_prev_track(self, alice):
        response = alice('включи русское радио')
        assert_audio_play_directive(response.directive)

        response = alice('включи радио маяк')
        assert_audio_play_directive(response.directive)

        response = alice('предыдущий')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'rusradio'

        response = alice('следующий')
        assert_response(response, text_re=r'(Включаю|Окей|Хорошо).*Маяк.*', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.type == EMusicPlayType.Name(EMusicPlayType.FmRadio)
        assert music_metadata.id == 'mayak'

    @pytest.mark.parametrize('command, text_re', [
        pytest.param('перемотай назад на 30 секунд', r'.*прямой эфир.*', id='rewind_backward'),
        pytest.param('поставь на повтор', r'.*прямой эфир.*', id='repeat'),
        pytest.param('включи заново', r'.*прямой эфир.*', id='replay'),
        pytest.param('перемешай музыку', r'Пока я умею такое только в Яндекс.Музыке.', id='shuffle'),
        pytest.param('поставь лайк', r'Пока я умею такое только в Яндекс.Музыке.', id='like'),
        pytest.param('поставь дизлайк', r'Пока я умею такое только в Яндекс.Музыке.', id='dislike'),
    ])
    def test_play_fm_radio_unsupported_commands(self, alice, command, text_re):
        response = alice('включи русское радио')
        assert_audio_play_directive(response.directive)

        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert not response.directive


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestsFmRadioSmartTv(object):

    owners = ('amullanurov', 'mike88',)

    @pytest.mark.parametrize('', [
        pytest.param(id='hw_radio', marks=pytest.mark.experiments(*EXPERIMENTS_FM_RADIO)),
        pytest.param(id='old_radio')
    ])
    def test_play_fm_radio_general(self, alice):
        response = alice('включи радио')
        assert_response(response, text_re=r'Включаю.', scenario=scenario.Vins)
        assert response.directive.name == directives.names.MusicPlayDirective
