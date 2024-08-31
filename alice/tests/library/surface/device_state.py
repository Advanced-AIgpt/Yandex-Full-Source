import alice.megamind.protos.common.device_state_pb2 as device_state_pb2
import alice.tests.library.uniclient as uniclient
import icalendar
import pytz


class ScreenEnum(object):
    main = 'main'
    gallery = 'gallery'
    tv_gallery = 'tv_gallery'
    season_gallery = 'season_gallery'
    description = 'description'
    payment = 'payment'
    music_player = 'music_player'
    video_player = 'video_player'
    radio_player = 'radio_player'
    mordovia_webview = 'mordovia_webview'


class DeviceState(uniclient.DeviceState):
    class _ExtraState(object):
        def __init__(self):
            class _item(object):
                def __getattr__(self, key):
                    return None

            for f in device_state_pb2.TDeviceState.DESCRIPTOR.fields:
                setattr(self, f.name, _item())

    def __init__(self, device_state):
        super().__init__(device_state)
        self._extra = DeviceState._ExtraState()

    @property
    def extra(self):
        return self._extra

    @property
    def alarms(self):
        tz = self.extra.AlarmState.timezone
        tz = pytz.timezone(tz) if tz else None
        cal = icalendar.Calendar.from_ical(self.AlarmState.ICalendar)
        return [_.get('dtstart').dt.astimezone(tz) for _ in cal.walk('vevent')]

    def dict(self):
        proto = super().dict()

        # fix uint
        for t in proto.get('timers', {}).get('active_timers', []):
            t['start_timestamp'] = int(t['start_timestamp'])

        # fix timestamp
        def _fix_timestamp(obj, key):
            timestamp = obj.get(key)
            if timestamp:
                obj[key] = int(timestamp)

        audio = proto.get('audio_player', {})
        _fix_timestamp(audio, 'last_play_timestamp')
        _fix_timestamp(audio.get('current_stream', {}), 'last_play_timestamp')
        _fix_timestamp(audio, 'last_stop_timestamp')

        music = proto.get('music', {})
        _fix_timestamp(music, 'last_play_timestamp')
        _fix_timestamp(music.get('currently_playing', {}), 'last_play_timestamp')
        _fix_timestamp(music.get('player', {}), 'timestamp')

        video = proto.get('video', {})
        _fix_timestamp(video, 'last_play_timestamp')
        _fix_timestamp(video.get('currently_playing', {}), 'last_play_timestamp')

        return proto
