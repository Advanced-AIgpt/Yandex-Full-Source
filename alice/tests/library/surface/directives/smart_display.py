from . import mixin


class DirectivesMixin(mixin.BaseMixin):
    def add_card(self, data):
        pass

    def rotate_cards(self, data):
        pass

    def show_view(self, data):
        pass

    def hide_view(self, data):
        pass

    def update_space_actions(self, data):
        pass

    def set_main_screen(self, data):
        pass

    def set_upper_shutter(self, data):
        pass

    def do_not_disturb_on(self, data):
        pass

    def do_not_disturb_off(self, data):
        pass

    def start_video_call_login_directive(self, data):
        pass

    def start_video_call_directive(self, data):
        pass

    def accept_video_call_directive(self, data):
        pass

    def discard_video_call_directive(self, data):
        pass

    def web_view_media_session_play(self, data):
        pass

    def web_view_media_session_pause(self, data):
        pass
