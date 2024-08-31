from . import mixin
from . import navi


class DirectivesMixin(
    mixin.SkillsDirectives,
):
    def open_uri(self, data):
        if 'play=1' in data.uri:
            self._device_state.Music.Player.Pause = False
        elif data.uri == 'yandexauto://sound?action=volume_up':
            mixin.SoundDirectives.sound_louder(self, data)
        elif data.uri == 'yandexauto://sound?action=volume_down':
            mixin.SoundDirectives.sound_quiter(self, data)
        elif data.uri == 'yandexauto://sound?action=mute':
            mixin.SoundDirectives.sound_mute(self, data)
        elif data.uri == 'yandexauto://sound?action=unmute':
            mixin.SoundDirectives.sound_unmute(self, data)
        elif data.uri.startswith('yandexnavi'):
            navi.DirectivesMixin.open_uri(self, data)


class OldDirectivesMixin(mixin.BaseMixin):
    def car(self, data):
        if data.intent == 'volume_up':
            mixin.SoundDirectives.sound_louder(self, data)
        elif data.intent == 'volume_down':
            mixin.SoundDirectives.sound_quiter(self, data)

    def yandexnavi(self, data):
        pass
