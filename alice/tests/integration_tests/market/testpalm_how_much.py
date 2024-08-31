import json
import re
from urllib.parse import urlparse, parse_qs

import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest

from market import div_card, util


class TestPalmHowMuch(object):
    owners = ('mllnr',)

    _model_url_regexp = re.compile(r'https://m.market.yandex.ru/product--[\w-]+/\d+\?')
    _offer_url_regexp = re.compile(r'https://m.market.yandex.ru/offer/[\d\w-]+\?')

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

    @pytest.mark.parametrize('surface', [surface.watch])
    @pytest.mark.parametrize('next_command, intent_name', [
        ('купи', intent.ProhibitionError),
        ('перейди на сайт', intent.ProhibitionError),
        ('подробнее', intent.GeneralConversation),
    ])
    def test_watch_how_much_sixth_iphone(self, alice, next_command, intent_name):
        """
        https://testpalm.yandex-team.ru/testcase/alice-19
        """
        response = alice('сколько стоит шестой айфон')
        assert response.intent == intent.ProtocolHowMuch
        assert any(re.search(pattern, response.text) for pattern in [
            r'.+\. Это средняя цена по данным Яндекс Маркета\.',
            r'Заглянула на Маркет\. В среднем — около .+\.',
            r'Посмотрела на Маркете\. Это стоит около .+\.',
            r'Примерно .+\. Нашла на Маркете\.',
        ])

        response = alice(next_command)
        assert response.intent == intent_name

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.station,
        surface.watch,
    ])
    def test_avg_price_screenless(self, alice):
        """
        https://testpalm2.yandex-team.ru/alice/testsuite/5ef605b67440c319edcd1bb4?testcase=2702
        """
        response = alice('стоимость велосипеда')
        assert response.intent == intent.ProtocolHowMuch
        self._check_with_regexps(self._avg_price_no_gallery_regexps, response.text)
        self._check_with_regexps(self._avg_price_no_gallery_regexps, response.output_speech_text)

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    def test_gallery(self, alice):
        """
        common for:
        https://testpalm.yandex-team.ru/testcase/alice-1202
        https://testpalm.yandex-team.ru/testcase/alice-1203
        """
        response = alice('зимние шины какая цена')

        assert response.intent == intent.ProtocolHowMuch

        self._check_with_regexps(self._avg_price_regexps, response.text)
        self._check_with_regexps(self._avg_price_regexps, response.output_speech_text)

        gallery = div_card.MarketGallery(response.div_card)
        assert len(gallery) > 0
        for item in gallery:
            util.assert_picture(item.picture_url)
            assert item.title
            assert item.price.value > 0
            assert item.price.currency == '₽'
            assert item.action_url
            url_cgi = parse_qs(urlparse(item.action_url).query)
            assert re.match(self._uuid_regexp, url_cgi['uuid'][0])

        assert gallery.market_url_icon == util.MARKET_ICON_URL
        self._check_with_regexps(self._market_url_caption, gallery.market_url_caption)
        parsed_market_url = urlparse(gallery.market_url)
        assert (
            parsed_market_url.path.startswith('/catalog--')
            or parsed_market_url.path.startswith('/search')
        )
        cgi = parse_qs(parsed_market_url.query)
        assert cgi['text'][0] == 'зимние шины'
        assert re.match(self._uuid_regexp, cgi['uuid'][0])

    @pytest.mark.parametrize('surface', [surface.yabro_win])
    def test_gallery_urls_on_desktop(self, alice):
        """
        common for:
        https://testpalm.yandex-team.ru/testcase/alice-1202
        https://testpalm.yandex-team.ru/testcase/alice-1203
        """
        response = alice('стоимость велосипеда')

        assert response.intent == intent.ProtocolHowMuch

        gallery = div_card.MarketGallery(response.div_card)
        assert len(gallery) > 0
        assert gallery.first.action_url.startswith('https://market.yandex.ru/')
        assert gallery.market_url.startswith('https://market.yandex.ru/')

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    def test_gallery_urls_on_touch(self, alice):
        """
        common for:
        https://testpalm.yandex-team.ru/testcase/alice-1202
        https://testpalm.yandex-team.ru/testcase/alice-1203
        """
        response = alice('стоимость велосипеда')

        assert response.intent == intent.ProtocolHowMuch

        gallery = div_card.MarketGallery(response.div_card)
        assert len(gallery) > 0
        assert gallery.first.action_url.startswith('https://m.market.yandex.ru/')
        assert gallery.market_url.startswith('https://m.market.yandex.ru/')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_gallery_model(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1202
        """
        response = alice('сколько стоит электронная книга')
        assert response.intent == intent.ProtocolHowMuch

        for item in div_card.MarketGallery(response.div_card):
            if re.match(self._model_url_regexp, item.action_url):
                break
        else:
            assert False, 'expected at least one gallery item with model-like url'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_gallery_offer(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-1203
        """
        response = alice('сколько стоит кайт')
        assert response.intent == intent.ProtocolHowMuch

        for item in div_card.MarketGallery(response.div_card):
            if re.match(self._offer_url_regexp, item.action_url):
                break
        else:
            assert False, 'expected at least one gallery item with offer-like url'

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_adult(self, alice):
        """
        https://testpalm.yandex-team.ru/testcase/alice-38
        """

        response = alice('Сколько стоит пистолет')
        assert response.intent == intent.HowMuch
        assert response.text in {
            'Извините, у меня нет хорошего ответа.',
            'У меня нет ответа на такой запрос.',
            'Я пока не умею отвечать на такие запросы.',
            'Простите, я не знаю что ответить.',
            'Я не могу на это ответить.',
        }

        response = alice('Купи')
        assert response.intent == intent.ProhibitionError
        assert response.text in {
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        }
