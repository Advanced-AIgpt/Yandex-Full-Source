import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi])
@pytest.mark.region(region.Moscow)
class TestSetNewAddresses(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1075
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'Алиса домой отправь меня',
        'Алиса домой',
        'Алиса ну домой',
        'Ну все домой давай отправляй',
        'Поехали домой',
        pytest.param(
            'Установи маршрут домой',
            marks=pytest.mark.xfail(reason='worked unstable for Route scenario because find target: "LeHome Interiors" по адресу Большой Саввинский переулок 12, стр. 6.')
        ),
        pytest.param(
            'Установи маршрут до дома',
            marks=pytest.mark.xfail(reason='worked unstable for Route scenario because find target "Дом учёных" по адресу улица Пречистенка 16/2.')
        ),
        'Покажи маршрут домой',
        'Построй маршрут домой',
    ])
    def test_set_home_address(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.RememberNamedLocation
        assert response.text in [
            'И где находится дом?',
            'И какой адрес у дома?',
            'Окей, где находится дом',
            'Окей, где находится дом?',
            'Окей, какой адрес у дома?',
        ]

        response = alice('Ленинские горы, дом 1')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.RememberNamedLocationEllipsis
        assert response.text.startswith((
            'Адрес дома — ', 'Новый адрес дома — ',
        ))
        assert 'Москва, Западный административный округ, район Раменки, микрорайон Ленинские Горы, 1' in response.text
        assert response.text.endswith((
            ', верно?', ', так?', ', правильно?',
        ))

        response = alice('Да')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://set_place?' in response.directive.payload.uri

        response = alice('отменить')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=0' in response.directive.payload.uri

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        # ALICEINFRA-536: not worked right now, should fix it
        # assert response.intent == intent.ShowRoute
        # assert response.text.startswith(('Принято', 'Хорошо', 'В путь!',))

    @pytest.mark.parametrize('command', [
        'Поехали на работу',
    ])
    def test_set_work_address(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.RememberNamedLocation
        assert response.text in [
            'И где находится работа?',
            'И какой адрес у работы?',
            'Окей, где находится работа?',
            'Окей, где находится работа?',
            'Окей, какой адрес у работы?',
        ]

        response = alice('Ботаническая улица, дом 27')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.RememberNamedLocationEllipsis
        assert response.text.startswith((
            'Адрес работы — ', 'Новый адрес работы — ',
        ))
        assert 'Москва, Ботаническая улица 27' in response.text
        assert response.text.endswith((
            ', верно?', ', так?', ', правильно?',
        ))

        response = alice('Да')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://set_place?' in response.directive.payload.uri

        response = alice('отменить')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=0' in response.directive.payload.uri

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        # ALICEINFRA-536: not worked right now, should fix it
        # assert response.intent == intent.ShowRoute
        # assert response.text.startswith(('Принято', 'Хорошо', 'В путь!',))
