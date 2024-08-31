from . import mixin
from ..device_state import ScreenEnum


class DirectivesMixin(
    mixin.ThinPlayerDirectives,
    mixin.SkillsDirectives,
    mixin.PlayerDirectives,
    mixin.SoundDirectives,
):
    def open_uri(self, data):
        if 'live-tv' in data.uri:
            self._device_state.Video.Player.Pause = False

    def show_gallery(self, data):
        pass

    def show_description(self, data):
        pass

    def show_login(self, data):
        pass

    def show_plus_purchase(self, data):
        pass

    def show_plus_promo(self, data):
        pass

    def music_play(self, data):
        self.player_pause(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.music_player

        self._device_state.Music.Player.Pause = False
        self._device_state.Music.SessionId = data.session_id
        if data.get('first_track_id'):
            self._device_state.Music.CurrentlyPlaying.TrackId = data.first_track_id
        self._device_state.Music.LastPlayTimestamp = self.alice_time_ms

    def set_smarttv_categories(self, data):
        pass

    def tv_set_carousel(self, data):
        pass

    def tv_set_carousels(self, data):
        pass

    def send_android_app_intent(self, data):
        pass

    def tv_open_details_screen(self, data):
        pass

    def tv_open_search_screen(self, data):
        pass

    def video_play(self, data):
        pass

    def tv_open_person_screen(self, data):
        pass

    def tv_open_collection_screen(self, data):
        pass

    def alarm_set_max_level(self, data):
        pass

    def screen_on(self, data):
        pass

    def screen_off(self, data):
        pass
