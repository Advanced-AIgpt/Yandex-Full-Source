import json
import uuid

import alice.megamind.protos.scenarios.directives_pb2 as directives_pb2
import google.protobuf.json_format as json_format
import requests
from google.protobuf.struct_pb2 import Struct
from retry import retry

from . import mixin
from ..device_state import ScreenEnum


PlayerRewindType = directives_pb2.TPlayerRewindDirective.EType


@retry(tries=3, delay=0.5)
def _load_view_state(url):
    response = requests.get(url, params=dict(waitall='da', evo_request=1, disable_renderer=1))
    assert response.status_code == 200, f'Device state request {response.url} failed with: {response.status_code}'

    view_state = response.json().get('evo_device_state')
    assert view_state, f'Empty device state for mordovia directive emulation {response.url}'
    return json_format.ParseDict(view_state, Struct())


class _TimerDirectives(mixin.BaseMixin):
    def _update_device_state(self):
        super()._update_device_state()

        now_ts = int(self._alice_time.timestamp())
        for timer in self._device_state.Timers.ActiveTimers:
            if not timer.Paused:
                remaining = timer.StartTimestamp + timer.Duration - now_ts  # incorrect if timer is paused TODO(danibw)
                timer.Remaining = remaining if remaining > 0 else 0
                timer.CurrentlyPlaying = (timer.Remaining == 0)

    def set_timer(self, data):
        now = self._alice_time
        timer = self._device_state.Timers.ActiveTimers.add(
            TimerId=str(uuid.uuid4()),
            StartTimestamp=int(now.timestamp()),
            Duration=data.duration,
            Remaining=data.duration,
            CurrentlyPlaying=False,
            Paused=False,
        )

        for directive in data.get('directives', []):
            json_format.ParseDict(directive.dict(), timer.Directives.add(), ignore_unknown_fields=False)

    def pause_timer(self, data):
        for timer in self._device_state.Timers.ActiveTimers:
            if timer.TimerId == data.timer_id:
                timer.Paused = True

    def resume_timer(self, data):
        for timer in self._device_state.Timers.ActiveTimers:
            if timer.TimerId == data.timer_id:
                timer.Paused = False

    def timer_stop_playing(self, data):
        self.cancel_timer(data)

    def cancel_timer(self, data):
        index = None
        for i, timer in enumerate(self._device_state.Timers.ActiveTimers):
            if timer.TimerId == data.timer_id:
                index = i
        del self._device_state.Timers.ActiveTimers[index]


class _PlayerDirectives(mixin.PlayerDirectives):
    def _update_device_state(self):
        super()._update_device_state()

        current_screen = self._device_state.Video.CurrentScreen
        if current_screen == ScreenEnum.video_player:
            progress = self._device_state.Video.CurrentlyPlaying.Progress
            last_play_timestamp = int(self._device_state.Video.LastPlayTimestamp)
            progress.Played = self._device_state.extra.Video.start_at or 0
            progress.Played += (self.alice_time_ms - last_play_timestamp)/1000
            if progress.Played > progress.Duration:
                progress.Played = progress.Duration

    def player_continue(self, data):
        if data.player == 'music':
            self._device_state.Music.Player.Pause = False
            self._device_state.Music.Player.ClearField('Timestamp')
            self._device_state.Video.CurrentScreen = ScreenEnum.music_player
            self._device_state.Video.ClearField('ViewState')
            self._device_state.Video.ClearField('ScreenState')

    def player_pause(self, data):
        current_screen = self._device_state.Video.CurrentScreen
        if current_screen == ScreenEnum.radio_player:
            self._device_state.Radio['player']['paused'] = True
        elif current_screen == ScreenEnum.music_player:
            self._device_state.Music.Player.Pause = True
            self._device_state.Music.Player.Timestamp = self.alice_time_ms

    def player_rewind(self, data):
        current_screen = self._device_state.Video.CurrentScreen
        if current_screen == ScreenEnum.video_player:
            progress = self._device_state.Video.CurrentlyPlaying.Progress
            if data.type == PlayerRewindType.Name(PlayerRewindType.Forward):
                progress.Played += data.amount
            elif data.type == PlayerRewindType.Name(PlayerRewindType.Backward):
                progress.Played -= data.amount
            elif data.type == PlayerRewindType.Name(PlayerRewindType.Absolute):
                progress.Played = data.amount


