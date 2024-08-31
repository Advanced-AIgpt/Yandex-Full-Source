import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestShowTimer(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    commands = ['покажи все таймеры']

    def test_without_timer(self, alice):
        response = alice('покажи все таймеры')
        assert response.intent == intent.TimerShow
        assert response.text == 'Я ничего не засекала' or response.text.startswith('У вас нет установленных')

    def test_with_one_timer(self, alice):
        alice('поставь таймер на 7 минут')

        response = alice('покажи все таймеры')
        assert response.intent == intent.TimerShow
        assert response.text.startswith('На данный момент запущен таймер на 7 минут')

    def test_with_two_timers(self, alice):
        alice('поставь таймер на 7 минут')
        alice('поставь таймер на 1 час')

        response = alice('покажи все таймеры')
        assert response.intent == intent.TimerShow
        assert response.text.startswith('На данный момент запущены следующие таймеры')


@pytest.mark.parametrize('surface', [surface.station])
class TestSetTimer(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    commands = ['поставь таймер', 'включи таймер']
    expected_directive = directives.names.SetTimerDirective

    @pytest.mark.parametrize('command', commands)
    def test_one_timer(self, alice, command):
        response = alice(command)
        assert response.intent == intent.TimerSet
        assert response.text == 'На какое время поставить таймер?'
        assert not response.directive

        response = alice('на 7 минут')
        assert response.directive.name == self.expected_directive

    @pytest.mark.parametrize('command', commands)
    def test_seven_minutes_timer(self, alice, command):
        response = alice(command + ' на 7 минут')
        assert response.directive.name == self.expected_directive

    @pytest.mark.parametrize('command', commands)
    def test_more_timers(self, alice, command):
        for i in range(10):
            response = alice(command + ' на 7 минут')
            assert response.directive.name == self.expected_directive
            assert len(alice.device_state.Timers.ActiveTimers) == i + 1
            alice.skip(seconds=10)


@pytest.mark.parametrize('surface', [surface.station])
class TestCancelTimer(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    commands = ['удали таймер', 'выключи таймер']
    expected_directive = directives.names.CancelTimerDirective

    @pytest.mark.parametrize('command', commands)
    def test_without_timer(self, alice, command):
        response = alice(command)
        assert response.intent == intent.TimerCancel
        assert response.text == 'Я ничего не засекала' or response.text.startswith('У вас нет установленных')
        assert not response.directive

    @pytest.mark.parametrize('command', commands)
    def test_with_one_timer(self, alice, command):
        alice('поставь таймер на 1 час')
        response = alice(command)
        assert response.directive.name == self.expected_directive

    @pytest.mark.parametrize('command', commands)
    def test_with_two_timers(self, alice, command):
        alice('поставь таймер на 1 час')
        hour_timer = alice.device_state.Timers.ActiveTimers[0]

        alice.skip(minutes=5)
        alice('поставь таймер на 30 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 2

        response = alice(command)
        assert response.intent == intent.TimerCancel
        assert response.text.startswith('На данный момент запущены следующие таймеры') and \
            response.text.endswith('Какой из них вы хотите удалить?')

        response = alice('первый')
        assert response.directive.name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 1
        assert hour_timer.TimerId == alice.device_state.Timers.ActiveTimers[0].TimerId

    @pytest.mark.parametrize('command', commands)
    def test_cancel_hour_timer(self, alice, command):
        alice('поставь таймер на 1 час')
        hour_timer = alice.device_state.Timers.ActiveTimers[0]

        alice.skip(minutes=5)
        alice('поставь таймер на 30 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 2

        response = alice(command + ' на 1 час')
        assert response.directive.name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 1
        assert hour_timer.TimerId != alice.device_state.Timers.ActiveTimers[0].TimerId

    @pytest.mark.parametrize('command', commands)
    def test_calcel_two_hour_timers(self, alice, command):
        alice('поставь таймер на 1 час')
        hour_timer = alice.device_state.Timers.ActiveTimers[0]

        alice.skip(minutes=5)
        alice('поставь таймер на 1 час')
        assert len(alice.device_state.Timers.ActiveTimers) == 2

        response = alice(command + ' на 1 час')
        assert response.intent == intent.TimerCancel
        assert response.text.startswith('На данный момент запущены следующие таймеры') and \
            response.text.endswith('Какой из них вы хотите удалить?')

        response = alice('второй')
        assert response.directive.name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 1
        assert hour_timer.TimerId == alice.device_state.Timers.ActiveTimers[0].TimerId

    @pytest.mark.parametrize('command', commands)
    def test_cancel_all_timers(self, alice, command):
        alice('поставь таймер на 1 час')
        alice.skip(minutes=5)
        alice('поставь таймер на 30 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 2

        response = alice('удали все таймеры')
        assert len(response.directives) >= 2
        assert response.directives[0].name == self.expected_directive
        assert response.directives[1].name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 0


@pytest.mark.parametrize('surface', [surface.station])
class TestPauseTimer(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    commands = ['поставь на паузу таймер', 'останови таймер']
    expected_directive = directives.names.PauseTimerDirective

    @pytest.mark.parametrize('command', commands)
    def test_with_one_timer(self, alice, command):
        alice('поставь таймер на 5 минут')
        response = alice(command)
        assert response.directive.name == self.expected_directive


@pytest.mark.parametrize('surface', [surface.station])
class TestStopPlayingTimer(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    commands = ['выключи таймер', 'хватит', 'стоп']
    expected_directive = directives.names.TimerStopPlayingDirective

    @pytest.mark.parametrize('command', commands)
    def test_with_one_timer(self, alice, command):
        alice('поставь таймер на 5 минут')
        alice.skip(minutes=5)
        response = alice(command)
        assert response.directive.name == self.expected_directive

    @pytest.mark.parametrize('command', commands)
    def test_with_two_timers(self, alice, command):
        alice('поставь таймер на 5 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 1
        timer_id = alice.device_state.Timers.ActiveTimers[0].TimerId
        alice.skip(minutes=2)

        alice('поставь таймер на 5 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 2
        alice.skip(minutes=3)

        response = alice(command)
        assert response.directive.name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 1
        assert timer_id != alice.device_state.Timers.ActiveTimers[0].TimerId

    @pytest.mark.parametrize('command', commands)
    def test_with_two_timers_off(self, alice, command):
        alice('поставь таймер на 5 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 1

        alice('поставь таймер на 5 минут')
        assert len(alice.device_state.Timers.ActiveTimers) == 2
        alice.skip(minutes=6)

        response = alice(command)
        assert len(response.directives) >= 2
        assert response.directives[0].name == self.expected_directive
        assert response.directives[1].name == self.expected_directive
        assert len(alice.device_state.Timers.ActiveTimers) == 0
