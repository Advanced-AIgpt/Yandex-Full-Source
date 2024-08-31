import alice.megamind.protos.common.device_state_pb2 as device_state_pb2
import alice.megamind.protos.scenarios.directives_pb2 as directives_pb2

from ..alice import ExecutionSuspended, EvoError
from ..device_state import ScreenEnum


class BaseMixin(object):
    def _update_device_state(self):
        pass

    def __getattr__(self, key):
        raise EvoError(
            f'{self.__class__.__name__} has no client directive \'{key}\'.\n'
            'You can define directive by declaring method here: '
            'arcadia/alice/tests/library/surface/directives.'
        )

    def set_cookies(self, data):
        self._megamind_cookies = data.value


class SkillsDirectives(BaseMixin):
    def open_dialog(self, data):
        self._dialog_id = data.dialog_id
        return self._handle_directives(data.directives)

    def update_dialog_info(self, data):
        pass

    def end_dialog_session(self, data):
        self.reset_session()

    def close_dialog(self, data):
        self._dialog_id = None

    def theremin_play(self, data):
        pass


class ImageDirectives(BaseMixin):
    def start_image_recognizer(self, data):
        pass


class SoundDirectives(BaseMixin):
    def sound_set_level(self, data):
        if 0 <= data.new_level <= 10:
            self._device_state.SoundLevel = data.new_level

    def sound_louder(self, data):
        if self._device_state.SoundLevel < 10:
            self._device_state.SoundLevel += 1

    def sound_quiter(self, data):
        if self._device_state.SoundLevel > 0:
            self._device_state.SoundLevel -= 1

    def sound_mute(self, data):
        self._device_state.SoundMuted = True

    def sound_unmute(self, data):
        self._device_state.SoundMuted = False


class PlayerDirectives(BaseMixin):
    def player_rewind(self, data):
        pass

    def player_next_track(self, data):
        pass

    def player_previous_track(self, data):
        pass

    def player_pause(self, data):
        pass

    def player_continue(self, data):
        pass

    def player_like(self, data):
        pass

    def player_dislike(self, data):
        pass

    def player_shuffle(self, data):
        pass

    def player_order(self, data):
        pass

    def player_replay(self, data):
        pass

    def player_repeat(self, data):
        pass


AudioRewindType = directives_pb2.TAudioRewindDirective.EType
AudioPlayerState = device_state_pb2.TDeviceState.TAudioPlayer.TPlayerState


class ThinPlayerDirectives(BaseMixin):
    def _update_device_state(self):
        super()._update_device_state()

        audio_player = self._device_state.AudioPlayer
        if audio_player.PlayerState == AudioPlayerState.Playing:
            position = self.alice_time_ms - int(audio_player.LastPlayTimestamp)
            position += self._device_state.extra.AudioPlayer.rewind_ms
            if position > audio_player.DurationMs:
                position = audio_player.DurationMs
            audio_player.PlayedMs += (position - audio_player.OffsetMs)
            audio_player.OffsetMs = position

    def audio_play(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.extra.AudioPlayer.callbacks = data.callbacks
        self._device_state.extra.AudioPlayer.rewind_ms = 0
        audio_player = self._device_state.AudioPlayer

        audio_player.PlayerState = AudioPlayerState.Idle
        audio_player.LastPlayTimestamp = self.alice_time_ms
        audio_player.LastStopTimestamp = 0

        audio_player.PlayedMs = 0
        audio_player.DurationMs = 0
        audio_player.OffsetMs = data.stream.offset_ms

        audio_player.CurrentlyPlaying.StreamId = data.stream.id
        audio_player.CurrentlyPlaying.Title = data.metadata.title
        audio_player.CurrentlyPlaying.SubTitle = data.metadata.subtitle
        audio_player.CurrentlyPlaying.LastPlayTimestamp = audio_player.LastPlayTimestamp
        audio_player.ScenarioMeta.clear()
        audio_player.ScenarioMeta.update(data.scenario_meta.dict())

        if hasattr(self._device_state.extra.AudioPlayer.callbacks, 'on_started'):
            self._request(event=self._device_state.extra.AudioPlayer.callbacks.on_started)
        audio_player.PlayerState = AudioPlayerState.Playing
        audio_player.DurationMs = 125000
        self._device_state.Video.CurrentScreen = ScreenEnum.music_player
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')

        if data.set_pause:
            self.audio_stop(data=None)
            return

        yield ExecutionSuspended(till=lambda: audio_player.OffsetMs == audio_player.DurationMs)
        if hasattr(self._device_state.extra.AudioPlayer.callbacks, 'on_finished'):
            self._request(event=self._device_state.extra.AudioPlayer.callbacks.on_finished)
        audio_player.PlayerState = AudioPlayerState.Finished

    def audio_stop(self, data):
        audio_player = self._device_state.AudioPlayer
        if audio_player.PlayerState == AudioPlayerState.Playing:
            callbacks = self._device_state.extra.AudioPlayer.callbacks
            if callbacks:
                self._request(event=callbacks.on_stopped)
            audio_player.PlayerState = AudioPlayerState.Stopped
            audio_player.LastStopTimestamp = self.alice_time_ms

    def clear_queue(self, data):
        self.audio_stop(data)
        self._response_queue.clear()

    def audio_player_rewind(self, data):
        if not self._device_state.AudioPlayer.LastPlayTimestamp:
            return

        audio_player = self._device_state.AudioPlayer
        offset_rewind_ms = audio_player.OffsetMs
        if data.type == AudioRewindType.Name(AudioRewindType.Forward):
            offset_rewind_ms += data.amount_ms
        elif data.type == AudioRewindType.Name(AudioRewindType.Backward):
            offset_rewind_ms -= data.amount_ms
        elif data.type == AudioRewindType.Name(AudioRewindType.Absolute):
            offset_rewind_ms = data.amount_ms

        if offset_rewind_ms < 0:
            offset_rewind_ms = 0
        elif offset_rewind_ms > audio_player.DurationMs:
            offset_rewind_ms = audio_player.DurationMs

        self._device_state.extra.AudioPlayer.rewind_ms += offset_rewind_ms - audio_player.OffsetMs
        audio_player.OffsetMs = offset_rewind_ms

    def multiroom_semantic_frame(self, data):
        pass
