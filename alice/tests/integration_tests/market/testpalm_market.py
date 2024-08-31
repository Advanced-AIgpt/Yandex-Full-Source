import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest

from market import div_card, util
from market.analytics_info import MarketGalleryAnalyticsInfo


class TestPalmMarket(object):

    owners = ('mllnr',)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_activation(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1136
        """
        # enter scenario
        response = alice('помоги с покупками')
        assert response.intent == intent.Market
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_activation_phrases())

        util.assert_has_suggests(response, [util.Suggests.CANCEL])
        response = alice.click(response.suggest(util.Suggests.CANCEL))
        assert response.intent == intent.MarketCancel

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_gallery(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1137
        """
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('наушники')
        util.assert_with_regexps(
            response.text,
            util.ChoicePhrases.get_ask_continue_phrases('наушники'),
        )
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketExtendedGallery(response.div_card)
        util.assert_extended_gallery(gallery, count=10)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_gallery_offer(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1139
        """
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('бипер')
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_phrases())
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketExtendedGallery(response.div_card)
        util.assert_extended_gallery(gallery)

        analytics_info = MarketGalleryAnalyticsInfo(response)
        assert analytics_info.find_item_idx(type_='offer') is not None, \
            'Expected at least one offer item in gallery'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_specify_color(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1140
        """
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Мобильный телефон')
        assert response.intent == intent.MarketEllipsis

        response = alice('красный')
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_phrases())
        assert 'красный' in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketExtendedGallery(response.div_card)
        util.assert_extended_gallery(gallery)

        util.MarketSearchUrlValidator(gallery.market_url) \
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_single_param('text', 'мобильный телефон красный')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_specify_price(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1141
        """
        # TODO(bas1330) стоит протестировать варианты "до 5000" "от 8000"
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Лучшие акустические гитары')
        assert response.intent == intent.MarketEllipsis

        response = alice('От шести до семи тысяч')
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_phrases())
        assert 'Цена: 6000 - 7000 ₽' in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketExtendedGallery(response.div_card)
        util.assert_extended_gallery(gallery)

        util.MarketSearchUrlValidator(gallery.market_url)\
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_param_is_not_set('text') \
            .assert_single_param('suggest_text', 'Акустические гитары') \
            .assert_single_param('pricefrom', '6000') \
            .assert_single_param('priceto', '7000')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, glfilter, response_text, url_suggest_text, url_text', [
        (
            'Утюг дорожный',
            '29163928:1',
            '✔ Дорожный',
            None,
            'утюг дорожный',
        ),
        (
            'С керамической подошвой',
            '16367311:16367314',
            'Материал подошвы: керамика',
            'Утюги с керамической подошвой',
            None,
        ),
        (
            'Мощностью от 2000 ватт',
            '14805494:2000~',
            'Мощность: от 2000 Вт',
            None,
            'утюг мощностью от 2000 ватт',
        ),
    ])
    def test_market_specify_param(self, alice, command, glfilter, response_text, url_suggest_text, url_text):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1142
        """
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Купить утюг')
        assert response.intent == intent.MarketContinue

        response = alice(command)
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_phrases())
        assert response_text in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketExtendedGallery(response.div_card)
        util.assert_extended_gallery(gallery)

        validator = util.MarketSearchUrlValidator(gallery.market_url)\
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_single_param('glfilter', glfilter)
        if url_suggest_text is not None:
            validator.assert_single_param('suggest_text', url_suggest_text)
        if url_text is not None:
            validator.assert_single_param('text', url_text)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_product_offers_card(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1143
        https://testpalm.yandex-team.ru/testcase/alice-1144
        """
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Мультиварка')
        assert response.intent == intent.MarketEllipsis

        analytics_info = MarketGalleryAnalyticsInfo(response)
        i = analytics_info.find_item_idx(type_='model', has_rating=True)
        assert i is not None, 'Expected at least one model with rating in gallery'
        gallery_item = div_card.MarketExtendedGallery(response.div_card)[i]
        gallery_item_meta = analytics_info[i]

        response = alice.click(gallery_item.buttons(util.Buttons.PRODUCT_DETAILS))
        product_card = div_card.MarketProductOffersCard(response.div_card)

        assert product_card.picture_url
        assert product_card.title in gallery_item.title
        if not gallery_item.price.is_from_price:
            # TODO(bas1330) может сломаться - https://st.yandex-team.ru/MALISA-944
            # assert gallery_item.price in product_card.offer_prices
            pass
        assert len(product_card.offer_prices) >= 1

        assert gallery_item.rating_icon_url \
            and gallery_item.rating_icon_url == product_card.rating_icon_url

        assert product_card.market_offers_button
        util.MarketProductUrlValidator(product_card.market_offers_button.action_url) \
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_tab('offers') \
            .assert_model_id(gallery_item_meta.model_id) \
            .assert_single_param('lr', '213')

        if gallery_item.has_voice_purchase_mark():
            assert util.Buttons.ORDER_WITH_ALICE in product_card.market_buttons

        # TODO(bas1330) стоит сделать 3 отдельных теста. На оффер и модели с/без покупки голосом
        util.assert_suggests(
            response,
            util.Suggests.get_product_card_suggests(
                has_order=gallery_item.has_voice_purchase_mark()
            ),
        )

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_voice_purchase_buttons(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1145
        https://testpalm.yandex-team.ru/testcase/alice-1146
        """
        # enter scenario
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Детское пюре')
        assert response.intent == intent.MarketEllipsis

        analytics_info = MarketGalleryAnalyticsInfo(response)
        i = analytics_info.find_item_idx(type_='model', has_voice_purchase=True)
        assert i is not None, 'Expected at least one model with voice purchase in gallery'
        gallery_item = div_card.MarketExtendedGallery(response.div_card)[i]
        assert gallery_item.has_voice_purchase_mark()

        response = alice.click(gallery_item.buttons(util.Buttons.PRODUCT_DETAILS))
        product_card = div_card.MarketProductOffersCard(response.div_card)
        util.assert_voice_purchase_links(product_card.voice_purchase_links)
        util.assert_suggests(response, util.Suggests.get_product_card_suggests(has_order=True))

        order_button = product_card.market_buttons.get(util.Buttons.ORDER_WITH_ALICE)
        assert order_button
        response = alice.click(order_button)

        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_start_phrases())
        assert response.intent == intent.MarketCheckout
        util.assert_has_suggests(response, util.Suggests.get_choice_suggests())

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_suggests(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1693
        """
        alice('Посоветуй товар')

        response = alice('блютуз гарнитура')
        gallery = div_card.MarketExtendedGallery(response.div_card)
        i = next((i for i, item in enumerate(gallery) if item.has_voice_purchase_mark()), None)
        assert i is not None, 'Expected at least one model with voice purchase in gallery'

        response = alice.click(gallery[i].buttons(util.Buttons.PRODUCT_DETAILS))
        util.assert_suggests(
            response,
            [util.Suggests.ORDER_WITH_ALICE] + util.Suggests.get_choice_suggests(),
        )

        # process to delivery options step
        response = alice.click(response.suggest(util.Suggests.ORDER_WITH_ALICE))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_address_phrases()):
            response = alice('город Москва, Льва Толстого 16')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            # Доступно только одно время доставки, поэтому Алиса переходит к подтверждению заказа
            return
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        # Проверяем варианты даты доставки
        options = util.get_phrase_options(response.text)
        assert len(options) >= 2
        for i, option in enumerate(options):
            assert option.index == i + 1
            util.assert_with_regexp(option.value, util.Delivery.DATETIME_REGEXP)
        # https://st.yandex-team.ru/MALISA-943
        util.assert_suggests(
            response,
            [str(i + 1) for i in range(len(options))] + util.Suggests.get_choice_suggests(),
        )

        # Проверяем дату доставки и саджест подтверждения заказа
        delivery_date = re.search(util.DATE_REGEXP, options[0].value).group()
        response = alice('один')
        card = div_card.OrderDetailsCard(response.div_card)
        assert delivery_date in card.details['Доставка']
        # https://st.yandex-team.ru/MALISA-943
        util.assert_suggests(
            response,
            ['Да, верно', 'Нет', util.Suggests.CANCEL, util.Suggests.START_AGAIN],
        )

        response = alice('нет')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_not_confirmed_order_phrases(),
        )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_exit_with_voice(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1147
        """
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Купить утюг')
        assert response.intent == intent.MarketContinue

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_exit_with_suggest_tap(self, alice):
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Купить утюг')
        assert response.intent == intent.MarketContinue

        response = alice.click(response.suggest(util.Suggests.CANCEL))
        assert response.intent == intent.MarketCancel

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_start_again(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1545
        """
        response = alice('Алиса, посоветуй товар')
        assert response.intent == intent.Market

        response = alice('Купить утюг')
        assert response.intent == intent.MarketContinue

        response = alice.click(response.suggest(util.Suggests.START_AGAIN))
        assert response.intent == intent.MarketStartAgain
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_start_again_phrases())

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel
