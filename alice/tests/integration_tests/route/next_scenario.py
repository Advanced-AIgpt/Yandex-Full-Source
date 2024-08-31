import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestNextScenarioAfterRouteSearchOnly(object):
    """
    Проверяем, что не циклимся на сценарии и следующие сценарии работают
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command, next_command', [
        ('поехали в Тулу', 'профессора качалова'),
    ])
    def test_search(self, alice, command, next_command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        response = alice(next_command)
        assert response.scenario == scenario.Search
        assert response.intent in {intent.Factoid, intent.ObjectAnswer}


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
    surface.navi,
    surface.automotive,
])
class TestNextScenarioAfterRoute(object):
    """
    Проверяем, что не циклимся на сценарии и следующие сценарии работают
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command, next_command', [
        ('поехали в Тулу', 'Собор Василия Блаженного'),
    ])
    def test_find_poi(self, alice, command, next_command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        response = alice(next_command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.FindPoi

    @pytest.mark.parametrize('command, next_command', [
        ('санкт петербург город кудрово проложить маршрут', 'в питер что ты ты знаешь как я городок'),
        ('сначала обещай а потом гавайи', 'я на самоваре чей'),
    ])
    def test_general_conversation(self, alice, command, next_command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        response = alice(next_command)
        assert response.scenario in {scenario.GeneralConversation, scenario.Search}


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [
    surface.navi, surface.automotive,
])
class TestNextScenarioAfterRouteNavi(object):
    """
    Проверяем, что не циклимся на сценарии и следующие сценарии работают
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command, next_command', [
        ('поехали в Тулу', 'профессора качалова'),
    ])
    def test_general_conversation(self, alice, command, next_command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        response = alice(next_command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.FindPoi
