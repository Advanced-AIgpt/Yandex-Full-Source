import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from market import util


class TestPalmOrdersStatus(object):
    owners = ('mllnr',)

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test_user_without_orders(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2554
        """
        response = alice('Статус моего заказа')
        assert response.scenario == scenario.MarketOrdersStatus
        util.assert_with_regexps(
            response.text,
            util.OrdersStatusPhrases.get_no_orders_phrases(),
            full=True,
        )
        util.assert_with_regexps(
            response.output_speech_text,
            util.OrdersStatusPhrases.get_no_orders_voice_phrases(),
            full=True,
        )

        button = response.button('Перейти на Яндекс.Маркет')
        assert button
        assert button['directives'][0]['payload']['uri'] == 'https://' + util.Netlocs.BLUE_MARKET

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test_user_with_orders(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2555
        https://testpalm.yandex-team.ru/testcase/alice-2556
        """
        response = alice('Что с моим заказом')
        assert response.scenario == scenario.MarketOrdersStatus
        util.assert_with_regexps(
            response.text,
            util.OrdersStatusPhrases.get_completed_order_phrases()
                + util.OrdersStatusPhrases.get_unfinished_order_phrases(),
            full=True,
        )
        util.assert_with_regexps(
            response.output_speech_text,
            util.OrdersStatusPhrases.get_completed_order_voice_phrases()
                + util.OrdersStatusPhrases.get_unfinished_order_voice_phrases(),
            full=True,
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou
