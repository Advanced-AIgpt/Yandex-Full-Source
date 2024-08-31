from alice.analytics.operations.core_spu.utils import (
    get_music_slots,
    get_music_filters,
    get_video_filters,
    is_regular_alarm,
    is_custom_iot_scenario,
)


class BaseCoreSPUActivity(object):
    name = None
    score = 0.
    allowed_device_types = []
    allowed_apps = []

    def __init__(self, device_id, device_type, app, activation_date, interval, job_date, is_exp_format=False):
        self.device_id = device_id
        self.device_type = device_type
        self.activation_date = activation_date
        self.app = app
        self.interval = interval
        self.job_date = job_date
        self.is_exp_format = is_exp_format

    def check_conditions(self, records):
        pass

    def check_allowed_device_types(self):
        is_allowed = True
        if self.allowed_device_types:
            if self.device_type not in self.allowed_device_types:
                is_allowed = False
        if self.allowed_apps:
            if self.app not in self.allowed_apps:
                is_allowed = False
        return is_allowed

    def check_records_score(self, records):
        if not self.check_allowed_device_types():
            return 0
        if not self.check_conditions(records):
            return 0
        return self.score


class ScenarioCoreSPUActivity(BaseCoreSPUActivity):
    generic_scenarios = []

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario in self.generic_scenarios:
                return True


class TVChannelActivity(ScenarioCoreSPUActivity):
    name = 'video.tv_channel'
    score = 1.
    generic_scenarios = ['tv_stream']
    allowed_apps = ['quasar']


class TVBroadcastActivity(ScenarioCoreSPUActivity):
    name = 'tv_broadcast'
    score = 1.
    generic_scenarios = ['tv_broadcast']

    def check_records_score(self, records):
        feature_score = super(TVBroadcastActivity, self).check_records_score(records)
        if self.app == 'quasar':
            return feature_score * 0.5
        else:
            return feature_score


class MorningShowActivity(ScenarioCoreSPUActivity):
    name = 'music.morning_show'
    score = 1.
    generic_scenarios = ['morning_show', 'alice_show']


class MusicAmbientSoundActivity(ScenarioCoreSPUActivity):
    name = 'music.ambient_sound'
    score = 1. / 5
    generic_scenarios = ['music_ambient_sound']


class MusicFairyTaleActivity(ScenarioCoreSPUActivity):
    name = 'music.fairy_tale'
    score = 1. / 5
    generic_scenarios = ['music_fairy_tale']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario in self.generic_scenarios and \
                    rec.music_answer_type in ('music_result', 'playlist'):
                return True


class AlarmActivity(ScenarioCoreSPUActivity):
    name = 'alarm'
    score = 1.
    generic_scenarios = ['alarm']


class TimerActivity(ScenarioCoreSPUActivity):
    name = 'timer'
    score = 1.
    generic_scenarios = ['timer']


class SleepTimerActivity(ScenarioCoreSPUActivity):
    name = 'sleep_timer'
    score = 1. / 2
    generic_scenarios = ['sleep_timer']


class NewsActivity(ScenarioCoreSPUActivity):
    name = 'news'
    score = 1.
    generic_scenarios = ['get_news']


class WeatherActivity(ScenarioCoreSPUActivity):
    name = 'weather'
    score = 1.
    generic_scenarios = ['weather']


class IoTActivity(ScenarioCoreSPUActivity):
    name = 'iot.common'
    score = 1.
    generic_scenarios = ['iot_do']


class TrafficRouteActivity(ScenarioCoreSPUActivity):
    name = 'traffic_route'
    score = 1.
    generic_scenarios = ['show_traffic', 'route']
    allowed_apps = ['small_smart_speakers']


class MusicRateActivity(BaseCoreSPUActivity):
    name = 'music.rate'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.intent in ('personal_assistant\tscenarios\tplayer_like',
                              'personal_assistant\tscenarios\tplayer_dislike'):
                return True


class RadioActivity(BaseCoreSPUActivity):
    name = 'radio'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'radio' and rec.intent == 'personal_assistant\tscenarios\tradio_play':
                return True


class RegularAlarmActivity(BaseCoreSPUActivity):
    name = 'alarm.regular'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'alarm' and rec.intent == 'personal_assistant\tscenarios\talarm_set':
                if is_regular_alarm(rec.analytics_info):
                    return True


class TodoReminderActivity(BaseCoreSPUActivity):
    name = 'todo_reminder'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario in ('todo', 'reminder'):
                return True


class IoTCustomScenarioActivity(BaseCoreSPUActivity):
    name = 'iot.custom'
    score = 2.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'iot_do':
                if is_custom_iot_scenario(rec.intent, rec.analytics_info, rec.query):
                    return True


class MusicTrackActivity(BaseCoreSPUActivity):
    name = 'music.track'
    score = 1. / 2

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'track':
                    return True


class MusicArtistActivity(BaseCoreSPUActivity):
    name = 'music.artist'
    score = 1. / 3

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'artist':
                    return True


class MusicAlbumActivity(BaseCoreSPUActivity):
    name = 'music.album'
    score = 1. / 3

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'album':
                    return True


class MusicPlaylistActivity(BaseCoreSPUActivity):
    name = 'music.playlist'
    score = 1. / 3

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'playlist':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)
                    if 'personality' not in music_filters:
                        return True


class MusicEmptyFilterActivity(BaseCoreSPUActivity):
    name = 'music.empty_filter'
    score = 1. / 2

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'filters':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)

                    if len(music_filters) == 0:
                        return True


class MusicGenreActivity(BaseCoreSPUActivity):
    name = 'music.genre'
    score = 1. / 5

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'filters':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)

                    if 'genre' in music_filters:
                        return True


class MusicPersonalityActivity(BaseCoreSPUActivity):
    name = 'music.personality'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type in ['filters', 'playlist']:
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)
                    if 'personality' in music_filters:
                        return True


class MusicMoodActivity(BaseCoreSPUActivity):
    name = 'music.mood_activity'
    score = 1.

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'filters':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)

                    if 'mood' in music_filters or 'activity' in music_filters:
                        return True


class MusicLanguageActivity(BaseCoreSPUActivity):
    name = 'music.language'
    score = 1. / 5

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'filters':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)

                    if 'language' in music_filters:
                        return True


class MusicEpochActivity(BaseCoreSPUActivity):
    name = 'music.epoch'
    score = 1. / 5

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'music' and rec.intent == 'personal_assistant\tscenarios\tmusic_play':
                if rec.music_answer_type == 'filters':
                    music_slots = get_music_slots(rec.analytics_info)
                    music_filters = get_music_filters(music_slots)

                    if 'epoch' in music_filters:
                        return True


class VideoRecommendActivity(BaseCoreSPUActivity):
    name = 'video.recommend'
    score = 1. / 3
    allowed_apps = ['quasar']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'video' and rec.intent == 'mm\tpersonal_assistant\tscenarios\tvideo_play':
                video_filters = get_video_filters(rec.analytics_info)
                for filter_ in video_filters:
                    if filter_['name'] == 'action' and filter_['value'] == 'recommend':
                        return True


class VideoGenreActivity(BaseCoreSPUActivity):
    name = 'video.genre'
    score = 1. / 3
    allowed_apps = ['quasar']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'video' and rec.intent == 'mm\tpersonal_assistant\tscenarios\tvideo_play':
                video_filters = get_video_filters(rec.analytics_info)
                for filter_ in video_filters:
                    if filter_['name'] == 'film_genre':
                        return True


class VideoEpochActivity(BaseCoreSPUActivity):
    name = 'video.epoch'
    score = 1. / 3
    allowed_apps = ['quasar']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'video' and rec.intent == 'mm\tpersonal_assistant\tscenarios\tvideo_play':
                video_filters = get_video_filters(rec.analytics_info)
                for filter_ in video_filters:
                    if filter_['name'] == 'release_date':
                        return True


class VideoCountryActivity(BaseCoreSPUActivity):
    name = 'video.country'
    score = 1. / 2
    allowed_apps = ['quasar']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'video' and rec.intent == 'mm\tpersonal_assistant\tscenarios\tvideo_play':
                video_filters = get_video_filters(rec.analytics_info)
                for filter_ in video_filters:
                    if filter_['name'] == 'country':
                        return True


class VideoEmptyFilterActivity(BaseCoreSPUActivity):
    name = 'video.empty_filter'
    score = 1. / 2
    allowed_apps = ['quasar']

    def check_conditions(self, records):
        for rec in records:
            if rec.generic_scenario == 'video' and rec.intent == 'mm\tpersonal_assistant\tscenarios\tvideo_play':
                video_filters = get_video_filters(rec.analytics_info)
                for filter_ in video_filters:
                    if filter_['name'] in ('find', 'play', 'search_text'):
                        return True


EXPBOXES_ACTIVITIES = [
    TVChannelActivity,
    TVBroadcastActivity,
    MorningShowActivity,
    TimerActivity,
    TrafficRouteActivity,
    SleepTimerActivity,
    NewsActivity,
    WeatherActivity,
    RadioActivity,
    AlarmActivity,
    RegularAlarmActivity,
    TodoReminderActivity,
    IoTActivity,
    IoTCustomScenarioActivity,

    MusicAmbientSoundActivity,
    MusicFairyTaleActivity,
    MusicRateActivity,
    MusicTrackActivity,
    MusicArtistActivity,
    MusicAlbumActivity,
    MusicPlaylistActivity,
    MusicEmptyFilterActivity,
    MusicGenreActivity,
    MusicPersonalityActivity,
    MusicMoodActivity,
    MusicLanguageActivity,
    MusicEpochActivity,

    VideoRecommendActivity,
    VideoGenreActivity,
    VideoEpochActivity,
    VideoCountryActivity,
    VideoEmptyFilterActivity,
]
