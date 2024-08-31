import json
import re
from urllib.parse import urlparse, parse_qs

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from market import div_card


class _TestHowMuch(object):

    owners = ('mllnr',)

    _avg_price_regexps = [
        re.compile(r'Посмотрела на Маркете\. Это стоит около \d+ (рубля|рублей)\. Вот\.$'),
        re.compile(r'Заглянула на Маркет\. В среднем — \d+ (рубль|рубля|рублей)\. Смотрите\.$'),
        re.compile(r'\d+ (рубль|рубля|рублей)\. Это средняя цена по данным Яндекс Маркета\.$'),
        re.compile(r'Примерно \d+ (рубль|рубля|рублей)\. Нашла на Маркете\. Смотрите\.$'),
    ]
    _avg_price_no_gallery_regexps = [
        re.compile(r'Посмотрела на Маркете\. Это стоит около \d+ (рубля|рублей)\.$'),
        re.compile(r'Заглянула на Маркет\. В среднем — около \d+ (рубля|рублей)\.$'),
        re.compile(r'\d+ (рубль|рубля|рублей)\. Это средняя цена по данным Яндекс Маркета\.$'),
        re.compile(r'Примерно \d+ (рубль|рубля|рублей)\. Нашла на Маркете\.$'),
    ]
    _market_url_caption = [
        re.compile(r'Посмотреть \d+ (вариант|варианта|вариантов) на Маркете$'),
    ]
    _uuid_regexp = re.compile(r'[a-z0-9]{32}$')

    @classmethod
    def _assert_avg_price_no_card_text_valid(cls, text):
        cls.__check_with_regexps(cls._avg_price_no_card_text_regexps, text)

    @classmethod
    def _assert_avg_price_no_card_voice_valid(cls, voice):
        cls.__check_with_regexps(cls._avg_price_no_card_voice_regexps, voice)

    @staticmethod
    def _check_with_regexps(regexps, text):
        for regexp in regexps:
            if re.match(regexp, text):
                return
        assert False, f'got unexpected phrase value "{text}"'

    @staticmethod
    def _get_products_data(response):
        """
        Данные для рендеренга галереи с древних времён хранятся внутри слота popular_good.
        Чтоб найти индекс элемента галереи с оффером беру, можно поискать его в слоте.
        """
        return json.loads(response.slots['popular_good']['string'])['results']

    @classmethod
    def _get_beru_gallery_item(cls, response):
        """
        Данные для рендеренга галереи с древних времён хранятся внутри слота popular_good.
        Чтоб найти индекс элемента галереи с оффером беру, можно поискать его в слоте.
        """
        gallery = div_card.MarketGallery(response.div_card)
        for i, product in enumerate(cls._get_products_data(response)):
            if product.get('voice_purchase'):
                return gallery[i]
        assert False, 'expected at least one product with beru offer'


class TestOnlyVinsActivated(object):

    owners = ('mllnr',)

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.station,
        surface.watch,
    ])
    def test_ya_search_unsupported(self, alice):
        response = alice('сколько стоит противогаз для морской свинки')
        assert response.intent in [intent.HowMuch, intent.ProtocolHowMuch]
        assert response.text in (
            'Извините, у меня нет хорошего ответа.',
            'У меня нет ответа на такой запрос.',
            'Я пока не умею отвечать на такие запросы.',
            'Простите, я не знаю что ответить.',
            'Я не могу на это ответить.',
        )
        assert not response.directive

    @staticmethod
    def _assert_open_uri_search_text_equals(expected, directive):
        assert directive.name == directives.names.OpenUriDirective
        uri = urlparse(directive['payload']['uri'])
        cgi = parse_qs(uri.query)
        assert len(cgi.get('text', [])) == 1, 'expected 1 "text" param'
        assert expected == cgi['text'][0], 'yandex search queries are different'

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test_ya_search_supported(self, alice):
        response = alice('алиса диски 16 красивые будут сколько мне стоить')
        assert response.intent in [intent.HowMuch, intent.ProtocolHowMuch]
        assert response.text in (
            'Ищу ответ',
            'Найдётся всё!',
            'Ищу в Яндексе',
            'Сейчас найду',
            'Сейчас найдём',
            'Одну секунду...',
            'Открываю поиск',
            'Ищу для вас ответ',
            'Давайте поищем',
        )
        self._assert_open_uri_search_text_equals('алиса короче диски стоят 16', response.directive)

    @pytest.mark.parametrize('surface', [
        surface.searchapp,
    ])
    def test_disable_ellipsis(self, alice):
        response = alice('алиса диски 16 красивые будут сколько мне стоить')
        assert response.intent in [intent.HowMuch, intent.ProtocolHowMuch]
        response = alice('а телефон')
        assert response.intent not in (intent.HowMuch, intent.HowMuchEllipsis)


@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments(
    'market_how_much_ext_gallery',
    f'mm_disable_protocol_scenario={scenario.MarketHowMuch}',
)
class TestProductOffersCard(_TestHowMuch):

    @pytest.mark.experiments('market_disable')
    def test_voice_purchase_disable(self, alice):
        response = alice('сколько стоит телефон')
        assert response.intent == intent.HowMuch

        # check gallery item with beru hasn't voice purchase mark
        beru_item = self._get_beru_gallery_item(response)
        assert not beru_item.has_voice_purchase_mark()

        # TODO
        # product_card_open_action = beru_item.get_action_url()
        # get response by product_card_open_action
        # check there is no 'order with alice' button in response
        # check there is no sku slot in form
        # check there is no suggest 'order with alice

    def test_voice_purchase(self, alice):
        response = alice('сколько стоит телефон')
        assert response.intent == intent.HowMuch

        # check gallery item with beru hasn't voice purchase mark
        beru_item = self._get_beru_gallery_item(response)
        assert beru_item.has_voice_purchase_mark()

        # TODO
        # product_card_open_action = beru_item.get_action_url()
        # get response by product_card_open_action
        # check there is an 'order with alice' button in response


class TestHowMuch(_TestHowMuch):
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_analytics_info(self, alice):
        response = alice('сколько стоит айфон')
        assert response.intent == intent.ProtocolHowMuch
        assert response.scenario_analytics_info.product_scenario == 'how_much'
        assert response.scenario == scenario.MarketHowMuch
