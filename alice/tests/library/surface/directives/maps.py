from . import mixin


class DirectivesMixin(
    mixin.BaseMixin,
):
    def open_uri(self, data):
        pass

    def type_silent(self, data):
        pass
