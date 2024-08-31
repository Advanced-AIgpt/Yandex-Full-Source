from alice.tests.library.vins_response import DivWrapper, DivIterableWrapper


class RouteGallery(DivIterableWrapper):
    class _Item(DivWrapper):
        def __init__(self, data):
            log_id = 'show_route_gallery_card'
            assert data.log_id == log_id, f'Expect {log_id}, but {data.log_id}'
            super().__init__(data)

        @property
        def map(self):
            return self.data.image

        @property
        def icon(self):
            return self.data.table[0][0]

        @property
        def distance(self):
            return self.data.table[0][1].text

        @property
        def footer(self):
            return self.data.footer

    def __init__(self, data):
        assert data.gallery, 'Expect gallery in div_card'
        super().__init__(data.gallery)
