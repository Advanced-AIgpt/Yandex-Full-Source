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
    'какие сейчас пробки',
    'пробки в Екатеринбурге',
    'ситуация на дорогах',
])
class Tests(_TestBase):

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.navi,
        surface.loudspeaker,
        surface.smart_tv,
        surface.station,
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowTraffic
        assert not response.directive
        assert 'балл' in response.output_speech_text


@pytest.mark.xfail
@pytest.mark.parametrize('surface', [
    surface.smart_display,
])
@pytest.mark.experiments('show_route_div_card_centaur_on')
class TestsCentaur(_TestBase):

    @pytest.mark.parametrize('command', [
        'какие сейчас пробки',
        'пробки в Екатеринбурге',
        'ситуация на дорогах',
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ShowTraffic
        assert response.directives[0].name == directives.names.ShowViewDirective
        # assert response.directives[0].Div2Card.Id.CardId == 'show.traffic.div.card'
        assert response.directives[1].name == directives.names.TtsPlayPlaceholderDirective
