from alice.tests.library.vins_response import DivWrapper, DivIterableWrapper, DivSeparatorWrapper
from cached_property import cached_property


_cursor_url = 'https://avatars.mds.yandex.net/get-bass/469429/orgs_c3d52c9b5c09dd0a78d814961dddf04c0c7d52db41048ca509137e6c302aae7e.png/orig'


class PoiGallery(DivIterableWrapper):
    class _Item(DivWrapper):
        @property
        def map(self):
            log_id = 'poi_gallery_card__map_image'
            assert self.data[0][0].log_id == log_id, f'first item in the block must be {log_id}'
            return self.data[0][0]

        @property
        def name(self):
            return self.data[2].title

        @property
        def address(self):
            return self.data[2].text

        @property
        def descripsion(self):
            return self.data[3].text

        @cached_property
        def reviews(self):
            if self.data[5].log_id == 'poi_gallery__reviews':
                return self.data[5]

        @property
        def reviews_stars(self):
            for i in range(5):
                yield self.reviews[0][i]

        @property
        def reviews_text(self):
            return self.reviews[0][5].text

        @property
        def open_hours(self):
            index = 6 if self.reviews else 5
            return self.data[index][0][0].text[22:].rstrip('</font>')

        @property
        def has_distance_cursor(self):
            index = 6 if self.reviews else 5
            return self.data[index][0][1].image_url == _cursor_url

        @property
        def distance(self):
            index = 6 if self.reviews else 5
            return self.data[index][0][2].text[22:].rstrip('</font>')

        def button(self, name):
            index = 7 if self.reviews else 6
            for button in self.data[index][1]:
                if name in button.text:
                    return button

    def __init__(self, data):
        super().__init__(data.gallery)


class PoiCard(object):
    class ImageGallery(DivIterableWrapper):
        def __init__(self, data):
            assert data.gallery, 'first div card must be gallery'
            super().__init__(data.gallery)

        @property
        def map(self):
            log_id = 'photo_gallery_card__route'
            assert self.data[0].log_id == log_id, f'first item in the gallery must be {log_id}'
            return self.data[0]

        @property
        def map_image_url(self):
            return self.map[0].image_url

        def fotos(self):
            log_id = 'photo_gallery_card__whole_card'
            for i in range(1, len(self.data[0])):
                assert self.data[0][i].log_id == log_id, f'other items in the gallery must be {log_id}'
                yield self.data[0][i]

    class Description(DivSeparatorWrapper):
        def __init__(self, data):
            log_id = 'whole_card__object_catalog'
            assert data.log_id == log_id, f'second div card must be {log_id}, but {data.log_id}'
            super().__init__(data)

        @property
        def name(self):
            return self[0].title[22:].rstrip('</font>')

        @property
        def has_distance_cursor(self):
            return self[1][0][0].image_url == _cursor_url

        @property
        def address(self):
            return self[1][0][1].text[22:].rstrip('</font>')

        @property
        def distance(self):
            return self[1][0][2].text

        @property
        def descripsion(self):
            return self[3].text

        @cached_property
        def reviews(self):
            log_id = 'poi_gallery__reviews'
            assert self[4].log_id == log_id, f'reviews block must be {log_id}, but {self.data[9].log_id}'
            return self[4]

        @property
        def reviews_stars(self):
            for i in range(5):
                yield self.reviews[0][i]

        @property
        def reviews_text(self):
            return self.reviews[0][5].text

        @property
        def open_hours(self):
            return self.reviews[0][6].text[22:].rstrip('</font>')

        def button(self, name):
            for button in self[5][1]:
                if name in button.text:
                    return button

    def __init__(self, div_cards):
        assert div_cards, 'must be at least 2 div cards'
        self.images = PoiCard.ImageGallery(div_cards[0])
        self.info = PoiCard.Description(div_cards[1])
