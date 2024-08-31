import uuid

from alice.acceptance.modules.request_generator.lib import app_presets
from alice.tests.library.uniclient import event

from .alice import Alice, EvoError
from .device_state import DeviceState, ScreenEnum
from .directives import automotive, browser, launcher, legatus, navi, maps, sdc, searchapp, \
    smart_display, smart_tv, station, station_pro, taximeter, watch, webtouch, yabro_win


def _create_fake_uuid():
    return 'f' * 16 + str(uuid.uuid4()).replace('-', '')[16:32]


def _union_features(preset_features, test_features):
    features = {_: True for _ in preset_features or []}
    features.update(test_features)
    return [_ for _, is_enabled in features.items() if is_enabled]


class _Alice(Alice):
    preset = None

    def __init__(self, settings, device_state):
        settings.auth_token = self.preset.auth_token
        settings.asr_topic = self.preset.asr_topic
        settings.user_agent = settings.user_agent or self.preset.user_agent
        settings.supported_features = _union_features(
            self.preset.supported_features,
            settings.supported_features,
        )
        settings.unsupported_features = _union_features(
            self.preset.unsupported_features,
            settings.unsupported_features,
        )
        settings.application = dict(
            self.preset.application,
            **settings.application,
        )
        settings.application.setdefault('lang', 'ru-RU')
        settings.application.setdefault('uuid', _create_fake_uuid())
        super().__init__(settings, DeviceState(device_state))


class _CameraMixin(object):
    def send_image(self, img_url, capture_mode):
        self._voice_session = True
        return self._send_event(event.ImageInput(
            img_url=img_url,
            capture_mode=capture_mode,
        ))

    def search_by_photo(self, photo):
        return self.send_image(photo, capture_mode='photo')

    def read_text(self, photo):
        return self.send_image(photo, capture_mode='voice_text')

    def scan_photo(self, photo):
        return self.send_image(photo, capture_mode='document')


class _RemotePlayerMixin(object):
    def remote_player_next_track(self):
        return self._send_event(event.PlayerNextTrack())

    def remote_player_prev_track(self):
        return self._send_event(event.PlayerPrevTrack())

    def remote_player_replay(self):
        return self._send_event(event.PlayerReplay())

    def remote_player_continue(self):
        return self._send_event(event.PlayerContinue())

    def remote_player_shuffle(self):
        return self._send_event(event.PlayerShuffle())

    def remote_player_rewind(self, time, rewind_type='forward'):
        return self._send_event(event.PlayerRewind(time, rewind_type))

    def remote_player_repeat(self):
        return self._send_event(event.PlayerRepeat())

    def remote_player_what_is_playing(self):
        return self._send_event(event.PlayerWhatIsPlaying())

    def remote_player_like(self):
        return self._send_event(event.PlayerLike())

    def remote_player_dislike(self):
        return self._send_event(event.PlayerDislike())


class _RemoteControlMixin(object):
    def setup_rcu_status_frame(self, status):
        return self._send_event(event.SetupRcuStatus(
            status=status
        ))

    def setup_rcu_auto_status_frame(self, status):
        return self._send_event(event.SetupRcuAutoStatus(
            status=status
        ))

    def setup_rcu_check_status_frame(self, status):
        return self._send_event(event.SetupRcuCheckStatus(
            status=status
        ))

    def setup_rcu_advanced_status_frame(self, status):
        return self._send_event(event.SetupRcuAdvancedStatus(
            status=status
        ))

    def setup_rcu_manual_start_frame(self):
        return self._send_event(event.SetupRcuManualStart())

    def setup_rcu_auto_start_frame(self, tv_model=None):
        return self._send_event(event.SetupRcuAutoStart(
            tv_model=tv_model
        ))

    def request_technical_support_frame(self):
        return self._send_event(event.RequestTechnicalSupport())

    def volume_up(self):
        pass

    def volume_down(self):
        pass


class _NaviAppMixin(object):
    def reset_route(self):
        self.device_state.Navigator.Clear()


class _AutoAppMixin(object):
    def greet(self, first_name):
        self.reset_session()
        return self._send_event(event.ServerAction(
            name='on_reset_session',
            mode='automotive.greeting',
            first_name=first_name,
        ))


class _SmartSpeaker(_Alice, station.DirectivesMixin, _RemotePlayerMixin):
    def login(self):
        raise EvoError('TODO: recreate uniclient and use new websocket to emulate reload')

    def plug_tv_in(self):
        pass

    def plug_tv_out(self):
        pass


class _YandexSmartSpeaker(_SmartSpeaker):
    def __init__(self, settings, device_state):
        settings.user_agent = self.preset.user_agent.replace('/', r'\/')
        settings.sync_state_supported_features = ['notifications']
        device_id = settings.application.get('device_id') or self.preset.application['device_id']
        device_state.setdefault('device_id', device_id)
        super().__init__(settings, device_state)


class Dexp(_SmartSpeaker):
    preset = app_presets.DEXP


class Quasar(_YandexSmartSpeaker):
    preset = app_presets.QUASAR

    def __init__(self, settings, device_state):
        quasar_device_state = {
            'is_tv_plugged_in': True,
            'video': {'current_screen': ScreenEnum.main},
            **device_state,
        }
        super().__init__(settings, quasar_device_state)

    def plug_tv_in(self):
        self.device_state.IsTvPluggedIn = True

    def plug_tv_out(self):
        self.device_state.IsTvPluggedIn = False


class YandexMax(Quasar, station_pro.DirectivesMixin, _RemoteControlMixin):
    preset = app_presets.YANDEXMAX


class SmartDisplay(_YandexSmartSpeaker, smart_display.DirectivesMixin):
    preset = app_presets.SMART_DISPLAY

    def collect_teasers(self, carousel_id):
        return self._send_event(event.CentaurCollectCards(
            carousel_id=carousel_id,
        ))

    def collect_main_screen(self, widget_gallery_position=None):
        return self._send_event(event.CentaurCollectMainScreen(
            widget_gallery_position=widget_gallery_position,
        ))

    def collect_widget_gallery(self, column, row):
        return self._send_event(event.CentaurCollectWidgetGallery(
            column=column,
            row=row,
        ))

    def add_widget_from_gallery(self, column, row, widget_config_data_slot):
        return self._send_event(event.CentaurAddWidgetFromGallery(
            column=column,
            row=row,
            widget_config_data_slot=widget_config_data_slot,
        ))


class YandexMini(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMINI


class YandexMidi(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMIDI


class YandexMicroBeige(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.BEIGE]


class YandexMicroGreen(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.GREEN]


class YandexMicroPink(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.PINK]


class YandexMicroPurple(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.PURPLE]


class YandexMicroRed(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.RED]


class YandexMicroYellow(_YandexSmartSpeaker):
    preset = app_presets.YANDEXMICRO_BY_COLOR[app_presets.StationColor.YELLOW]


class _AliceActualNaviApp(_Alice, _NaviAppMixin):
    def __init__(self, settings, device_state):
        navi_device_state = {
            'navigator': {'available_voice_ids': [
                'ru_female',
                'ru_male',
                'ru_alice',
                'ru_easter_egg',
                'ru_buzova',
                'ru_starwars_light',
                'ru_kharlamov',
                'ru_basta',
                'ru_optimus',
            ]},
            **device_state,
        }
        super().__init__(settings, navi_device_state)


class _Automotive(_AliceActualNaviApp, automotive.DirectivesMixin, _AutoAppMixin):
    pass


class Auto(_Automotive):
    preset = app_presets.AUTO


class AutoNew(_Automotive):
    preset = app_presets.AUTO_NEW


class AutoOld(_Alice, automotive.OldDirectivesMixin, _AutoAppMixin, _NaviAppMixin):
    preset = app_presets.AUTO_OLD

    def __init__(self, settings, device_state):
        settings.oauth_token = None
        settings.radio_stations = ['Авторадио', 'Монте-Карло', 'Юмор FM', 'Говорит Москва']
        super().__init__(settings, device_state)


class Navigator(_AliceActualNaviApp, navi.DirectivesMixin):
    preset = app_presets.NAVIGATOR

    def __init__(self, settings, device_state):
        settings.experiments.setdefault('internal_music_player', '1')
        settings.experiments.setdefault('music_for_everyone', '1')
        super().__init__(settings, device_state)


class NavigatorTr(_Alice, navi.DirectivesMixin, _NaviAppMixin):
    preset = app_presets.NAVIGATOR

    def __init__(self, settings, device_state):
        settings.application.setdefault('lang', 'tr-TR')
        super().__init__(settings, device_state)


class Maps(_Alice, maps.DirectivesMixin):
    preset = app_presets.MAPS


class Launcher(_Alice, launcher.DirectivesMixin):
    preset = app_presets.LAUNCHER

    def update_weather(self):
        return self._send_event(event.UpdateForm(
            payload={
                'form_update': {
                    'push_id': 'weather_today',
                    'name': 'personal_assistant.scenarios.get_weather',
                    'slots': [{
                        'optional': True,
                        'name': 'when',
                        'value': {
                            'seconds': 0,
                            'seconds_relative': True,
                        },
                        'type': 'datetime',
                        'source_text': 'сейчас',
                    }],
                },
                'resubmit': True,
            },
        ))


class Searchapp(_Alice, _CameraMixin, searchapp.DirectivesMixin):
    preset = app_presets.SEARCH_APP_PROD

    def __init__(self, settings, device_state):
        settings.experiments.setdefault('internal_music_player', '1')
        settings.experiments.setdefault('music_for_everyone', '1')
        super().__init__(settings, device_state)

    def activate(self):
        return self._send_event(event.OnboardingGetGreetings())

    def greet(self, first_start=False):
        self.reset_session()
        return self._send_event(event.ServerAction(
            name='on_reset_session' if first_start else 'on_get_greetings',
            mode='onboarding' if first_start else None,
        ))

    def image_search_onboarding(self):
        return self._send_event(event.UpdateForm(
            payload={
                'form_update': {
                    'name': 'personal_assistant.scenarios.onboarding_image_search',
                },
                'resubmit': True,
            },
        ))


class Aliceapp(Searchapp):
    preset = app_presets.ALICE_APP_PROD


class Browser(_Alice, browser.DirectivesMixin):
    preset = app_presets.BROWSER_PROD


class BrowserIos(Browser):
    preset = app_presets.BROWSER_PROD_IOS


class Sdc(_Alice, sdc.DirectivesMixin):
    preset = app_presets.SDC


class SmartTv(_Alice, smart_tv.DirectivesMixin):
    preset = app_presets.TV


class Module2(_Alice, smart_tv.DirectivesMixin):
    preset = app_presets.MODULE_2


class Taximeter(_Alice, taximeter.DirectivesMixin):
    preset = app_presets.TAXIMETER


class Watch(_Alice, watch.DirectivesMixin):
    preset = app_presets.ELARI_WATCH


class Webtouch(_Alice, webtouch.DirectivesMixin):
    preset = app_presets.WEBTOUCH_PROD


class WebtouchIos(Webtouch):
    preset = app_presets.WEBTOUCH_PROD_IOS


class YabroWin(_Alice, yabro_win.DirectivesMixin):
    preset = app_presets.YABRO_PROD


class Legatus(_Alice, legatus.DirectivesMixin):
    preset = app_presets.LEGATUS
