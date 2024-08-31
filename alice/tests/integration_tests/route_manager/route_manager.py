import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


LAYOUT_UNKNOWN = 0
LAYOUT_MAP = 1
LAYOUT_RIDE = 2

ROUTE_UNKNOWN = 0
ROUTE_STOPPED = 1
ROUTE_STOPPING = 2
ROUTE_MOVING = 3
ROUTE_WAITING_PASSENGER = 4
ROUTE_FINISHED = 5


@pytest.mark.parametrize('surface', [surface.sdc])
@pytest.mark.experiments(
    'mm_enable_protocol_scenario=RouteManager',
    'enable_outgoing_operator_calls',
    'hw_route_manager_handle_state',
)
@pytest.mark.voice
class TestRouteManager(object):
    owners = ('sdll')

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_WAITING_PASSENGER,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('начать поездку'),
        pytest.param('поехали')
    ])
    def test_start(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.RouteManagerStartDirective
        assert response.text == 'Начинаю поездку.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_MOVING,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('остановить поездку'),
        pytest.param('припаркуйся')
    ])
    def test_stop(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.RouteManagerStopDirective
        assert response.text == 'Останавливаю поездку.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_MOVING,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('показать маршрут'),
        pytest.param('покажи карту')
    ])
    def test_show_route(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.RouteManagerShowDirective
        assert response.text == 'Ваш маршрут на экране.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_STOPPING,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('продолжить поездку'),
        pytest.param('поехали дальше')
    ])
    def test_continue_while_stopping(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.RouteManagerContinueDirective
        assert response.text == 'Продолжаю поездку.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_STOPPED,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('продолжить поездку'),
        pytest.param('поехали дальше')
    ])
    def test_continue_when_stopped(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.RouteManagerContinueDirective
        assert response.text == 'Продолжаю поездку.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_MOVING,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('продолжить поездку'),
        pytest.param('поехали дальше')
    ])
    def test_cant_continue(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.RouteManager
        assert not response.directives
        assert response.text == 'В данный момент не могу продолжить поездку.'

    @pytest.mark.device_state(route_manager_state={
        'route': ROUTE_MOVING,
        'layout': LAYOUT_RIDE
    })
    @pytest.mark.parametrize('command', [
        pytest.param('позвонить оператору'),
        pytest.param('набери службу поддержки такси')
    ])
    def test_operator_call(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.MessengerCallDirective
