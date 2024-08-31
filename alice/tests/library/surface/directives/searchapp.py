from alice.tests.library.uniclient import event

from . import mixin


_SearchLevel = dict(none=0, moderate=1, strict=2)


class DirectivesMixin(
    mixin.ImageDirectives,
    mixin.PlayerDirectives,
    mixin.SkillsDirectives,
    mixin.SoundDirectives,
):
    def open_uri(self, data):
        if data.uri.startswith('musicsdk://') and 'play=true' in data.uri:
            self._device_state.Music.Player.Pause = False
            # TODO: Fill self._device_state.Music.Player.CurrentlyPlaying with some relevant data
            # See similar https://a.yandex-team.ru/arc/trunk/arcadia/alice/tests/integration_tests/surface/directives/station.py?rev=r8253927#L308

    def reminders_set(self, data):
        tsf = data['on_shoot_frame']['payload']['typed_semantic_frame']['reminders_on_shoot_semantic_frame']
        self._device_state.DeviceReminders.List.add(
            Id=tsf['id']['string_value'],
            Text=tsf['text']['string_value'],
            Epoch=tsf['epoch']['epoch_value'],
            TimeZone=tsf['timezone']['string_value'],
            Origin='')
        return self._request(event=data['on_success_callback'])

    def reminders_cancel(self, data):
        action = data['action']
        if action == 'id':
            ids = set(data['id'])
            reminders = self._device_state.DeviceReminders.List
            reminders = [_ for _ in reminders if _.Id not in ids]
            del self._device_state.DeviceReminders.List[:]
            self._device_state.DeviceReminders.List.extend(reminders)
        elif action == 'all':
            del self._device_state.DeviceReminders.List[:]

        return self._request(event=data['on_success_callback'])

    def tts_play_placeholder(self, data):
        pass

    def type(self, data):
        return self._request(request=data.text)

    def type_silent(self, data):
        pass

    def start_music_recognizer(self, data):
        pass

    def alarm_new(self, data):
        return self._request(event=event.UpdateForm(data.on_success.dict()))

    def show_alarms(self, data):
        pass

    def set_timer(self, data):
        return self._request(event=event.UpdateForm(data.on_success.dict()))

    def show_timers(self, data):
        pass

    def open_bot(self, data):
        pass

    def find_contacts(self, data):
        pass

    def special_button_list(self, data):
        pass

    def set_search_filter(self, data):
        self._additional_options.BassOptions.FiltrationLevel = _SearchLevel[data.new_level]

    def show_tv_gallery(self, data):
        pass

    def request_permissions(self, data):
        pass

    def take_screenshot(self, data):
        pass

    def show_buttons(self, data):
        pass

    def fill_cloud_ui(self, data):
        pass

    def show_view(self, data):
        pass
