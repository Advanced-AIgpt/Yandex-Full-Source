import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
class TestFavoritePlaces(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1220
    https://testpalm.yandex-team.ru/testcase/alice-1598
    """

    owners = ('isiv',)

    _FAVORITES = {
        'user_favorites': [
            {
                'name': 'Бабушка',
                'lat': 55.86972809,
                'lon': 37.66465378,
            },
            {
                'name': 'Мое любимое кафе',
                'lat': 55.73841476,
                'lon': 37.66647339,
            }
        ]
    }

    @pytest.mark.parametrize('command, favorites', [
        ('Поехали к бабушке', _FAVORITES['user_favorites'][0]),
        ('Поехали в мое любимое кафе', _FAVORITES['user_favorites'][1]),
    ])
    @pytest.mark.device_state(navigator=_FAVORITES)
    def test_go_to_favorites(self, alice, command, favorites):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        uri = response.directive.payload.uri
        assert uri.startswith('yandexnavi://build_route_on_map?confirmation=1')
        assert f'lat_to={favorites["lat"]:.6f}'[:-1] in uri and f'lon_to={favorites["lon"]:.6f}'[:-1] in uri

        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем до "{favorites["name"]}".' in response.text
