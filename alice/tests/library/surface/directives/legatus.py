from . import mixin


class DirectivesMixin(mixin.BaseMixin):
    def web_os_launch_app_directive(self, data):
        pass

    def web_os_show_gallery_directive(self, data):
        pass

    def end_dialog_session(self, data):
        pass

    def close_dialog(self, data):
        pass
