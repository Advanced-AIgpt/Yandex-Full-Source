import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from market import util


class TestMarketOrdersStatus(object):

    owners = ('mllnr',)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_analytics_info(self, alice):
        response = alice('что с моим заказом')
        assert response.intent == intent.MarketProtocolOrdersStatus
        assert response.scenario_analytics_info.product_scenario == 'market_orders_status'
        assert response.scenario == scenario.MarketOrdersStatus

        response = alice('я залогинился')
        assert response.intent == intent.MarketProtocolUserLoggedIn
        assert response.scenario_analytics_info.product_scenario == 'market_orders_status'
        assert response.scenario == scenario.MarketOrdersStatus

    @pytest.mark.no_oauth
    @pytest.mark.parametrize('surface', [
        surface.searchapp,
    ])
    def test_ask_login_searchapp(self, alice):
        response = alice('что с моим заказом')
        assert response.scenario == scenario.MarketOrdersStatus
        util.assert_with_regexps(response.text, util.Auth.get_ask_login_mobile_phrases())

    @pytest.mark.no_oauth
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.yabro_win,
    ])
    def test_ask_login(self, alice):
        response = alice('что с моим заказом')
        assert response.scenario == scenario.MarketOrdersStatus
        util.assert_with_regexps(response.text, util.Auth.get_ask_login_yabro_phrases())

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.station,
        surface.watch,
    ])
    def test_not_supported_platforms(self, alice):
        response = alice('что с моим заказом')
        assert response.scenario != scenario.MarketOrdersStatus
        # this intent still exists in vins classifier
        assert response.intent != intent.MarketOrdersStatus
