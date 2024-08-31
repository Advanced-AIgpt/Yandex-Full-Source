from alice.tests.library.vins_response import DivWrapper, DivIterableWrapper
from cached_property import cached_property


class _Button(DivWrapper):
    @property
    def image(self):
        return self.data[0].image

    @property
    def title(self):
        return self.data[2].text

    @property
    def directives(self):
        return self.data[2].directives


class MoreInfo(DivWrapper):
    @property
    def title(self):
        return self.data.container[1].title

    @cached_property
    def buttons(self):
        return [_Button(_) for _ in self.data.container.gallery]

    def button(self, title):
        for b in self.buttons:
            if title in b.title:
                return b


class Clothes(DivIterableWrapper):
    class _Item(DivIterableWrapper):
        class _Item(DivWrapper):
            @property
            def image_url(self):
                return self.data[0].image_url

            @property
            def price(self):
                return self.data[2].title

            @property
            def name(self):
                return self.data[3].title

            @property
            def shop(self):
                return self.data[5].text

        def __init__(self, data):
            super().__init__(data, data.content.gallery)

    def __init__(self, data):
        super().__init__(data, data.tabs)


class MarketGoods(DivWrapper):
    @property
    def image_url(self):
        return self.data.image.image_url

    @property
    def price(self):
        return self.data[2].title

    @property
    def name(self):
        return self.data[2].text

    @cached_property
    def source(self):
        if self.data[3]._type == 'div-universal-block':
            return self.data[3].text

    @property
    def reviews(self):
        index = 4 if self.source else 3
        return self.data[index].first.text


class MarketGallery(DivIterableWrapper):
    class _Item(DivWrapper):
        @property
        def image_url(self):
            return self.data[1].side_element.image_url

        @property
        def price(self):
            return self.data[1].title

        @property
        def name(self):
            return self.data[1].text

    def __init__(self, data):
        super().__init__(data, data.gallery)


class Market(object):
    def __init__(self, div_cards):
        assert div_cards, 'must be at least 2 div cards'
        self._div_cards = div_cards

    @cached_property
    def exact_goods(self):
        card = self._div_cards[0]
        return MarketGoods(card) if card.log_id == 'image_recognizer' else None

    @cached_property
    def goods_gallery(self):
        index = 1 if self.exact_goods else 0
        return MarketGallery(self._div_cards[index])


class Entity(DivWrapper):
    @property
    def photo(self):
        return self.data.image

    @property
    def name(self):
        return self.data[1].title

    @property
    def description(self):
        return self.data[1].text

    @property
    def source(self):
        return self.data[2].title


class MuseumArtwork(DivWrapper):
    @property
    def photo(self):
        return self.data.image

    @property
    def name(self):
        return self.data[3].title

    @property
    def description(self):
        return self.data[3].text

    @property
    def source(self):
        return self.data[4].text

    @property
    def listen_audio(self):
        return self.data.table


class SimilarArtwork(DivWrapper):
    @property
    def original_photo(self):
        return self.data[0][0].image

    @property
    def similar_photo(self):
        return self.data[0][1].image

    @property
    def title(self):
        return self.data[1].universal.title

    @property
    def source(self):
        return self.data[1].table[0][0].text


class SimilarPeople(DivWrapper):
    @property
    def similar_photo(self):
        return self.data[0][0].image

    @property
    def original_photo(self):
        return self.data[0][1].image

    @property
    def title(self):
        return self.data[1].universal.title

    @property
    def source(self):
        return self.data[1].table[0][0].text


class ScannedDocument(object):
    def __init__(self, div_cards):
        assert len(div_cards) == 3, 'must be 3 div cards'
        self._div_cards = div_cards

    @property
    def scanned_photo(self):
        return self._div_cards[0].container.image

    @cached_property
    def buttons(self):
        return [_Button(_) for _ in self._div_cards[1].container.gallery]

    def button(self, title):
        for b in self.buttons:
            if title in b.title:
                return b
