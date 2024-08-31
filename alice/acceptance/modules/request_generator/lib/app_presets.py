import attr

from copy import deepcopy
from enum import Enum


class StationColor(Enum):
    GREEN = ('green', 'G')
    PURPLE = ('purple', 'P')
    RED = ('red', 'R')
    BEIGE = ('beige', 'B')
    YELLOW = ('yellow', 'Y')
    PINK = ('pink', 'N')


@attr.s
class AppPreset():
    application = attr.ib()
    auth_token = attr.ib()
    asr_topic = attr.ib()
    user_agent = attr.ib()
    supported_features = attr.ib(default=None)
    unsupported_features = attr.ib(default=None)
    capabilities = attr.ib(default=None)


DEFAULT_APP = AppPreset(
    application={
        'app_id': 'ru.yandex.searchplugin',
        'app_version': '10.00',
        'os_version': '11.0',
        'platform': 'iphone',
        'device_id': 'feedface-cdd2-4933-bd61-691bbc1dc56e',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 8.1.0; DUA-L22 Build/HONORDUA-L22; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/76.0.3809.89 Mobile Safari/537.36 YandexSearch/8.80'
    ),
)
IOT_APP_PROD_IOS = AppPreset(
    application={
        'app_id': 'com.yandex.iot',
        'app_version': '9510',
        'device_manufacturer': 'Apple',
        'device_model': 'iPhone',
        'os_version': '15.4.1',
        'platform': 'iphone',
        'device_id': 'feedface-a105-4abb-86f3-5105e892a8b9',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (iPhone; CPU iPhone OS 15_4_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) '
        'Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/95.10 Safari/604.1 IOT/1.0'
    ),
    supported_features=[
        'iot_ios_device_setup',
    ],
)
IOT_APP_PROD_ANDROID = AppPreset(
    application={
        'app_id': 'com.yandex.iot',
        'app_version': '21.112',
        'device_manufacturer': 'Redmi',
        'device_model': 'M2101K6G',
        'os_version': '11',
        'platform': 'android',
        'device_id': 'feedface-a712-4abb-86f3-5105e892a8b9',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; arm; Android 11; M2101K6G) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Chrome/94.0.4606.85 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.112'
    ),
    supported_features=[
        'iot_android_device_setup',
    ],
)
SEARCH_APP_PROD = AppPreset(
    application={
        'app_id': 'ru.yandex.searchplugin',
        'app_version': '21.22',
        'device_manufacturer': 'samsung',
        'device_model': 'SM-G965F',
        'os_version': '9',
        'platform': 'android',
        'device_id': 'feedface-e22e-4abb-86f3-5105e892a8b9',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 8.1.0; DUA-L22 Build/HONORDUA-L22; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/76.0.3809.89 Mobile Safari/537.36 YandexSearch/8.80'
    ),
    supported_features=[
        'bonus_cards_camera',
        'bonus_cards_list',
        'can_open_videotranslation_onboarding',
        'pedometer',
        'pwd_app_manager',
        'reader_app',
        'whocalls_message_filtering',
        'phone_address_book',
        'open_link_outgoing_device_calls',
        'cloud_ui',
        'cloud_ui_filling',
    ],
)
SEARCH_APP_BETA = AppPreset(
    application={
        'app_id': 'ru.yandex.searchplugin.beta',
        'app_version': '8.70',
        'device_manufacturer': 'xiaomi',
        'device_model': 'Redmi Note 5',
        'os_version': '8.1.0',
        'platform': 'android',
        'device_id': 'feedface-2e90-443b-89be-24bcfdfeea93',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 9; JAT-LX1 Build/HONORJAT-LX1; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/75.0.3770.100 Mobile Safari/537.36 YandexSearch/8.90'
    ),
    supported_features=[
        'bonus_cards_camera',
        'bonus_cards_list',
        'can_open_videotranslation_onboarding',
        'pedometer',
        'pwd_app_manager',
        'reader_app',
        'whocalls_message_filtering',
        'phone_address_book',
        'open_link_outgoing_device_calls',
        'cloud_ui',
        'cloud_ui_filling',
    ],
)
SEARCH_APP_IPAD = AppPreset(
    application={
        'app_id': 'ru.yandex.mobile.search.ipad',
        'app_version': '1906.1.157',
        'device_manufacturer': 'Apple',
        'device_model': 'iPad',
        'os_version': '9.2.1',
        'platform': 'iphone',
        'device_id': 'feedface-5062-4e72-9696-58382afec807',
    },
    auth_token='cc96633d-59d4-4724-94bd-f5db2f02ad13',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (iPad; CPU OS 12_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) '
        'Version/12.0 YaBrowser/19.7.3.121.11 Mobile/15E148 Safari/605.1'
    ),
    supported_features=[
        'bonus_cards_camera',
        'bonus_cards_list',
        'can_open_videotranslation_onboarding',
        'pedometer',
        'pwd_app_manager',
        'reader_app',
        'whocalls_message_filtering',
        'cloud_ui',
        'cloud_ui_filling',
    ],
)
SEARCH_APP_IOS = AppPreset(
    application={
        'app_id': 'ru.yandex.mobile',
        'app_version': '3100',
        'device_manufacturer': 'Apple',
        'device_model': 'iPhone',
        'os_version': '14.6',
        'platform': 'iphone',
        'device_id': 'feedface-cdd2-4933-bd61-691bbc1dc56e',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) '
        'Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/31.00 Safari/604.1'
    ),
    supported_features=[
        'bonus_cards_camera',
        'bonus_cards_list',
        'can_open_videotranslation_onboarding',
        'pedometer',
        'pwd_app_manager',
        'reader_app',
        'whocalls_message_filtering',
        'phone_address_book',
        'open_link_outgoing_device_calls',
        'cloud_ui',
        'cloud_ui_filling',
    ],
)
ALICE_APP_PROD = AppPreset(
    application={
        'app_id': 'com.yandex.alice',
        'app_version': '20.112',
        'device_manufacturer': 'samsung',
        'device_model': 'SM-G965F',
        'os_version': '9',
        'platform': 'android',
        'device_id': '1c1fc3be-3f18-c630-a732-9832370bb1eb',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 8.1.0; DUA-L22 Build/HONORDUA-L22; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/76.0.3809.89 Mobile Safari/537.36 YandexSearch/8.80'
    ),
)
BROWSER_PROD = AppPreset(
    application={
        'app_id': 'com.yandex.browser',
        'app_version': '19.7.1.93',
        'device_manufacturer': 'HUAWEI',
        'device_model': 'ANE-LX1',
        'os_version': '8.0.0',
        'platform': 'android',
    },
    supported_features=[
        'pwd_app_manager',
        'phone_address_book',
        'cloud_ui',
        'cloud_ui_filling',
    ],
    auth_token='1f8abf45-d7a8-4bb6-9b4c-31a2bb9668e0',
    asr_topic='desktop-general',
    user_agent='com.yandex.browser/19.7.1.93',
)
BROWSER_PROD_IOS = AppPreset(
    application={
        'app_id': 'ru.yandex.mobile.search',
        'app_version': '21.5.1.845',
        'device_manufacturer': 'Apple',
        'device_model': 'iPhone',
        'os_version': '14.6',
        'platform': 'iphone',
        'device_id': 'feedface-1641-407d-a590-cc26abb19df1',
    },
    auth_token='1f8abf45-d7a8-4bb6-9b4c-31a2bb9668e0',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) '
        'Mobile/15E148 YaBrowser/21.5.1.845 YaApp_iOS/31.00 Safari/604.1'
    ),
    supported_features=[
        'open_ibro_settings',
        'pwd_app_manager',
        'phone_address_book',
        'cloud_ui',
        'cloud_ui_filling',
    ],
)
BROWSER_ALPHA = AppPreset(
    application={
        'app_id': 'com.yandex.browser.alpha',
        'app_version': '19.7.3.46',
        'device_manufacturer': 'samsung',
        'device_model': 'SM-J120H',
        'os_version': '5.1.1',
        'platform': 'android',
        'device_id': 'feedface-b0df-4b69-bb96-e9bda4504ee8',
    },
    supported_features=[
        'pwd_app_manager',
        'phone_address_book',
        'cloud_ui',
        'cloud_ui_filling',
    ],
    auth_token='1f8abf45-d7a8-4bb6-9b4c-31a2bb9668e0',
    asr_topic='desktop-general',
    user_agent='com.yandex.browser.alpha/19.7.3.46',
)
BROWSER_BETA = AppPreset(
    application={
        'app_id': 'com.yandex.browser.beta',
        'app_version': '19.7.2.88',
        'device_manufacturer': 'HONOR',
        'device_model': 'AUM-L41',
        'os_version': '8.0.0',
        'platform': 'android',
        'device_id': 'feedface-e2ad-496a-8251-f05bd4ee4d18',
    },
    supported_features=[
        'pwd_app_manager',
        'phone_address_book',
        'cloud_ui',
        'cloud_ui_filling',
    ],
    auth_token='1f8abf45-d7a8-4bb6-9b4c-31a2bb9668e0',
    asr_topic='desktop-general',
    user_agent='com.yandex.browser.beta/19.7.2.88',
)
STROKA = AppPreset(
    application={
        'app_id': 'winsearchbar',
        'app_version': '6.3.0.9600',
        'device_manufacturer': 'Unknown',
        'device_model': 'Unknown',
        'os_version': '6.1.7601',
        'platform': 'Windows',
        'device_id': 'feedface-c8e2-47f4-be39-96991d4c90a8',
    },
    auth_token='14e2f152-e03a-439d-9abe-f470c27db24e',
    asr_topic='desktop-general',
    user_agent='winsearchbar/5.5.0.1923 (Unknown Unknown; Windows 6.3.0.9600)',
)
NAVIGATOR = AppPreset(
    application={
        'app_id': 'ru.yandex.yandexnavi',
        'app_version': '3.91',
        'device_manufacturer': 'xiaomi',
        'device_model': 'Redmi Note 5',
        'os_version': '8.1.0',
        'platform': 'android',
        'device_id': 'feedface-aa9d-4c8b-89f1-74f9a1739089',
    },
    auth_token='27fbd96d-ec5b-4688-a54d-421d81aa8cd2',
    asr_topic='dialog-maps',
    user_agent='',
)
MAPS = AppPreset(
    application={
        'app_id': 'ru.yandex.yandexmaps',
        'app_version': '10.5.4',
        'device_manufacturer': 'samsung',
        'device_model': 'SM-G996B',
        'os_version': '11',
        'platform': 'android',
        'device_id': 'feedface-e11e-a666-31f3-5106e892a9a2',
    },
    auth_token='87d9fcbc-602c-43df-becb-772a15340ea2',
    asr_topic='dialog-general',
    user_agent='',
    supported_features=[
        'open_link',
        'navigator',
        'maps_download_offline',
        'phone_address_book',
    ],
)
LAUNCHER = AppPreset(
    application={
        'app_id': 'com.yandex.launcher',
        'app_version': '2.1.2',
        'device_manufacturer': 'Fly',
        'device_model': 'FS507',
        'os_version': '6.0',
        'platform': 'android',
        'device_id': 'feedface-e7a8-4b9b-a63d-02c469c20516',
    },
    auth_token='1f2a4085-473c-4ab7-b010-cb9a3500dd37',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 8.1.0; YNDX-000SB Build/8710.1.A.0063.20190415; wv) '
        'AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/70.0.3538.110 Mobile Safari/537.36'
    ),
)
SMART_DISPLAY = AppPreset(
    application={
        'app_id': 'ru.yandex.centaur',
        'app_version': '1.0',
        'device_manufacturer': 'Yandex',
        'device_model': 'Station',
        'os_version': '6.0.1',
        'platform': 'android',
        'device_id': 'feedface-e8a2-4439-b2e7-689d95f277b7',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; arm_64; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/75.0.3770.143 YandexStation/2.3.3.13.510432417.20190918 (YandexIO) Safari/537.36'
    ),
    supported_features=[
        'multiroom',
        'change_alarm_sound',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'audio_client',
        'notifications',
        'tts_play_placeholder',
        'incoming_messenger_calls',
        'outgoing_messenger_calls',
        'publicly_available',
        'video_codec_AVC',
        'audio_codec_AAC',
        'audio_codec_VORBIS',
        'audio_codec_OPUS',
        'directive_sequencer',
        'show_view',
        'div2_cards',
        'show_timer',
        'do_not_disturb',
        'handle_android_app_intent',
    ],
)
QUASAR = AppPreset(
    application={
        'app_id': 'ru.yandex.quasar.app',
        'app_version': '1.0',
        'device_manufacturer': 'Yandex',
        'device_model': 'Station',
        'os_version': '6.0.1',
        'platform': 'android',
        'device_id': 'feedface-e8a2-4439-b2e7-689d95f277b7',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; arm_64; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/75.0.3770.143 YandexStation/2.3.3.13.510432417.20190918 (YandexIO) Safari/537.36'
    ),
    supported_features=[
        'absolute_volume_change',
        'relative_volume_change',
        'mordovia_webview',
        'multiroom',
        'multiroom_cluster',
        'multiroom_audio_client',
        'multiroom_audio_client_v2',
        'change_alarm_sound',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'audio_client',
        'audio_client_hls',
        'notifications',
        'tts_play_placeholder',
        'incoming_messenger_calls',
        'outgoing_messenger_calls',
        'publicly_available',
        'video_codec_AVC',
        'audio_codec_AAC',
        'audio_codec_VORBIS',
        'audio_codec_OPUS',
        'directive_sequencer',
        'set_alarm_semantic_frame_v2',
        "stereo_pair",
        'muzpult',
        'audio_bitrate192',
        'audio_bitrate320',
        'prefetch_invalidation',
        'equalizer'
    ],
)
YANDEXMAX = AppPreset(
    application={
        'app_id': 'ru.yandex.quasar.app',
        'app_version': '1.0',
        'device_manufacturer': 'Yandex',
        'device_model': 'Station_2',
        'os_version': '9',
        'platform': 'android',
        'device_id': '280b4000-0112-3300-000f-3834524e5050',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; arm; Android 9; yandexstation_2 Build/PPR1.180610.011; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/81.0.4044.138 YandexStation2/1.10.1.24.808865580.20201030.10 (YandexIO)'
    ),
    supported_features=[
        'absolute_volume_change',
        'relative_volume_change',
        'mordovia_webview',
        'multiroom',
        'multiroom_cluster',
        'multiroom_audio_client',
        'multiroom_audio_client_v2',
        'change_alarm_sound',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'led_display',
        'clock_display',
        'audio_client',
        'audio_client_hls',
        'tts_play_placeholder',
        'bluetooth_rcu',
        'incoming_messenger_calls',
        'outgoing_messenger_calls',
        'publicly_available',
        'video_codec_AVC',
        'video_codec_HEVC',
        'video_codec_VP9',
        'audio_codec_AAC',
        'audio_codec_AC3',
        'audio_codec_EAC3',
        'audio_codec_VORBIS',
        'audio_codec_OPUS',
        'notifications',
        'bluetooth_player',
        'cec_available',
        'directive_sequencer',
        'set_alarm_semantic_frame_v2',
        "stereo_pair",
        'muzpult',
        'audio_bitrate192',
        'audio_bitrate320',
        'prefetch_invalidation',
        'equalizer',
    ],
)
YANDEXMINI = AppPreset(
    application={
        'app_id': 'aliced',
        'app_version': '1.0',
        'device_manufacturer': 'Yandex',
        'device_model': 'yandexmini',
        'os_version': '1.0',
        'platform': 'Linux',
        'device_id': 'feedface-4e95-4fc9-ba19-7bf943a7bf55',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/61.0.3163.98 Safari/537.36 YandexStation/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)'
    ),
    supported_features=[
        'absolute_volume_change',
        'relative_volume_change',
        'multiroom',
        'multiroom_cluster',
        'multiroom_audio_client',
        'multiroom_audio_client_v2',
        'change_alarm_sound',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'bluetooth_player',
        'audio_client',
        'audio_client_hls',
        'notifications',
        'tts_play_placeholder',
        'incoming_messenger_calls',
        'outgoing_messenger_calls',
        'publicly_available',
        'directive_sequencer',
        'set_alarm_semantic_frame_v2',
        "stereo_pair",
        'muzpult',
        'audio_bitrate192',
        'audio_bitrate320',
        'prefetch_invalidation',
    ],
)

