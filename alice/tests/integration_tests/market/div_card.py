import re

from alice.tests.library.vins_response import DivWrapper, DivIterableWrapper
from alice.tests.library.vins_response.div_card.div_card import (
    DivButtons,
    DivTable,
    DivContainer,
)
from cached_property import cached_property

from market import util


_RATING_URL_REGEXP = r'http://avatars.mds.yandex.net/get-mpic/\d+/rating-[1-5]-[05]/1'


class MarketGallery(DivIterableWrapper):
    """
    Используется для галереи в сценариях покупки и "сколько стоит".
    Потенциально это галереи с разной вёрсткой, но во многом они похожи.
    При необходимости нужно будет создать отдельные классы под разные галереи.
    """
    class _Item(DivWrapper):
        @property
        def picture_url(self):
            return self.data.image.image_url

        @property
        def price_text(self):
            return self.data.table.first.text

        @property
        def price(self):
            return util.Price.from_str(self.price_text)

        @property
        def title(self):
            return self.data[3].text

        def has_voice_purchase_mark(self):
            mark_element = self.data.first
            return 'Можно купить через Алису' in mark_element.title

    def __init__(self, data):
        assert data.gallery, 'no gallery in response'
        super().__init__(data.gallery)

    @property
    def market_url(self):
        if self.data.tail:
            return self.data.tail.action_url

    @property
    def market_url_caption(self):
        if self.data.tail:
            return self.data.tail.text

    @property
    def market_url_icon(self):
        if self.data.tail:
            return self.data.tail.icon_url


class MarketExtendedGallery(DivIterableWrapper):
    class _Item(DivWrapper):
        @cached_property
        def raw_str(self):
            return str(self.data.raw)

        @property
        def picture_url(self):
            return self.data[1][0].image.image_url

        @property
        def price_text(self):
            return self.data[2].table.first.text

        @property
        def price(self):
            return util.Price.from_str(self.price_text)

        @property
        def title(self):
            return self.data[2][1].text

        def has_voice_purchase_mark(self):
            mark_element = self.data.first
            return 'Можно купить через Алису' in mark_element.title

        @cached_property
        def rating_icon_url(self):
            match = re.search(_RATING_URL_REGEXP, self.raw_str)
            if match:
                return self.raw_str[match.start(): match.end()]

    def __init__(self, data):
        assert data.gallery, 'no gallery in response'
        super().__init__(data.gallery)

    @property
    def market_url(self):
        return self.data.tail.action_url

    @property
    def market_url_caption(self):
        return self.data.tail.text

    @property
    def market_url_icon(self):
        return self.data.tail.icon_url


class MarketProductOffersCard(DivWrapper):
    @property
    def picture_url(self):
        return self.data[1].side_element.image_url

    @property
    def title(self):
        return self.data[1].title

    @property
    def market_price(self):
        return self.data[1]

    @cached_property
    def raw_str(self):
        return str(self.data.raw)

    @cached_property
    def market_buttons(self):
        buttons = {}
        for block in self.data:
            if isinstance(block, DivButtons):
                buttons.update({button.text: button for button in block})
        return buttons

    @cached_property
    def market_offers_button(self):
        for button_text, button in self.market_buttons.items():
            if re.fullmatch(util.Buttons.PRODUCT_OFFERS, button_text):
                return button

    @cached_property
    def offer_prices(self):
        # На карточке только цены офферов обёрнуты в тег <b>. Если это изменится, нужно будет
        # придумать другой способ извлекать цены офферов.
        return [
            util.Price.from_match(match)
            for match in re.finditer(rf'<b>{util.Price.REGEXP}</b>', self.raw_str)
        ]

    @cached_property
    def rating_icon_url(self):
        match = re.search(_RATING_URL_REGEXP, self.raw_str)
        if match:
            return self.raw_str[match.start(): match.end()]

    @cached_property
    def voice_purchase_links(self):
        block = self.data[-2]
        # Предпоследний блок - либо кнопка, либо табличка с урлами
        if isinstance(block, DivTable):
            return {cell.text: cell.action_url for cell in block[0]}


class MarketBlueProductCard(DivWrapper):
    @cached_property
    def raw_str(self):
        return str(self.data.raw)

    @property
    def picture_url(self):
        return self.data[0].image_url

    @property
    def title(self):
        return self.data[3].text

    @property
    def price_text(self):
        return self.data[2].first.text

    @cached_property
    def price(self):
        return util.Price.from_str(self.price_text)

    @cached_property
    def voice_purchase_links(self):
        return {cell.text: cell.action_url for cell in self.data[-2][0]}


class OrderDetailsCard(DivWrapper):
    @cached_property
    def details(self):
        return {
            block[0].text: block[1].text
            for block in self.data[0]
            if isinstance(block, DivContainer)
        }

    @property
    def price_text(self):
        return self.data[0][-7].title


class ShoppingList(DivIterableWrapper):
    class _Item(DivWrapper):
        @cached_property
        def index(self):
            font_value = self.data[0][0].title
            match = re.match(r'<font .+>(?P<idx>\d+)</font>', font_value)
            if match:
                return int(match['idx'])

        @property
        def value(self):
            return self.data[1].title

        @property
        def market_url(self):
            return self.data[1].action_url

        @property
        def offer_number_label(self):
            if self.market_url:
                return self.data[1].text

        @property
        def picture_url(self):
            if self.market_url:
                return self.data[1].side_element.image_url

    def __init__(self, data):
        # TODO(bas1330) слайсы тут не работают. Не разобрался почему
        items = [data[i] for i in range(1, len(data) - 3)]
        super().__init__(data, items=items)
