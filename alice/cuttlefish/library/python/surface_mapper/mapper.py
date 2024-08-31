from .bindings import (
    MapperImpl,
)


class Mapper:
    def __init__(self):
        self._impl = MapperImpl()

    def map(self, app_id: str):
        return self._impl.map(app_id)

    def try_map(self, app_id: str):
        return self._impl.try_map(app_id)
