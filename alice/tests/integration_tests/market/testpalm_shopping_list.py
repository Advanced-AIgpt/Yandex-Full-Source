import re

import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from market import div_card, util


def assert_shopping_list(card, expected_items):
    assert [item.value for item in card] == list(expected_items)
    for i, item in enumerate(card):
        assert card[i].index == i + 1


class TestPalmShoppingList(object):
    owners = ('mllnr',)

    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_add_items(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2314
        """
        response = alice('Очисти список покупок')

        response = alice('Алиса, добавь в список покупок зимнее пальто')
        util.assert_with_regexps(
            response.text, util.ShoppingList.get_item_added_phrases('зимнее пальто'))
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice('Алиса, добавь зимнее пальто в список покупок')
        util.assert_with_regexps(response.text, util.ShoppingList.get_duplicate_item_phrases())
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice.click(response.suggest('Список покупок'))
        util.assert_with_regexps(response.text, util.ShoppingList.get_show_list_card_phrases())
        assert_shopping_list(div_card.ShoppingList(response.div_card), ['зимнее пальто'])

        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса')
        util.assert_with_regexps(
            response.text,
            util.ShoppingList.get_items_added_phrases([
                'зеленый чай',
                'кофе в зернах',
                'горшок для кактуса',
            ]),
        )
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice.click(response.button('Список покупок'))
        util.assert_with_regexps(response.text, util.ShoppingList.get_show_list_card_phrases())
        assert_shopping_list(div_card.ShoppingList(response.div_card), [
            'зимнее пальто',
            'зеленый чай',
            'кофе в зернах',
            'горшок для кактуса',
        ])

    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_remove_items(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2315
        """
        response = alice('Очисти список покупок')
        response = alice('Алиса, добавь в список покупок зимнее пальто')
        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса')
        response = alice('Список покупок')
        assert_shopping_list(div_card.ShoppingList(response.div_card), [
            'зимнее пальто',
            'зеленый чай',
            'кофе в зернах',
            'горшок для кактуса',
        ])

        response = alice('Удали кофе в зернах из списка покупок')
        util.assert_with_regexps(
            response.text, util.ShoppingList.get_item_removed_phrases('кофе в зернах'))
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice('Удали наушники')
        util.assert_with_regexps(response.text, util.ShoppingList.get_nothing_to_remove_phrases())
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice.click(response.suggest('Список покупок'))
        util.assert_with_regexps(response.text, util.ShoppingList.get_show_list_card_phrases())
        assert_shopping_list(div_card.ShoppingList(response.div_card), [
            'зимнее пальто',
            'зеленый чай',
            'горшок для кактуса',
        ])

        response = alice('Удали зимнее пальто и зеленый чай из списка покупок')
        util.assert_with_regexps(response.text, util.ShoppingList.get_items_removed_phrases([
            'зимнее пальто',
            'зеленый чай',
        ]))
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice('Список покупок')
        util.assert_with_regexps(response.text, util.ShoppingList.get_show_list_card_phrases())
        assert_shopping_list(div_card.ShoppingList(response.div_card), ['горшок для кактуса'])

    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_remove_all(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2316
        """
        response = alice('Очисти список покупок')
        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса')
        response = alice('Список покупок')
        assert_shopping_list(div_card.ShoppingList(response.div_card), [
            'зеленый чай',
            'кофе в зернах',
            'горшок для кактуса',
        ])

        response = alice('Алиса, удали мой список покупок')
        util.assert_with_regexps(response.text, util.ShoppingList.get_all_removed_phrases())

        response = alice('Что в моем списке покупок?')
        util.assert_with_regexps(response.text, util.ShoppingList.get_empty_list_phrases())

    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_list_with_market_goods(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2317
        """
        response = alice('Очисти список покупок')
        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса')
        response = alice('Список покупок')
        util.assert_with_regexps(response.text, util.ShoppingList.get_show_list_card_phrases())
        card = div_card.ShoppingList(response.div_card)
        has_market_goods = False
        for i, item in enumerate(card):
            assert item.index == i + 1
            assert item.value
            if item.market_url:
                has_market_goods = True
                assert re.search(r'\d+ предложени(е|я|й)', item.offer_number_label)
                assert item.picture_url
                util.MarketSearchUrlValidator(item.market_url) \
                    .assert_netloc(util.Netlocs.BLUE_MARKET_MOBILE) \
                    .assert_single_param('text', item.value)
        assert has_market_goods, (
            'expected to have at least one item with market url\n'
            'try to change user shopping list'
        )

    @pytest.mark.no_oauth
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_authorization(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2321
        """
        response = alice('Добавь в список покупок новый айфон')
        util.assert_with_regexps(response.text, util.Auth.get_ask_login_mobile_phrases())
        assert response.button('Авторизация')
        assert response.suggest('Я залогинился')

        response = alice.click(response.suggest('Я залогинился'))
        util.assert_with_regexps(response.text, util.Auth.get_ask_login_again_mobile_phrases())

        alice.login(auth.RobotMarketNoBeruOrders)

        response = alice.click(response.suggest('Я залогинился'))
        util.assert_with_regexps(response.text, util.ShoppingList.get_item_added_phrases('новый айфон'))
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

        response = alice('Удали новый айфон')
        util.assert_with_regexps(response.text, util.ShoppingList.get_item_removed_phrases('новый айфон'))
        assert response.button('Список покупок')
        assert response.suggest('Список покупок')

    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_fact(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2322
        """
        response = alice('Алиса, удали кетчуп из списка покупок')

        response = alice('Алиса, добавь в список покупок кетчуп')
        util.assert_with_regexps(response.text, util.ShoppingList.get_item_added_phrases('кетчуп'))
        fact_card = str(response.div_card.raw)
        assert 'A вы знаете?' in fact_card
        # TODO  ^ latin letter
        assert 'кетчуп' in fact_card

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_voice_add_items(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2318
        """
        response = alice('Очисти список покупок')

        response = alice('Добавь в список покупок наушники и крем для рук')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_items_added_phrases([
                'наушники',
                'крем для рук',
            ]),
        )

        response = alice('Добавь в список покупок наушники и крем для рук')
        util.assert_with_regexps(
            response.output_speech_text, util.ShoppingList.get_duplicate_items_phrases())

        response = alice('Алиса, что в моем списке покупок')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_show_list_voice_phrases([
                'наушники',
                'крем для рук',
            ]),
        )

        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса.')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_items_added_phrases([
                'зеленый чай',
                'кофе в зернах',
                'горшок для кактуса',
            ]),
        )

        # TODO(bas1330) далее идёт проверка, что в ПП тот же список покупок,
        # но в EVO пока нельзя менять платформу на лету

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICERELEASE-3269')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_voice_remove_items(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2319
        """
        response = alice('Очисти список покупок')
        response = alice('Добавь в список покупок наушники и крем для рук')
        response = alice('Добавь зеленый чай, кофе в зернах и горшок для кактуса.')
        response = alice('Алиса, что в моем списке покупок')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_show_list_voice_phrases([
                'наушники',
                'крем для рук',
                'зеленый чай',
                'кофе в зернах',
                'горшок для кактуса',
            ]),
        )

        response = alice('Удали кофе в зернах из списка покупок')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_item_removed_phrases('кофе в зернах'),
        )

        response = alice('Удали беруши')
        util.assert_with_regexps(
            response.output_speech_text, util.ShoppingList.get_not_found_phrases())

        response = alice('Удали наушники и зеленый чай из списка покупок')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_items_removed_phrases([
                'наушники',
                'зеленый чай',
            ]),
        )

        response = alice('Список покупок')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_show_list_voice_phrases([
                'крем для рук',
                'горшок для кактуса',
            ]),
        )

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_voice_remove_all(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2320
        """
        response = alice('Очисти список покупок')
        response = alice('Добавь в список покупок наушники и крем для рук')

        response = alice('Алиса, удали мой список покупок')
        util.assert_with_regexps(
            response.output_speech_text, util.ShoppingList.get_all_removed_phrases())

        response = alice('Что в моем списке покупок?')
        util.assert_with_regexps(
            response.output_speech_text, util.ShoppingList.get_empty_list_phrases())

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_voice_fact_disabled(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2323
        """
        response = alice('Алиса, удали кетчуп из списка покупок')

        response = alice('Алиса, добавь в список покупок кетчуп')
        util.assert_with_regexps(
            response.output_speech_text,
            util.ShoppingList.get_item_added_phrases('кетчуп'),
            full=True,
        )
