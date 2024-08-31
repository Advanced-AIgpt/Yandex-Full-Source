from . import mixin


class _PlayerDirectives(mixin.PlayerDirectives):
    def player_pause(self, data):
        self._device_state.Music.Player.Pause = True


class DirectivesMixin(
    _PlayerDirectives,
    mixin.SkillsDirectives,
):
    def open_uri(self, data):
        if 'music' in data.uri and 'play' in data.uri:
            self._device_state.Music.Player.Pause = False
        elif data.uri == 'yandexnavi://set_setting?name=soundNotifications&value=Alerts':
            mixin.SoundDirectives.sound_mute(self, data)
        elif data.uri == 'yandexnavi://set_setting?name=soundNotifications&value=All':
            mixin.SoundDirectives.sound_unmute(self, data)
        elif data.uri == 'yandexnavi://clear_route':
            self._device_state.Navigator.Clear()
        elif data.uri.startswith('yandexnavi://build_route_on_map?confirmation=1'):
            self._device_state.Navigator.States.append('waiting_for_route_confirmation')
        elif data.uri.startswith('yandexnavi://external_confirmation?confirmed='):
            self._device_state.Navigator.States.pop()

    def type_silent(self, data):
        pass

    def show_tv_gallery(self, data):
        pass
