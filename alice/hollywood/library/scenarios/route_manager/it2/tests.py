import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


SUGGEST_ICON_START = "https://static-alice.s3.yandex.net/scenarios/route_manager/start_ride.png"
SUGGEST_ICON_STOP = "https://static-alice.s3.yandex.net/scenarios/route_manager/stop_ride.png"
SUGGEST_ICON_SHOW_RIDE = "https://static-alice.s3.yandex.net/scenarios/route_manager/show_ride.png"
SUGGEST_ICON_SHOW_ROUTE = "https://static-alice.s3.yandex.net/scenarios/route_manager/show_route.png"
SUGGEST_ICON_CALL_SUPPORT = "https://static-alice.s3.yandex.net/scenarios/route_manager/call_support.png"

LAYOUT_UNKNOWN = 0
LAYOUT_MAP = 1
LAYOUT_RIDE = 2

ROUTE_UNKNOWN = 0
ROUTE_STOPPED = 1
ROUTE_STOPPING = 2
ROUTE_MOVING = 3
ROUTE_WAITING_PASSENGER = 4


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['route_manager']


def _check_suggests(layout, expected_suggests):
    actual_suggests = set()
    for suggest_button in layout.SuggestButtons:
        actual_suggests.add((suggest_button.ActionButton.Title, suggest_button.ActionButton.Theme.ImageUrl))
    assert actual_suggests == set(expected_suggests)


@pytest.mark.scenario(name='RouteManager', handle='route_manager')
@pytest.mark.experiments(*['hw_route_manager_handle_state'])
@pytest.mark.parametrize('surface', [surface.sdc])
class Tests:

    @pytest.mark.parametrize('command', [
        pytest.param('начать поездку', id='start'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_WAITING_PASSENGER,
        "layout": LAYOUT_RIDE
    })
    def test_start(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Начинаю поездку.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Остановить машину', SUGGEST_ICON_STOP),
            ('Показать маршрут', SUGGEST_ICON_SHOW_ROUTE),
        ])
        assert len(layout.Directives) == 1
        assert layout.Directives[0].RouteManagerStartDirective

    @pytest.mark.parametrize('command', [
        pytest.param('начать поездку', id='start'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_MOVING,
        "layout": LAYOUT_MAP
    })
    def test_start_while_moving(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'В данный момент не могу начать поездку.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Остановить машину', SUGGEST_ICON_STOP),
            ('На экран поездки', SUGGEST_ICON_SHOW_RIDE),
        ])
        assert len(layout.Directives) == 0

    @pytest.mark.parametrize('command', [
        pytest.param('остановить поездку', id='stop'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_MOVING,
        "layout": LAYOUT_RIDE
    })
    def test_stop(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Останавливаю поездку.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Продолжить поездку', SUGGEST_ICON_START),
            ('Показать маршрут', SUGGEST_ICON_SHOW_ROUTE),
        ])
        assert len(layout.Directives) == 1
        assert layout.Directives[0].RouteManagerStopDirective

    @pytest.mark.parametrize('command', [
        pytest.param('продолжи поездку', id='continue'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_STOPPING,
        "layout": LAYOUT_RIDE
    })
    def test_continue(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Продолжаю поездку.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Остановить машину', SUGGEST_ICON_STOP),
            ('Показать маршрут', SUGGEST_ICON_SHOW_ROUTE),
        ])
        assert len(layout.Directives) == 1
        assert layout.Directives[0].RouteManagerContinueDirective

    @pytest.mark.parametrize('command', [
        pytest.param('покажи маршрут', id='show'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_MOVING,
        "layout": LAYOUT_RIDE
    })
    def test_show_route(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Ваш маршрут на экране.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Остановить машину', SUGGEST_ICON_STOP),
            ('На экран поездки', SUGGEST_ICON_SHOW_RIDE),
        ])
        assert len(layout.Directives) == 1
        assert layout.Directives[0].RouteManagerShowDirective

    @pytest.mark.parametrize('command', [
        pytest.param('покажи маршрут', id='show'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_STOPPING,
        "layout": LAYOUT_MAP
    })
    def test_show_route_already_shown(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Ваш маршрут уже на экране.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Продолжить поездку', SUGGEST_ICON_START),
            ('На экран поездки', SUGGEST_ICON_SHOW_RIDE),
        ])
        assert len(layout.Directives) == 0

    @pytest.mark.parametrize('command', [
        pytest.param('покажи маршрут', id='show'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_STOPPING,
        "layout": LAYOUT_UNKNOWN
    })
    def test_show_route_unknown_layout(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Ваш маршрут на экране.'
        assert len(layout.SuggestButtons) == 3
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
            ('Продолжить поездку', SUGGEST_ICON_START),
            ('На экран поездки', SUGGEST_ICON_SHOW_RIDE),
        ])
        assert len(layout.Directives) == 1

    @pytest.mark.parametrize('command', [
        pytest.param('покажи маршрут', id='error'),
    ])
    @pytest.mark.device_state(route_manager_state={
        "route": ROUTE_UNKNOWN,
        "layout": LAYOUT_MAP
    })
    def test_unknown_route(self, alice, command):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Произошла какая-то ошибка. Попробуйте ещё раз или позвоните оператору.'
        assert len(layout.SuggestButtons) == 1
        _check_suggests(layout, [
            ('Позвонить в поддержку', SUGGEST_ICON_CALL_SUPPORT),
        ])
        assert len(layout.Directives) == 0
