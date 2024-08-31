import json


class MarketGalleryAnalyticsInfo(object):
    class _GalleryItem:
        def __init__(self, data):
            self._data = data

        @property
        def type(self):
            return self._data['type']

        @property
        def model_id(self):
            return self._data['id']

        @property
        def rating_icon_url(self):
            if 'rating' in self._data:
                return self._data['rating'].get('icon_url')

        def has_voice_purchase(self):
            return self._data.get('voice_purchase', False)

    def __init__(self, response):
        # На данный момент инф-я для создания галереи дублируется в слотах.
        # Удобно использовать её в качестве meta информации о галерее для тестов.
        self._data = json.loads(response.slots['result'].string)

    def __iter__(self):
        for item in self._data['models']:
            yield self._GalleryItem(item)

    def __getitem__(self, index):
        return self._GalleryItem(self._data['models'][index])

    def __len__(self):
        return len(self._data['models'])

    def find_item_idx(self, has_rating=None, type_=None, has_voice_purchase=None):
        """
        Возвращает первый подходящий по фильтрам элемент галлереи.
        Значение фильтра None означает, что по нему фильтровать не нужно.
        """
        for i, item in enumerate(self):
            if type_ is not None and item.type != type_:
                continue
            if has_rating is not None and (item.rating_icon_url is not None) != has_rating:
                continue
            if has_voice_purchase is not None and item.has_voice_purchase() != has_voice_purchase:
                continue
            return i