YANDEXMIDI = deepcopy(YANDEXMINI)
YANDEXMIDI.application.update({
    'device_model': 'yandexmidi',
    'device_id': 'feedface-2fcf-49bf-ac11-c68ad244e4ff'
})
YANDEXMIDI.capabilities = [{
    "parameters": {
        "supported_protocols": ["Zigbee", "WiFi"]
    },
    "@type": "type.googleapis.com/NAlice.TIotDiscoveryCapability"
}]

YANDEXMICRO_BY_COLOR = dict()

for station_color_item in StationColor:
    color_str = station_color_item.value[0]
    color_letter = station_color_item.value[1]
    YANDEXMICRO_BY_COLOR[station_color_item] = AppPreset(
        application={
            'app_id': 'aliced',
            'app_version': '1.0',
            'device_manufacturer': 'Yandex',
            'device_model': 'yandexmicro',
            'device_color': color_str,
            'os_version': '1.0',
            'platform': 'Linux',
            'device_id': f'f{color_letter}edface-4e95-4fc9-ba19-7bf943a7bf55',
        },
        auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
        asr_topic='quasar-general',
        user_agent=(
            'Mozilla/5.0 (Linux; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
            'Version/4.0 Chrome/61.0.3163.98 Safari/537.36 YandexStation/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)'
        ),
        supported_features=[
            'absolute_volume_change',
            'relative_volume_change',
            'multiroom',
            'multiroom_cluster',
            'multiroom_audio_client',
            'multiroom_audio_client_v2',
            'change_alarm_sound',
            'change_alarm_sound_level',
            'music_player_allow_shots',
            'bluetooth_player',
            'audio_client',
            'audio_client_hls',
            'notifications',
            'tts_play_placeholder',
            'incoming_messenger_calls',
            'outgoing_messenger_calls',
            'publicly_available',
            'directive_sequencer',
            'set_alarm_semantic_frame_v2',
            "stereo_pair",
            'muzpult',
            'audio_bitrate192',
            'audio_bitrate320',
            'prefetch_invalidation',
        ],
    )

