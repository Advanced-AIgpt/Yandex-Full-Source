TVANDROID_SESSIONS_DIR = '//home/smarttv/logs/tvandroid_sessions/1d'
TVANDROID_SESSIONS_PATH = '$tvandroid_sessions_dir/{{{start_date}..{end_date}}}'
CRYPTA_DEVICE_ID_CID_PATH = '//home/crypta/public/matching/by_id/mm_device_id/crypta_id'
CRYPTA_PUID_CID_PATH = '//home/crypta/public/matching/by_id/puid/crypta_id'
TVANDROID_DAYUSE_CUBE_DIR = '//home/sda/cubes/tv/dayuse'
ACTIVATIONS_CUBE_DIR = '//home/sda/cubes/tv/activations'
ACTIVATIONS_CUBE_PATH = '$activations_cube_dir/last'

DAYS_UPDATE_PERIOD = 2
DAYS_RECALC_PERIOD = 1

TVANDROID_SESSIONS_FIELDS = [
    'device_id',
    'puid',
    'uuid',
    'geo_id',
    'manufacturer',
    'model',
    'app_version',
    'session_id',
    'is_logged_in',
    'has_leased_module',
    'has_plus',
    'has_paid_plus',
    'active_tv_gift',
    'active_other_gift',
    'active_tv_gift_plus',
    'active_other_trial',
    'potential_gift',
    'event_timestamp',
    'diagonal',
    'resolution',
    'is_factory_ip',
    'build_fingerprint',
    'eth0',
    'wlan0',
    'mac_id',
    'buckets',
    'event_name',
    'event_value',
    'event_datetime',
    'quasar_device_id',
    'tandem_device_id',
    'tandem_connection_state',
]
DATETIME_FORMAT = '%Y-%m-%d %H:%M:%S'
DATE_FORMAT = '%Y-%m-%d'
MUSIC_HOME_APP_VERSION = '1.5'
UNDEFINED = 'undefined'
OPEN_EVENTS = [
    'vh_content_card_opened',
    'search_content_card_opened',
    'native_player_opened',
    'tv_player_opened',
    'webview_player_opened',
    'youtube_player_opened',
]
EVENT_MAP = {
    'tv_player_opened': 'tv',
    'tv_content_changed': 'tv',
    'tv_player_action': 'tv',
    'tv_channel_favorite_added': 'tv',
    'tv_channel_favorite_removed': 'tv',
    'serp_shown': 'search',
    'serp_opened': 'search',
    'collection_opened': 'search',
    'app_install_page_clicked': 'installed_apps'
}
SCREENS = {
    'search': {'type': 'outer'},
    'tv': {'type': 'outer'},
    'main': {'type': 'outer'},
    'movie': {'type': 'outer'},
    'series': {'type': 'outer'},
    'kids': {'type': 'outer'},
    'blogger': {'type': 'outer'},
    'music': {'type': 'outer'},
    'installed_apps': {'type': 'outer'},
    "native_player": {'type': 'inner'},
    "episode_content_card": {'type': 'inner'},
    "tv_player": {'type': 'inner'},
    "film_content_card": {'type': 'inner'},
    "show_more_items": {'type': 'inner'},
    "webview_player": {'type': 'inner'},
    "app_details": {'type': 'inner'},
    "search_collection_page": {'type': 'inner'},
    "search_content_card": {'type': 'inner'},
    "home": {'type': 'outer', 'name': 'main'},
    "section_rows": {'type': 'inner'},
    "section_grid": {'type': 'inner'},
    "special_event": {'type': 'inner'},
    "": {'type': 'unknown'},
    "unknown": {'type': 'unknown'},
}
HDMI_EVENTS = ['hdmi_opened', 'hdmi_heartbeat', 'hdmi_session_info']
HDMI_HEARTBEAT_OR_SESSION_INFO = ['hdmi_heartbeat', 'hdmi_session_info']

ADS_INVALID_DURATION = 2147483647

DATE_FORMAT = "%Y-%m-%d"
