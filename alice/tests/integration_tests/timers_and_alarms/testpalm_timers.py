import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


class TestPalmTimers(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-34
    https://testpalm.yandex-team.ru/testcase/alice-419
    https://testpalm.yandex-team.ru/testcase/alice-1082
    """

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_alice_34(self, alice):
        response = alice('Поставь таймер на пять минут')

        assert response.text == 'Я скоро научусь это делать. Время пролетит незаметно.'
        assert response.intent == intent.TimerSet
        assert not response.directive

    @pytest.mark.parametrize('surface', [surface.station])
    def test_alice_419(self, alice):
        response = alice('включи таймер')
        assert response.text == 'На какое время поставить таймер?'

        alice('поставь таймер на 1 минуту')
        assert alice.device_state.Timers.ActiveTimers
        timer_id = alice.device_state.Timers.ActiveTimers[0].TimerId

        alice.skip(minutes=1)
        response = alice('выключи таймер')
        assert timer_id == response.directive.payload.timer_id
        assert not alice.device_state.Timers.ActiveTimers

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_alice_1082(self, alice):
        response = alice('Поставь таймер на 20 секунд')
        assert '20 секунд' in response.text

        alice.skip(seconds=20)

        response = alice('Засеки 10 минут')
        assert '10 минут' in response.text

        response = alice('Покажи список таймеров')
        assert response.text.endswith('список таймеров.') or \
            response.text.endswith('ваши таймеры.')

    @pytest.mark.parametrize('surface', [
        surface.smart_tv,
        surface.yabro_win,
    ])
    def test_timer_unsupported(self, alice):
        response = alice('Поставь таймер на 20 секунд')

        assert 'не умею' in response.text
        assert response.intent == intent.TimerSet
        assert not response.directive