YANDEXMICRO = YANDEXMICRO_BY_COLOR[StationColor.BEIGE]
DEXP = AppPreset(
    application={
        'app_id': 'aliced',
        'app_version': '1.0',
        'device_manufacturer': 'Dexp',
        'device_model': 'lightcomm',
        'os_version': '1.0',
        'platform': 'Linux',
        'device_id': 'feedface-0497-42f0-9227-206459a7f439',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/61.0.3163.98 Safari/537.36 YandexStation/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)'
    ),
    supported_features=[
        'change_alarm_sound',
        'music_player_allow_shots',
    ],
)
AUTO_NEW = AppPreset(
    application={
        'app_id': 'yandex.auto',
        'app_version': '1.9.0',
        'device_manufacturer': 'Allwinner',
        'device_model': 't3-polo-yaCS',
        'os_version': '6.0.1',
        'platform': 'android',
        'device_id': 'feedface-ea21-444a-93b0-1110590b9620',
    },
    auth_token='1a198b89-2443-4ac9-85b9-9db178271aec',
    asr_topic='dialog-maps',
    user_agent='',
)
AUTO = AppPreset(
    application={
        'app_id': 'yandex.auto',
        'app_version': '1.5.0',
        'device_manufacturer': 'Allwinner',
        'device_model': 't3-polo-yaCS',
        'os_version': '6.0.1',
        'platform': 'android',
        'device_id': 'feedface-ea21-444a-93b0-1110590b9620',
    },
    auth_token='1a198b89-2443-4ac9-85b9-9db178271aec',
    asr_topic='dialog-maps',
    user_agent='',
)
AUTO_OLD = AppPreset(
    application={
        'app_id': 'yandex.auto.old',
        'app_version': '1.2.0',
        'device_manufacturer': 'Allwinner',
        'device_model': 't3-polo-yaCS',
        'os_version': '6.0.1',
        'platform': 'android',
        'device_id': 'feedface-ea21-444a-93b0-1110590b9620',
    },
    auth_token='1a198b89-2443-4ac9-85b9-9db178271aec',
    asr_topic='dialog-maps',
    user_agent='',
)
ELARI_WATCH = AppPreset(
    application={
        'app_id': 'ru.yandex.iosdk.elariwatch',
        'app_version': '1.0',
        'device_manufacturer': 'KidPhone3G',
        'device_model': 'KidPhone3G',
        'os_version': '4.4.2',
        'platform': 'android',
        'device_id': 'feedface-ec60-4d27-884f-163d4c21bdfb',
    },
    auth_token='9051faff-b426-4251-9343-df10ee4d7a5d',
    asr_topic='dialog-general',
    user_agent='',
)
SMALL_SMART_SPEAKERS = AppPreset(
    application={
        'app_id': 'aliced',
        'app_version': '1.0',
        'device_manufacturer': 'Elari',
        'device_model': 'elari_a98',
        'os_version': '1.0',
        'platform': 'Linux',
        'device_id': 'feedface-72fe-48e4-a1d5-ea09a546a7e6',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/61.0.3163.98 Safari/537.36 YandexStation/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)'
    ),
    supported_features=[
        'audio_client',
    ],
)
YABRO_PROD = AppPreset(
    application={
        'app_id': 'YaBro',
        'app_version': '19.6.2.599',
        'device_manufacturer': 'Unknown',
        'device_model': 'Unknown',
        'os_version': '10.0.17134',
        'platform': 'Windows',
        'device_id': 'feedface-e38e-463d-be15-4fa56996c863',
    },
    auth_token='14e2f152-e03a-439d-9abe-f470c27db24e',
    asr_topic='desktop-general',
    user_agent='YaBro/19.7.2.470 (Unknown Unknown; Windows 6.3.9600.19401)',
)
YABRO_BETA = AppPreset(
    application={
        'app_id': 'YaBro.beta',
        'app_version': '19.7.0.1374',
        'device_manufacturer': 'Unknown',
        'device_model': 'Unknown',
        'os_version': '10.0.17134',
        'platform': 'Windows',
        'device_id': 'feedface-8046-4c81-938a-31d766174f51',
    },
    auth_token='14e2f152-e03a-439d-9abe-f470c27db24e',
    asr_topic='desktop-general',
    user_agent='YaBro.beta/19.9.1.74 (Unknown Unknown; Windows 10.0.17134.885)',
)
TAXIMETER = AppPreset(
    application={
        'app_id': 'ru.yandex.taximeter',
        'app_version': '9.30',
        'device_manufacturer': 'Unknown',
        'device_model': 'Unknown',
        'os_version': '10.0',
        'platform': 'android',
        'device_id': 'feedface-cdd2-4933-bd61-691bbc1dc56e',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='desktop-general',
    user_agent='Taximeter 9.30 (1234)',
)
TV = AppPreset(
    application={
        'app_id': 'com.yandex.tv.alice',
        'app_version': '2.1000.1000',
        'device_manufacturer': 'YTV',
        'device_model': 'yandex_tv_mt9632_cv',
        'platform': 'android',
        'device_id': 'feedface-6219-45e3-a140-41993ef7dad9',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='queries',
    user_agent='com.yandex.io.sdk/2.1000.1000.8305 (YTV YU43CV9632; Android 9)',
    supported_features=[
        'vertical_screen_navigation',
        'directive_sequencer',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'music_recognizer',
        'go_home',
        'publicly_available',
        'change_alarm_sound',
        'tts_play_placeholder',
        'music_quasar_client',
        'tv_open_collection_screen_directive',
        'has_synchronized_push',
        'tv_open_details_screen_directive',
        'tv_open_person_screen_directive',
        'server_action',
        'tandem_setup',
        'tv_open_search_screen_directive',
        'live_tv_scheme',
        'video_protocol',
        'video_play_directive',
        'unauthorized_music_directives',
        'audio_client_hls',
        'cec_available',
        'audio_client',
        'absolute_volume_change',
        'handle_android_app_intent',
        'tv_open_store'
    ],
    unsupported_features=[
        'outgoing_phone_calls',
        'open_link',
        'set_timer',
        'set_alarm',
        'synchronized_push_implementation',
    ],
)
MODULE_2 = AppPreset(
    application={
        'app_id': 'com.yandex.tv.alice',
        'app_version': '2.1000.1000',
        'device_manufacturer': 'Yandex',
        'device_model': 'yandexmodule_2',
        'platform': 'android',
        'device_id': 'feedface-6219-45e3-a140-41993ef7dad9',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='queries',
    user_agent='com.yandex.io.sdk/2.1000.1000.8305 (YTV YU43CV9632; Android 9)',
    supported_features=[
        'change_alarm_sound',
        'change_alarm_sound_level',
        'music_player_allow_shots',
        'tts_play_placeholder',
        'publicly_available',
        'directive_sequencer',
        'music_quasar_client',
        'video_protocol',
        'has_synchronized_push',
        'server_action',
        'music_recognizer',
        'live_tv_scheme',
        'video_play_directive',
        'unauthorized_music_directives',
        'tv_open_details_screen_directive',
        'tv_open_search_screen_directive',
        'tv_open_person_screen_directive',
        'tv_open_collection_screen_directive',
        'audio_client',
        'audio_client_hls',
        'vertical_screen_navigation',
        'bluetooth_rcu',
        'cec_available',
        'go_home',
        'relative_volume_change',
        'handle_android_app_intent',
        'tv_open_store',
        'tandem_setup',
    ],
    unsupported_features=[
        'outgoing_phone_calls',
        'open_link',
        'set_timer',
        'set_alarm',
        'synchronized_push_implementation',
        'absolute_volume_change'
    ],
)
LEGATUS = AppPreset(
    application={
        'app_id': 'legatus',
        'app_version': '1.0',
        'device_manufacturer': 'LG Electronics',
        'device_model': 'WEBOS22',
        'os_version': '7.2.0',
        'platform': 'W22H',
        'device_id': 'feedface-2fc3-49bf-8c11-c68ad244e4ff',
    },
    auth_token='51ae06cc-5c8f-48dc-93ae-7214517679e6',
    asr_topic='quasar-general',
    user_agent=(
        'Mozilla/5.0 (Linux; arm_64; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) '
        'Version/4.0 Chrome/75.0.3770.143 YandexStation/2.3.3.13.510432417.20190918 (YandexIO) Safari/537.36'
    ),
    supported_features=[
        'video_protocol',
        'music_sdk_client',
    ],
    unsupported_features=[
        'open_link',
        'set_alarm',
        'set_timer',
        'absolute_volume_change',
        'relative_volume_change',
        'mute_unmute_volume',
        'player_continue_directive',
        'player_pause_directive',
        'synchronized_push_implementation',
        'player_rewind_directive',
    ],
    capabilities=[{
        "parameters": {
            "available_apps": [
                {
                    "name": "kinopoisk",
                    "app_id": "com.685631.3411"
                },
                {
                    "name": "youtube",
                    "app_id": "youtube.leanback.v4"
                }
            ]
        },
        "meta": {
            "supported_directives": [
                "WebOSLaunchAppDirectiveType",
                "WebOSShowGalleryDirectiveType"
            ]
        },
        "@type": "type.googleapis.com/NAlice.TWebOSCapability"
    }]
)

SDC = AppPreset(
    application={
        'app_id': 'ru.yandex.sdg.taxi.inhouse',
        'app_version': '1.0.16',
        'device_manufacturer': 'Apple',
        'device_model': 'iPad',
        'os_version': '15.3.1',
        'platform': 'iphone',
        'device_id': 'feedface-6219-45e3-a140-41993ef7dac0',
    },
    auth_token='06762f99-b8cf-46d7-8482-6d46638ae755',
    asr_topic='dialog-maps',
    user_agent='Mozilla/5.0 (iPhone; CPU iPhone OS 10_3 like Mac OS X) AppleWebKit/603.1.23 (KHTML, like Gecko) Version/10.0 Mobile/14E5239e Safari/602.1',
    supported_features=[
        'navigator',
        'route_manager_start',
        'route_manager_stop',
        'outgoing_operator_calls',
    ],
    unsupported_features=[
        'open_link',
    ],
)

WEBTOUCH_PROD = AppPreset(
    application={
        'app_id': 'ru.yandex.webtouch',
        'app_version': '1.0',
        'device_manufacturer': 'HUAWEI',
        'device_model': 'ANE-LX1',
        'os_version': '8.0.0',
        'platform': 'android',
    },
    supported_features=[
        'open_link',
        'server_action',
        'cloud_ui',
        'show_promo',
        'show_view'
    ],
    unsupported_features=[
        'player_pause_directive'
    ],
    auth_token='effd5a3f-fd42-4a18-83a1-61766a6d0924',
    asr_topic='desktop-general',
    user_agent='Version/4.0 Chrome/76.0.3809.89 Mobile Safari/537.36 YandexSearch/8.80',
)

WEBTOUCH_PROD_IOS = AppPreset(
    application={
        'app_id': 'ru.yandex.webtouch',
        'app_version': '1.0',
        'device_manufacturer': 'Apple',
        'device_model': 'iPhone',
        'os_version': '14.6',
        'platform': 'iphone',
        'device_id': 'feedface-1641-407d-a590-cc26abb19df1',
    },
    unsupported_features=[
        'player_pause_directive'
    ],
    auth_token='effd5a3f-fd42-4a18-83a1-61766a6d0924',
    asr_topic='dialog-general',
    user_agent=(
        'Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) '
    ),
    supported_features=[
        'open_link',
        'server_action',
        'cloud_ui',
        'show_promo',
        'show_view'
    ]
)

APP_PRESET_CONFIG = {
    'iot_app_prod_ios': IOT_APP_PROD_IOS,
    'iot_app_prod_android': IOT_APP_PROD_ANDROID,
    'search_app_prod': SEARCH_APP_PROD,
    'search_app_beta': SEARCH_APP_BETA,
    'search_app_ipad': SEARCH_APP_IPAD,
    'search_app_ios': SEARCH_APP_IOS,
    'alice_prod': ALICE_APP_PROD,
    'browser_prod': BROWSER_PROD,
    'browser_prod_ios': BROWSER_PROD_IOS,
    'browser_alpha': BROWSER_ALPHA,
    'browser_beta': BROWSER_BETA,
    'webtouch_prod': WEBTOUCH_PROD,
    'webtouch_prod_ios': WEBTOUCH_PROD_IOS,
    'stroka': STROKA,
    'navigator': NAVIGATOR,
    'launcher': LAUNCHER,
    'auto': AUTO,
    'auto_new': AUTO_NEW,
    'auto_old': AUTO_OLD,
    'elariwatch': ELARI_WATCH,
    'small_smart_speakers': SMALL_SMART_SPEAKERS,
    'yabro_prod': YABRO_PROD,
    'yabro_beta': YABRO_BETA,
    'taximeter': TAXIMETER,
    'tv': TV,
    'module_2': MODULE_2,
    'centaur': SMART_DISPLAY,
    'smart_display': SMART_DISPLAY,  # For backward compatibility
    'maps': MAPS,
    'legatus': LEGATUS,
    'sdc': SDC,
    # stations
    'quasar': QUASAR,
    'yandexmax': YANDEXMAX,
    'yandexmini': YANDEXMINI,
    'yandexmidi': YANDEXMIDI,
    'dexp': DEXP,
    'yandexmicro_beige': YANDEXMICRO_BY_COLOR[StationColor.BEIGE],
    'yandexmicro_red': YANDEXMICRO_BY_COLOR[StationColor.RED],
    'yandexmicro_green': YANDEXMICRO_BY_COLOR[StationColor.GREEN],
    'yandexmicro_purple': YANDEXMICRO_BY_COLOR[StationColor.PURPLE],
    'yandexmicro_yellow': YANDEXMICRO_BY_COLOR[StationColor.YELLOW],
    'yandexmicro_pink': YANDEXMICRO_BY_COLOR[StationColor.PINK],
}
STATIONS = {
    'quasar',
    'yandexmax',
    'yandexmini',
    'yandexmidi',
    'dexp',
    'yandexmicro_beige',
    'yandexmicro_red',
    'yandexmicro_green',
    'yandexmicro_purple',
    'yandexmicro_yellow',
    'yandexmicro_pink',
}


def get_preset_attr(preset_key, attr_name):
    app_preset_obj = APP_PRESET_CONFIG.get(preset_key)
    return getattr(app_preset_obj, attr_name) if app_preset_obj else None
