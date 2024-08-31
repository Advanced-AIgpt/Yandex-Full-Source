import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


CHILD_RADIO_ID = 'detskoe'
MAXIMUM_RADIO_ID = 'maximum'

CHILD_RADIO_ANSWER_REGEX = '.*(Д|д)етское радио'
MAXIMUM_RADIO_ANSWER_REGEX = '.*(Р|р)адио "Максимум"'
SHORT_RADIO_INTRO_REGEX = r'Секунду\.'


def _assert_radio_station(response, device_state, text_regex, radio_id):
    assert response.intent == intent.RadioPlay
    assert re.match(text_regex, response.text)
    assert response.directive.name == directives.names.RadioPlayDirective
    assert device_state.Radio['radioId'] == radio_id


def get_radio_player_paused(device_state):
    assert 'player' in device_state.Radio.fields
    assert 'paused' in device_state.Radio['player'].fields
    return device_state.Radio['player']['paused']


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestRadio(object):

    owners = ('karina-usm',)

    def test_radio_named_station(self, alice):
        response = alice('включи радио максимум')
        _assert_radio_station(response, alice.device_state, MAXIMUM_RADIO_ANSWER_REGEX, MAXIMUM_RADIO_ID)

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
    @pytest.mark.parametrize('command', ['включи радио', 'включи детское радио'])
    def test_radio_safe_child_settings(self, alice, command):
        response = alice(command)
        _assert_radio_station(response, alice.device_state, CHILD_RADIO_ANSWER_REGEX, CHILD_RADIO_ID)

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
    def test_radio_forbidden_station(self, alice):
        response = alice('включи радио максимум')
        assert response.intent == intent.RadioPlay
        assert response.text == 'Лучше слушать эту станцию вместе с родителями.'
        assert not response.directive

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
    def test_radio_next_station_child(self, alice):
        response = alice('включи детское радио')
        _assert_radio_station(response, alice.device_state, CHILD_RADIO_ANSWER_REGEX, CHILD_RADIO_ID)

        response = alice('дальше')
        _assert_radio_station(
            response,
            alice.device_state,
            f'{SHORT_RADIO_INTRO_REGEX}|{CHILD_RADIO_ANSWER_REGEX}',
            CHILD_RADIO_ID,
        )


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestRadioShuffleCommand(object):

    owners = ('nkodosov',)

    def test_shuffle(self, alice):
        response = alice('включи радио')
        response = alice('перемешай')
        assert not response.directive
        assert response.text == 'Пока я умею такое только в Яндекс.Музыке.'


@pytest.mark.experiments(
    'enable_shuffle_in_hw_music',
)
class TestRadioShuffleCommandExp(TestRadioShuffleCommand):
    pass


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestRadioPrevNextCommands(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1535
    """

    owners = ('nkodosov',)

    def test_alice_1535(self, alice):
        response = alice('включи радио')
        radio_ids = []

        assert response.directive.name == directives.names.RadioPlayDirective
        radio_ids.append(response.directive.payload.radioId)

        response = alice('Следующая станция')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        radio_ids.append(response.directive.payload.radioId)

        response = alice('Дальше')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        radio_ids.append(response.directive.payload.radioId)

        response = alice('Перемотай вперёд')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        radio_ids.append(response.directive.payload.radioId)

        response = alice('Давай дальше')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        radio_ids.append(response.directive.payload.radioId)

        response = alice('Предыдущая станция')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        assert response.directive.payload.radioId == radio_ids[-2]

        response = alice('Предыдущая')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        assert response.directive.payload.radioId == radio_ids[-3]

        response = alice('Перемотай назад')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        assert response.directive.payload.radioId == radio_ids[-4]

        response = alice('Давай предыдущий')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.text == 'Секунду.'
        assert response.directive.payload.radioId == radio_ids[-5]


@pytest.mark.experiments(
    'enable_shuffle_in_hw_music',
)
class TestRadioPrevNextCommandsExp(TestRadioPrevNextCommands):
    pass


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
class TestRadioCommandsTvUnplugged(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1695
    """

    owners = ('nkodosov',)

    def check_response_text(self, response):
        assert response.text in [
            f'Окей! "Радио {response.directive.payload.title}".',
            f'Хорошо! "Радио {response.directive.payload.title}".',
            f'Включаю "Радио {response.directive.payload.title}".',

            f'Окей! Радио "{response.directive.payload.title}".',
            f'Хорошо! Радио "{response.directive.payload.title}".',
            f'Включаю Радио "{response.directive.payload.title}".',

            f'Окей! "{response.directive.payload.title}".',
            f'Хорошо! "{response.directive.payload.title}".',
            f'Включаю "{response.directive.payload.title}".',

            f'Окей! радио "{response.directive.payload.title}".',
            f'Хорошо! радио "{response.directive.payload.title}".',
            f'Включаю радио "{response.directive.payload.title}".',
        ]

    def test_alice_1695(self, alice):
        response = alice('включи радио')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)

        response = alice('стоп')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert get_radio_player_paused(alice.device_state)

        response = alice('играй')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)
        self.check_response_text(response)

        last_radio_id = response.directive.payload.radioId

        response = alice('Следующее')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert response.directive.payload.radioId != last_radio_id
        self.check_response_text(response)

        response = alice('Предыдущее')
        assert response.directive.name == directives.names.RadioPlayDirective
        self.check_response_text(response)
        assert response.directive.payload.radioId == last_radio_id


@pytest.mark.experiments(
    'enable_shuffle_in_hw_music',
)
class TestRadioCommandsTvUnpluggedExp(TestRadioCommandsTvUnplugged):
    pass


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.loudspeaker(is_tv_plugged_in=True),
    surface.station,
])
class TestRadioCommandsTvPlugged(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1536
    """

    owners = ('nkodosov',)

    def test_alice_1536(self, alice):
        response = alice('включи радио')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)

        response = alice('стоп')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert get_radio_player_paused(alice.device_state)

        response = alice('играй')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)
        assert response.text == 'Секунду.'

        response = alice('пауза')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert get_radio_player_paused(alice.device_state)

        response = alice('включи')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)
        assert response.text == 'Секунду.'

        response = alice('выключи')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert get_radio_player_paused(alice.device_state)

        response = alice('запусти')
        assert response.directive.name == directives.names.RadioPlayDirective
        assert not get_radio_player_paused(alice.device_state)
        assert response.text == 'Секунду.'


@pytest.mark.experiments(
    'enable_shuffle_in_hw_music',
)
class TestRadioCommandsTvPluggedExp(TestRadioCommandsTvPlugged):
    pass
