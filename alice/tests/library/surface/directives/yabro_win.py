from . import mixin


class DirectivesMixin(
    mixin.SkillsDirectives,
):
    def open_uri(self, data):
        pass

    def type(self, data):
        return self._request(request=data.text)

    def type_silent(self, data):
        pass

    def show_tv_gallery(self, data):
        pass

    def power_off(self, data):
        pass

    def open_soft(self, data):
        pass