class _SkillsDirectives(mixin.SkillsDirectives):
    def end_dialog_session(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.main
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')

    def close_dialog(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.main
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')


class _MordoviaWebViewDirectives(mixin.BaseMixin):
    def mordovia_show(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.mordovia_webview
        self._device_state.Video.ScreenState.ViewKey = data.view_key
        self._device_state.Video.ViewState.CopyFrom(_load_view_state(data.url))
        self._device_state.Video.ClearField('CurrentlyPlaying')

    def mordovia_command(self, data):
        current_view_key = self._device_state.Video.ScreenState.ViewKey
        assert data.view_key == current_view_key, f'Current view_key ({current_view_key}) and mordovia_command target view_key ({data.view_key}) differs'

        if data.command == 'change_path':
            self._device_state.Video.ViewState.CopyFrom(
                _load_view_state(f'https://hamster.yandex.ru{json.loads(data.meta)["path"]}')
            )


class _NotificationDirectives(mixin.BaseMixin):
    def notify(self, data):
        pass


class _VideoCommandDirectives(mixin.BaseMixin):
    def change_audio(self, data):
        pass

    def change_subtitles(self, data):
        pass

    def show_video_settings(self, data):
        pass


class DirectivesMixin(
    _NotificationDirectives,
    _PlayerDirectives,
    _TimerDirectives,
    _SkillsDirectives,
    _VideoCommandDirectives,
    _MordoviaWebViewDirectives,
    mixin.ThinPlayerDirectives,
    mixin.SoundDirectives,
):
    def alarm_set_sound(self, data):
        sound_alarm_setting = data.get('sound_alarm_setting')
        if sound_alarm_setting:
            # TODO(danibw): remove after r8116892 is in prod
            if hasattr(sound_alarm_setting, 'type'):
                self._device_state.AlarmState.SoundAlarmSetting.Type = sound_alarm_setting.type
            if hasattr(sound_alarm_setting, 'info'):
                json_format.ParseDict(sound_alarm_setting.info.dict(), self._device_state.AlarmState.SoundAlarmSetting.RawInfo)

    def alarm_reset_sound(self, data):
        self._device_state.AlarmState.ClearField('SoundAlarmSetting')

    def alarms_update(self, data):
        self._device_state.extra.AlarmState.timezone = self._application.Timezone
        self._device_state.AlarmState.ICalendar = data.state
        self._device_state.AlarmState.CurrentlyPlaying = False
        self._device_state.AlarmsState = data.state

    def alarm_stop(self, data):
        self._device_state.AlarmState.CurrentlyPlaying = False

    def go_home(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.main
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')

    def go_forward(self, data):
        pass

    def go_backward(self, data):
        pass

    def go_down(self, data):
        pass

    def go_up(self, data):
        pass

    def go_top(self, data):
        pass

    def go_to_the_end(self, alice):
        pass

    def go_to_the_beginning(self, alice):
        pass

    def music_play(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.music_player
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')
        self._device_state.Music.LastPlayTimestamp = self.alice_time_ms

        self._device_state.Music.Player.Pause = False
        self._device_state.Music.SessionId = data.session_id

        # '123456' - is a fake track_id for the case we don't have one...
        first_track_id = data.get('first_track_id', '123456')
        currently_playing = self._device_state.Music.CurrentlyPlaying
        currently_playing.TrackId = first_track_id
        currently_playing.LastPlayTimestamp = self._device_state.Music.LastPlayTimestamp

        # Fake track_info which in real life client retrieves from music backend websocket API
        track_info = currently_playing.RawTrackInfo
        track_info['id'] = first_track_id
        track_info['durationMs'] = 150000
        track_info['type'] = 'music'
        track_info['title'] ='Fake EVO Title'
        artist = track_info.get_or_create_list('artists').add_struct()
        artist['id'] = 234567
        artist['name'] = 'Fake EVO Artist'
        artist['composer'] = True
        album = track_info.get_or_create_list('albums').add_struct()
        album['id'] = 345687
        album['title'] = 'Fake EVO Album'
        album['genre'] = 'alternative'

    def radio_play(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.radio_player
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')
        self._device_state.Radio['radioId'] = data.radioId
        self._device_state.Radio['last_play_timestamp'] = self.alice_time_ms

        currently_playing = self._device_state.Radio.get_or_create_struct('currently_playing')
        currently_playing['radioId'] = data.radioId

        player = self._device_state.Radio.get_or_create_struct('player')
        player['paused'] = False

    def video_play(self, data):
        self.player_pause(data=None)
        self.audio_stop(data=None)

        self._device_state.Video.CurrentScreen = ScreenEnum.video_player
        self._device_state.Video.ClearField('ViewState')
        self._device_state.Video.ClearField('ScreenState')
        self._device_state.Video.LastPlayTimestamp = self.alice_time_ms

        currently_playing = self._device_state.Video.CurrentlyPlaying
        currently_playing.LastPlayTimestamp = self._device_state.Video.LastPlayTimestamp
        json_format.ParseDict(data.item.dict(), currently_playing.RawItem, ignore_unknown_fields=True)
        if data.next_item:
            json_format.ParseDict(data.next_item.dict(), currently_playing.RawNextItem, ignore_unknown_fields=True)
        if data.tv_show_item:
            json_format.ParseDict(data.tv_show_item.dict(), currently_playing.RawTvShowItem)

        self._device_state.extra.Video.start_at = data.start_at
        currently_playing.Progress.Played = data.start_at
        currently_playing.Progress.Duration = currently_playing.RawItem.Duration

        # TODO(mihajlova): set from RawItem after
        # https://st.yandex-team.ru/PLAYERANDROID-401
        if data.audio_language:
            currently_playing.AudioLanguage = data.audio_language
        if data.subtitles_language:
            currently_playing.SubtitlesLanguage = data.subtitles_language

    def tts_play_placeholder(self, data):
        pass

    def start_music_recognizer(self, data):
        pass

    def show_gallery(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.gallery
        self._device_state.Video.ScreenState.ViewKey = ''

        screen_state = self._device_state.Video.ScreenState
        for item in data.items:
            json_format.ParseDict(item.dict(), screen_state.RawItems.add(), ignore_unknown_fields=False)

        screen_state.VisibleItems.extend(list(range(3)))

    def show_tv_gallery(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.tv_gallery
        self._device_state.Video.ScreenState.ViewKey = ''

        screen_state = self._device_state.Video.ScreenState
        for item in data.items:
            json_format.ParseDict(item.dict(), screen_state.RawItems.add(), ignore_unknown_fields=False)

        screen_state.VisibleItems.extend(list(range(10)))

    def send_bug_report(self, data):
        pass

    def start_multiroom(self, data):
        # TODO(sparkle): set "multiroom" field in device_state?
        pass

    def stop_multiroom(self, data):
        # TODO(sparkle): unset "multiroom" field in device_state?
        pass

    def show_description(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.description
        self._device_state.Video.ScreenState.ViewKey = ''
        json_format.ParseDict(data.item.dict(), self._device_state.Video.ScreenState.RawItem)
        if data.tv_show_item:
            json_format.ParseDict(data.tv_show_item.dict(), self._device_state.Video.ScreenState.TvShowItem)

    def show_pay_push_screen(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.payment
        self._device_state.Video.ScreenState.ViewKey = ''
        json_format.ParseDict(data.item.dict(), self._device_state.Video.ScreenState.RawItem)
        if data.tv_show_item:
            json_format.ParseDict(data.tv_show_item.dict(), self._device_state.Video.ScreenState.TvShowItem)

    def show_season_gallery(self, data):
        self._device_state.Video.CurrentScreen = ScreenEnum.season_gallery
        self._device_state.Video.ScreenState.ViewKey = ''

        screen_state = self._device_state.Video.ScreenState
        json_format.ParseDict(data.tv_show_item.dict(), self._device_state.Video.ScreenState.TvShowItem)
        self._device_state.Video.ScreenState.Season = data.season
        for item in data.items:
            json_format.ParseDict(item.dict(), screen_state.RawItems.add(), ignore_unknown_fields=False)

        screen_state.VisibleItems.extend(list(range(10)))

    def alarm_set_max_level(self, data):
        self._device_state.AlarmState.MaxSoundLevel = data.new_level

    def start_bluetooth(self, data):
        pass

    def stop_bluetooth(self, data):
        pass

    def open_uri(self, data):
        # Station doesn't support it. Added in order to not fail some test cases.
        pass

    def show_alarms(self, data):
        # Station doesn't support it. Added in order to not fail some test cases.
        pass

    def success_starting_onboarding(self, data):
        pass

    def set_glagol_metadata(self, data):
        pass

    def listen(self, data):
        pass

    def draw_scled_animations(self, data):
        # Supported only on YandexMini2.
        pass

    def draw_led_screen(self, data):
        pass

    def show_clock(self, data):
        pass

    def hide_clock(self, data):
        pass

    def client_push_typed_semantic_frame(self, data):
        pass

    def messenger_call(self, data):
        pass

    def set_fixed_equalizer_bands_directive(self, data):
        pass

    def set_adjustable_equalizer_bands_directive(self, data):
        pass

    def draw_animation_directive(self, data):
        pass

    def enrollment_start(self, data):
        pass

    def enrollment_cancel(self, data):
        pass

    def enrollment_finish(self, data):
        pass

    def multiaccount_remove_account(self, data):
        pass
