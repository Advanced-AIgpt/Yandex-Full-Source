import re
import collections

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest

from market import div_card, util


class TestPalmMarketBeru(object):

    owners = ('mllnr',)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_beru_activation(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1223
        """
        response = alice('помоги купить на беру')
        assert response.intent == intent.Market
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_activation_phrases())

        util.assert_has_suggests(response, [util.Suggests.CANCEL])
        response = alice.click(response.suggest(util.Suggests.CANCEL))
        assert response.intent == intent.MarketCancel

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_gallery(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1225
        """
        response = alice('Помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('подгузники')
        assert response.intent == intent.MarketEllipsis
        util.assert_with_regexps(
            response.text,
            util.ChoicePhrases.get_ask_continue_blue_phrases('подгузники'),
        )
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketGallery(response.div_card)
        util.assert_blue_gallery(gallery, count=10)
        util.MarketSearchUrlValidator(gallery.market_url) \
            .assert_netloc(util.Netlocs.BLUE_MARKET_MOBILE)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_specify_color(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1227
        """
        response = alice('Помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('Мобильный телефон')
        assert response.intent == intent.MarketEllipsis

        response = alice('синего цвета')
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_blue_phrases())
        assert 'Цвет: синий' in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketGallery(response.div_card)
        util.assert_blue_gallery(gallery, count=10)
        util.MarketSearchUrlValidator(gallery.market_url) \
            .assert_netloc(util.Netlocs.BLUE_MARKET_MOBILE) \
            .assert_single_param('text', 'мобильный телефон синего цвета') \
            .assert_single_param('glfilter', '13887626:13898977')

    @pytest.mark.parametrize('command, response_text, price_from, price_to', [
        ('За пятнадцать тысяч рублей', 'Цена: 13500 - 16500 ₽', 13500, 16500),  # price +- 10%
        ('От десяти до двадцати тысяч рублей', 'Цена: 10000 - 20000 ₽', 10000, 20000),
        ('Не дороже семнадцати тысяч рублей', 'Цена: до 17000 ₽', None, 17000),
        ('От семи тысяч рублей', 'Цена: от 7000 ₽', 7000, None),
    ])
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_specify_price(self, alice, command, response_text, price_from, price_to):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1228
        """
        response = alice('Помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('Мобильный телефон')
        assert response.intent == intent.MarketEllipsis

        response = alice(command)
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_blue_phrases())
        assert response_text in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketGallery(response.div_card)
        util.assert_blue_gallery(gallery, count=10, price_from=price_from, price_to=price_to)
        util.MarketSearchUrlValidator(gallery.market_url)\
            .assert_netloc(util.Netlocs.BLUE_MARKET_MOBILE) \
            .assert_single_param('suggest_text', 'Мобильные телефоны') \
            .assert_single_param('pricefrom', str(price_from) if price_from else price_from) \
            .assert_single_param('priceto', str(price_to) if price_to else price_to)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_specify_param(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1230
        """
        response = alice('Помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('Пылесос')
        assert response.intent == intent.MarketEllipsis

        response = alice('bosch')
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_blue_phrases())
        assert 'Производитель: Bosch' in response.text
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        gallery = div_card.MarketGallery(response.div_card)
        util.assert_blue_gallery(gallery, count=10)
        util.MarketSearchUrlValidator(gallery.market_url)\
            .assert_netloc(util.Netlocs.BLUE_MARKET_MOBILE) \
            .assert_single_param('glfilter', '7893318:13480891') \
            .assert_single_param('text', 'пылесос bosch') \
            .assert_param_is_not_set('suggest_text')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_product_card(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1234
        """
        response = alice('Помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('Детские автокресла')
        assert response.intent == intent.MarketEllipsis
        gallery_item = div_card.MarketGallery(response.div_card).first

        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))

        # check card
        card = div_card.MarketBlueProductCard(response.div_card)
        util.assert_picture(card.picture_url)
        assert card.title == gallery_item.title
        assert card.price == gallery_item.price
        assert card.buttons(util.Buttons.CHECKOUT)
        util.assert_voice_purchase_links(card.voice_purchase_links)
        # check card delivery
        has_promo_delivery = 'Бесплатная доставка' in card.raw_str
        if not has_promo_delivery:
            util.assert_has_regexp(card.raw_str, util.Delivery.PRODUCT_CARD_REGEXP)

        util.assert_suggests(
            response, [util.Suggests.CHECKOUT] + util.Suggests.get_choice_suggests())

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_point_to_market(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1226
        """
        alice('помоги купить на беру')
        response = alice('купить atomic hawk')
        assert response.intent == intent.MarketContinue
        util.assert_suggests(response, util.Suggests.get_choice_suggests())
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_point_to_market_phrases())

        # check text and link to Market on the button
        assert len(response.buttons) == 1
        button = response.button(util.Buttons.OPEN_MARKET)
        util.MarketSearchUrlValidator(button['directives'][0]['payload']['uri']) \
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_single_param('text', 'atomic hawk')
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_gradually_point_to_market(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1232
        """
        alice('помоги купить на беру')
        response = alice('atomic')
        assert response.intent == intent.MarketEllipsis  # items found, show here
        util.assert_with_regexps(response.text, util.ChoicePhrases.get_ask_continue_blue_phrases())

        response = alice('hawk')
        # check text and link to Market on the button
        assert len(response.buttons) == 1
        button = response.button(util.Buttons.OPEN_MARKET)  # now items not found, go to market
        util.MarketSearchUrlValidator(button['directives'][0]['payload']['uri']) \
            .assert_netloc(util.Netlocs.MARKET_MOBILE) \
            .assert_single_param('text', 'atomic hawk')
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_garbage(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1231
        """
        alice('помоги купить на беру')
        response = alice('погода в екатеринбурге')
        assert response.intent == intent.MarketGarbage

        assert any(lolwhat in response.text for lolwhat in [
            'Извините, я вас не поняла. Давайте как-нибудь по-другому.',
            'Это не похоже на параметры товара.',
        ])
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_disable_by_voice(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1235
        """
        response = alice('помоги купить на беру')
        assert response.intent == intent.Market

        response = alice('мультиварки')
        assert response.intent == intent.MarketEllipsis

        response = alice('хватит')
        assert response.intent == intent.MarketCancel

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
    def test_user_with_orders_checkout(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1236
        https://testpalm.yandex-team.ru/testcase/alice-1819
        Проверяем, что почта, телефон и адрес достаются из последнего заказа пользователя.
        У пользователя последний заказ должен быть курьерской доставкой в московский офис.
        ПВЗ заказов у пользователя быть не должно.
        TODO(bas1330) consider documentation how to setup users
        """
        # goto checkout step
        alice('помоги купить на беру')
        response = alice('детские автокресла')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))

        checkout_button = response.div_card.buttons(util.Buttons.CHECKOUT)
        assert checkout_button.directives
        type_silent_directive = checkout_button.directives[0]
        assert type_silent_directive['name'] == directives.names.TypeTextSilentDirective
        assert type_silent_directive['payload']['text'] == util.Buttons.CHECKOUT

        # goto delivery options step
        response = alice.click(checkout_button)
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        # check delivery options step
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )
        delivery_options = util.get_phrase_options(response.text)
        assert len(delivery_options) >= 2
        delivery_date_match = re.search(util.DATE_REGEXP, delivery_options[0].value)
        assert delivery_date_match, f'can\'t find date in delivery option "{delivery_options[0]}"'
        delivery_date = delivery_date_match.group()

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Почта'] == 'with-orders@yandex.ru'
        assert card.details['Телефон'] == '+7 912 043-00-04'
        assert delivery_date in card.details['Доставка']
        assert card.details['Способ доставки'] == 'Курьерская доставка'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        assert card.details['Адрес'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert card.details['Количество'] == '1'
        assert 'бесплатно' in card.details['Стоимость доставки']
        assert card.details['Итого к оплате'] == gallery_item.price_text
        assert card.price_text == gallery_item.price_text

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithPickup)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_pvz_delivery(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1818
        У пользователя последний заказ должен быть ПВЗ доставкой.
        Заказов с курьерской доставкой у пользователя быть не должно.
        TODO(bas1330) consider documentation how to setup users
        """
        # goto delivery address step
        alice('помоги купить на беру')
        response = alice('детские каши')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        # check pvz
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_or_select_pvz_phrases(),
        )
        pvz = response.text.split('\n')[-1]
        assert re.fullmatch(util.Delivery.PVZ_REGEXP, pvz)
        match = re.match(util.Delivery.PVZ_REGEXP, pvz)
        pvz_address = match['address']
        pvz_price = match['price']

        response = alice('да')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Способ доставки'] == 'Самовывоз'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        # TODO(bas1330) заменить на replace('<br/>', ' ') после исправления MALISA-791
        assert card.details['Адрес'].replace('<br/> ', '') == pvz_address
        assert pvz_price in card.details['Стоимость доставки']

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDeliveryAndPickup)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_courier_delivery_by_index(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1820
        """
        # goto delivery address step
        alice('помоги купить на беру')
        response = alice('детские каши')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        # check delivery address options
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_or_select_option_phrases(),
        )
        address_options = util.get_phrase_options(response.text)
        assert len(address_options) == 2
        assert re.fullmatch(util.Delivery.COURIER_REGEXP, address_options[0].value)
        assert re.fullmatch(util.Delivery.PVZ_REGEXP, address_options[1].value)
        match = re.match(util.Delivery.COURIER_REGEXP, address_options[0].value)
        delivery_address = match['address']
        delivery_price = match['price']

        response = alice('один')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Почта'] == 'alice-pvz-kur2@yandex.ru'
        assert card.details['Способ доставки'] == 'Курьерская доставка'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        assert card.details['Адрес'] == delivery_address
        assert delivery_price in card.details['Стоимость доставки']

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithPickup)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_say_address_when_pvz_available(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1821
        """
        # goto delivery address step
        alice('помоги купить на беру')
        response = alice('детские каши')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_or_select_pvz_phrases(),
        )
        pvz = response.text.split('\n')[-1]
        assert re.fullmatch(util.Delivery.PVZ_REGEXP, pvz)

        response = alice('город Москва, Льва Толстого 16')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Способ доставки'] == 'Курьерская доставка'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        assert card.details['Адрес'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert 'бесплатно' in card.details['Стоимость доставки']

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithPickup)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_deny_pvz(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1822
        """
        # goto delivery address step
        alice('помоги купить на беру')
        response = alice('детские каши')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_or_select_pvz_phrases(),
        )
        pvz = response.text.split('\n')[-1]
        assert re.fullmatch(util.Delivery.PVZ_REGEXP, pvz)

        response = alice.click(response.suggest('Нет'))
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_address_phrases())

        response = alice('город Москва, Льва Толстого 16')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Способ доставки'] == 'Курьерская доставка'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        assert card.details['Адрес'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert 'бесплатно' in card.details['Стоимость доставки']

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDeliveryAndPickup)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_select_pvz_by_index(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1823
        """
        # goto delivery address step
        alice('помоги купить на беру')
        response = alice('детские каши')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases()):
            assert False, (
                'Rare case when only one delivery option available\n'
                'If it happens often, maybe scenario is broken'
            )

        # check delivery address options
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_or_select_option_phrases(),
        )
        address_options = util.get_phrase_options(response.text)
        assert len(address_options) == 2
        assert re.fullmatch(util.Delivery.COURIER_REGEXP, address_options[0].value)
        assert re.fullmatch(util.Delivery.PVZ_REGEXP, address_options[1].value)
        match = re.match(util.Delivery.PVZ_REGEXP, address_options[1].value)
        delivery_address = match['address']
        delivery_price = match['price']

        response = alice.click(response.suggest('2'))
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Способ доставки'] == 'Самовывоз'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        # TODO(bas1330) заменить на replace('<br/>', ' ') после исправления MALISA-791
        assert card.details['Адрес'].replace('<br/> ', '') == delivery_address
        assert delivery_price in card.details['Стоимость доставки']

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketNoBeruOrders)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
    def test_delivery_unavailable(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1151
        """
        # goto address step
        alice('помоги купить на беру')
        response = alice('детское пюре')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_address_phrases())

        response = alice('Териберка, Мурманская 14')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_delivery_unavailable_phrases(),
        )
        util.assert_suggests(response, util.Suggests.get_choice_suggests())

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716')
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
    def test_guest_checkout(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1237
        """
        alice('помоги купить на беру')
        response = alice('детские автокресла')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons('Оформить заказ на Яндекс.Маркете'))

        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')

        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_email_phrases())

        response = alice('sparkle@yandex-team.ru')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_phone_phrases())

        response = alice('+7-123-456-78-90')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_address_phrases())

        response = alice('город Москва, Льва Толстого 16')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Почта'] == 'sparkle@yandex-team.ru'
        assert card.details['Телефон'] == '+7 (123) 456-78-90'
        assert card.details['Способ доставки'] == 'Курьерская доставка'
        assert card.details['Способ оплаты'] == 'Наличными при получении заказа'
        assert card.details['Адрес'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert card.details['Количество'] == '1'
        assert 'бесплатно' in card.details['Стоимость доставки']
        assert card.details['Итого к оплате'] == gallery_item.price_text
        assert card.price_text == gallery_item.price_text

        response = alice('нет')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_not_confirmed_order_phrases(),
        )
        assert util.Suggests.CANCEL not in {suggest.title for suggest in response.suggests}

        open_beru_button = response.cards[0].buttons[0]
        assert open_beru_button.title == 'Оформить заказ на Яндекс.Маркете'
        button_directive = open_beru_button.directives[0]
        assert button_directive.name == directives.names.OpenUriDirective
        assert button_directive.payload.uri.startswith('https://m.pokupki.market.yandex.ru/checkout?offer=')

        response = alice('как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.experiments('market_pvz')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_suggests(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1694
        """
        # goto delivery options step
        alice('помоги купить на беру')
        response = alice('детские подгузники')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        util.assert_has_suggests(response, [util.Suggests.CHECKOUT])
        response = alice.click(response.suggest(util.Suggests.CHECKOUT))
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

        # check delivery options
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


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmMarketBeruIntervalsVoice(object):

    owners = ('mllnr',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    def test_delivery_by_choice_number(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2025
        """
        # process to delivery options step
        alice('помоги купить на беру')
        response = alice('детское пюре')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
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

        # проверка доступных интервалов доставки
        options = util.get_phrase_options(response.text)
        assert len(options) >= 2

        # получаем дату доставки
        delivery_date_match = re.search(util.DATE_REGEXP, options[-1].value)
        assert delivery_date_match, f'can\'t find date in delivery option "{options[-1]}"'
        delivery_date = delivery_date_match.group()

        # берем последний интервал
        response = alice(
            ['первый', 'второй', 'третий', 'четвёртый', 'пятый', 'шестой'][len(options) - 1]
        )
        card = div_card.OrderDetailsCard(response.div_card)
        assert delivery_date in card.details['Доставка']

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    def test_delivery_by_weekday(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2026
        """
        alice('помоги купить на беру')
        response = alice('детское пюре')
        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons(util.Buttons.CHECKOUT))
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

        # берем интервал с уникальным днем доставки
        options = util.get_phrase_options(response.text)
        by_weekdays = collections.defaultdict(list)
        for opt in options:
            util.assert_with_regexp(opt.value, util.Delivery.DATETIME_REGEXP)
            weekday = re.search(util.WEEKDAY_REGEXP, opt.value).group()
            date = re.search(util.DATE_REGEXP, opt.value).group()
            by_weekdays[weekday].append(date)
        unique_weekday = next((wd for wd in by_weekdays if len(by_weekdays[wd]) == 1), None)
        if not unique_weekday:
            # такое может произойти, что нет уникального дня
            return
        delivery_date = by_weekdays[unique_weekday][0]

        response = alice(unique_weekday)
        card = div_card.OrderDetailsCard(response.div_card)
        assert delivery_date in card.details['Доставка']

        response = alice('Хватит')
        assert response.intent == intent.MarketCancel
