import copy
import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest

from market import div_card, util


class TestPalmRecurringPurchase(object):
    owners = ('mllnr',)

    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_search_product_not_on_market(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2009
        """
        response = alice('купить сопливчик как обычно')
        assert response.text == (
            'К сожалению, не нашла подходящих товаров, которые можно купить на Яндекс.Маркете. '
            'Давайте поговорим на другую тему.'
        )
        assert len(response.cards) == 1  # только текст, других карточек нет

        response = alice('как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketNoPhoneAndOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_user_without_phone_and_orders(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2011
        """
        response = alice('купить стиральный порошок как обычно')

        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons('Оформить заказ на Яндекс.Маркете'))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')

        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_ask_phone_phrases())

        response = alice('+7-123-456-78-90')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_phrases(),
            full=True,
        )

        response = alice('город Москва, Льва Толстого 16')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Почта'] == 'no-phone-and-orders2@yandex.ru'
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

        response = alice('как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketNoOrders)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_user_with_phone_and_without_orders(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2012
        """
        response = alice('купить туалетную бумагу как обычно')

        gallery_item = div_card.MarketGallery(response.div_card).first
        response = alice.click(gallery_item.buttons(util.Buttons.BLUE_GALLERY_ORDER_WITH_ALICE))
        response = alice.click(response.div_card.buttons('Оформить заказ на Яндекс.Маркете'))
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('один')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_address_phrases(),
            full=True,
        )

        response = alice('город Москва, Льва Толстого 16')
        util.assert_with_regexps(
            response.text,
            util.CheckoutPhrases.get_ask_delivery_options_phrases(),
        )

        response = alice('один')
        util.assert_with_regexps(response.text, util.CheckoutPhrases.get_confirm_order_phrases())
        card = div_card.OrderDetailsCard(response.div_card)
        assert card.details['Почта'] == 'no-orders1@yandex.ru'
        # у тестового blackbox фальшивый номер
        assert card.details['Телефон'] in ['+7 (906) 600-27-13', '+7 (000) 000-00-13']
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

        response = alice('как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.no_oauth
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_authorization(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1998
        """
        response = alice('Купить автокресло как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_ask_login_mobile_phrases(),
        )
        assert response.button('Авторизация')
        assert response.suggest('Я залогинился')

        response = alice.click(response.suggest('Я залогинился'))
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_ask_login_again_mobile_phrases(),
        )

        alice.login(auth.RobotMarketWithDelivery)

        response = alice.click(response.suggest('Я залогинился'))
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_orders_history_phrases(),
        )
        gallery = div_card.MarketGallery(response.div_card)
        util.assert_orders_gallery(gallery)
        util.assert_suggests(response, [util.Suggests.CANCEL])
        gallery_item = gallery[0]

        # check we render product title as client phrase on gallery item tap
        assert gallery_item.directives
        type_silent_directive = gallery_item.directives[0]
        assert type_silent_directive['name'] == directives.names.TypeTextSilentDirective
        product_title = type_silent_directive['payload']['text']
        # В галерее длинное название может быть сокращено, и тогда в конце будет "..."
        assert product_title.startswith(gallery_item.title.rstrip('.'))

        response = alice.click(gallery_item)
        card = div_card.MarketBlueProductCard(response.div_card)
        util.assert_picture(card.picture_url)
        assert card.title == product_title
        assert 'Общие характеристики:' in card.raw_str
        assert 'Производитель:' in card.raw_str
        assert card.price == gallery_item.price
        assert card.buttons(util.Buttons.CHECKOUT)
        util.assert_voice_purchase_links(card.voice_purchase_links)
        # check card delivery
        has_promo_delivery = 'Бесплатная доставка' in card.raw_str
        if not has_promo_delivery:
            util.assert_has_regexp(card.raw_str, util.Delivery.PRODUCT_CARD_REGEXP)

        util.assert_suggests(response, [util.Suggests.CHECKOUT, util.Suggests.CANCEL])

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_from_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2000
        """
        # goto delivery options step
        response = alice('Купить автокресло как обычно')
        gallery = div_card.MarketGallery(response.div_card)
        gallery_item = gallery[0]
        response = alice.click(gallery_item)
        util.assert_has_suggests(response, [util.Suggests.CHECKOUT])
        response = alice.click(response.suggest(util.Suggests.CHECKOUT))
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
        options = util.get_phrase_options(response.text)
        assert len(options) >= 2
        for i, option in enumerate(options):
            assert option.index == i + 1
            util.assert_with_regexp(option.value, util.Delivery.OPTION_REGEXP)
        # https://st.yandex-team.ru/MALISA-943
        util.assert_suggests(
            response,
            [util.Suggests.CANCEL] + [str(i + 1) for i in range(len(options))],
        )

        # select first delivery option and extract delivery date
        delivery_date_match = re.search(util.DATE_REGEXP, options[0].value)
        assert delivery_date_match, f'can\'t find date in delivery option "{options[0]}"'
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

    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_market_gallery(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2006
        """
        response = alice('Купить зубную пасту как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_goto_market_gallery_phrases(),
        )

        gallery = div_card.MarketGallery(response.div_card)
        util.assert_blue_gallery(gallery, count=10)
        util.MarketSearchUrlValidator(gallery.market_url) \
            .assert_single_param('suggest_text', 'Зубная паста')

        util.assert_suggests(response, [util.Suggests.CANCEL])

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_one_product_in_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2007
        """
        response = alice('Купить смартфон как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_orders_history_phrases(),
        )
        card = div_card.MarketBlueProductCard(response.div_card)
        util.assert_picture(card.picture_url)
        assert card.title
        assert 'Общие характеристики:' in card.raw_str
        assert 'Производитель:' in card.raw_str
        assert card.price.value > 0
        assert card.price.currency == '₽'
        assert card.buttons(util.Buttons.CHECKOUT)
        util.assert_voice_purchase_links(card.voice_purchase_links)
        # check card delivery
        has_promo_delivery = 'Бесплатная доставка' in card.raw_str
        if not has_promo_delivery:
            util.assert_has_regexp(card.raw_str, util.Delivery.PRODUCT_CARD_REGEXP)

        util.assert_suggests(response, [util.Suggests.CANCEL, util.Suggests.CHECKOUT])

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_checkout_from_one_product_in_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2008
        """
        # goto delivery options step
        response = alice('Купить смартфон как обычно')
        product_card = div_card.MarketBlueProductCard(response.div_card)
        assert response.suggest(util.Suggests.CHECKOUT)
        response = alice.click(response.suggest(util.Suggests.CHECKOUT))
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
        options = util.get_phrase_options(response.text)
        assert len(options) >= 2
        for i, option in enumerate(options):
            assert option.index == i + 1
            util.assert_with_regexp(option.value, util.Delivery.OPTION_REGEXP)
        # https://st.yandex-team.ru/MALISA-943
        util.assert_suggests(
            response,
            [util.Suggests.CANCEL] + [str(i + 1) for i in range(len(options))],
        )

        # select first delivery option and extract delivery date
        delivery_date_match = re.search(util.DATE_REGEXP, options[0].value)
        assert delivery_date_match, f'can\'t find date in delivery option "{options[0]}"'
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
        assert card.details['Итого к оплате'] == product_card.price_text
        assert card.price_text == product_card.price_text

    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_voice_empty_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2186
        """
        response = alice('Купить стиральный порошок как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_empty_orders_history_phrases(),
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_voice_checkout_from_one_product_in_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2187
        """
        response = alice('Купить смартфон как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_order_history_item_phrases(),
        )
        product = util.RecurringPurchase.try_get_order_history_item(response.text)
        total_price = copy.copy(product.price)

        response = alice('Да')
        item_count = None
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('2')
            item_count = '2'
            total_price.value *= 2
        util.assert_with_regexps(
            response.text, util.RecurringPurchase.get_next_delivery_option_phrases())
        option = util.RecurringPurchase.try_get_next_delivery_option(response.text)

        response = alice('Да')
        util.assert_with_regexps(response.text, util.RecurringPurchase.get_order_details_phrases())
        order_details = util.RecurringPurchase.try_get_order_details(response.text)
        assert order_details['title'] == product.title
        assert order_details['price_text'] == product.price_text
        assert order_details['item_count'] == item_count
        assert order_details['delivery_price'] == 'бесплатно'
        assert order_details['total_price_text'] == str(total_price)
        assert order_details['phone'] == '+7 912 043-00-04'
        assert order_details['delivery_type'] == 'Курьерская доставка'
        assert order_details['address'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert order_details['delivery_option'] == option

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_voice_checkout_from_one_product_in_orders_history_with_additional_question(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2188
        Дополнительно проверяется отказ при подтверждении заказа
        """
        response = alice('Купить смартфон как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_order_history_item_phrases(),
        )
        product = util.RecurringPurchase.try_get_order_history_item(response.text)
        total_price = copy.copy(product.price)

        response = alice('А есть другой?')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_dont_understand_confirmation_phrases(),
        )

        response = alice('Да')
        item_count = None
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('2')
            item_count = '2'
            total_price.value *= 2
        util.assert_with_regexps(
            response.text, util.RecurringPurchase.get_next_delivery_option_phrases())
        option = util.RecurringPurchase.try_get_next_delivery_option(response.text)

        response = alice('Да')
        util.assert_with_regexps(response.text, util.RecurringPurchase.get_order_details_phrases())
        order_details = util.RecurringPurchase.try_get_order_details(response.text)
        assert order_details['title'] == product.title
        assert order_details['price_text'] == product.price_text
        assert order_details['item_count'] == item_count
        assert order_details['delivery_price'] == 'бесплатно'
        assert order_details['total_price_text'] == str(total_price)
        assert order_details['phone'] == '+7 912 043-00-04'
        assert order_details['delivery_type'] == 'Курьерская доставка'
        assert order_details['address'] == 'Россия, Москва, улица Льва Толстого, 16'
        assert order_details['delivery_option'] == option

        response = alice('Нет')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_order_denied_phrases(),
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_deny_poduct_in_orders_history(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2189
        """
        response = alice('Купить смартфон как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_order_history_item_phrases(),
        )

        response = alice('А есть другой?')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_dont_understand_confirmation_phrases(),
        )

        response = alice('Нет')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_order_denied_phrases(),
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_voice_repeat_many_items_history_and_cancel(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2191
        """
        response = alice('Купить автокресло как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_order_history_items_phrases(),
        )
        orders_text = response.text

        response = alice('Повтори')
        assert response.text == orders_text

        response = alice('1')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_start_checkout_phrases(),
        )

        response = alice('Хватит')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_canceled_phrases(),
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-716#611b8fc686d54e2310498f5f')
    @pytest.mark.voice
    @pytest.mark.oauth(auth.RobotMarketWithDelivery)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_voice_repeat_one_item_history_and_order_details_and_cancel(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-2192
        """
        response = alice('Купить смартфон как обычно')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_voice_order_history_item_phrases(),
        )
        orders_text = response.text

        response = alice('Повтори')
        assert response.text == orders_text

        response = alice('Да')
        if util.fits_regexps(response.text, util.CheckoutPhrases.get_ask_item_count_phrases()):
            response = alice('1')
        util.assert_with_regexps(
            response.text, util.RecurringPurchase.get_next_delivery_option_phrases())

        response = alice('Да')
        util.assert_with_regexps(response.text, util.RecurringPurchase.get_order_details_phrases())
        order_details_text = response.text

        response = alice('Повтори')
        assert response.text == order_details_text

        response = alice('Хватит')
        util.assert_with_regexps(
            response.text,
            util.RecurringPurchase.get_canceled_phrases(),
        )

        response = alice('Как дела?')
        assert response.intent == intent.HowAreYou
