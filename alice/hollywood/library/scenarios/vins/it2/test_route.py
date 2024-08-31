import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['vins']


@pytest.mark.evo
@pytest.mark.scenario(name='Vins', handle='vins')
@pytest.mark.voice
@pytest.mark.region(region.Moscow)
class _TestBase:
    pass


@pytest.mark.parametrize('command', [
    'сколько ехать до Внуково',
    'поехали в магазин',
    'построй маршрут из Москвы в Волгоград',
    'поехали в Сочи из Смоленска',
])
class Tests(_TestBase):

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test_open_map(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert not response.directive

        response = alice('открой на карте')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.smart_tv,
        surface.station,
    ])
    def test_unsupported(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert not response.directive

        response = alice('открой на карте')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRouteOnMap
        assert not response.directive
        assert (
            response.text.startswith(('Я справлюсь с этим лучше на', 'Я бы и рада', 'Я могу разве что')) or
            response.text == 'Сейчас под рукой нет карты. Да и рук у меня нет. Давайте сменим тему.'
        )


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.navi,
])
class TestsNavi(_TestBase):

    @pytest.mark.parametrize('command', [
        'сколько ехать до Внуково',
    ])
    def test_open_map(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert not response.directive

        response = alice('открой на карте')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective

    @pytest.mark.parametrize('command', [
        'поехали в магазин',
        'построй маршрут из Москвы в Волгоград',
        'поехали в Сочи из Смоленска',
    ])
    def test_go_to_route(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.Uri

        response = alice('Поехали')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.Uri


@pytest.mark.parametrize('surface', [
    surface.smart_display,
])
@pytest.mark.experiments('show_route_div_card_centaur_on', 'show_route_gallery')
class TestsCentaur(_TestBase):

    @pytest.mark.parametrize('command', [
        'поехали в магазин',
        'сколько ехать до Внуково',
        'построй маршрут из Москвы в Волгоград',
        'поехали в Сочи из Смоленска',
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowRoute
        assert response.directives[0].name == directives.names.ShowViewDirective
        assert response.directives[0].Div2Card.Id.CardId == 'show.route.div.card'
        assert response.directives[1].name == directives.names.TtsPlayPlaceholderDirective
