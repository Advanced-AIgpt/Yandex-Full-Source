TVANDROID_SESSIONS_DIR = '//home/smarttv/logs/tvandroid_sessions/1d'
TVANDROID_SESSIONS_PATH = '$tvandroid_sessions_dir/{{{start_date}..{end_date}}}'
STRM_DIR = '//home/smarttv/logs/strm/1d'
STRM_PATH = '$strm_dir/{{{start_date}..{end_date}}}'
CONTENT_INFO_PATH = '//home/sda/cubes/tv/content_info/last'
WATCHING_CUBE_DIR = '//home/sda/cubes/tv/watching'
WATCHING_CUBE_PATH = '$watching_cube_dir/{dates}'

DAYS_UPDATE_PERIOD = 7

APPMETRICA_WATCHING_LOG_FIELDS = [
    'device_id', 'session_id', 'manufacturer', 'model', 'app_version', 'diagonal',
    'resolution', 'fielddate', 'board', 'build', 'firmware_version', 'platform',
    'view_source', 'clid1', 'clid100010', 'channel_name', 'channel_id', 'url',
    'channel_type', 'player_session_id', 'test_buckets', 'is_logged_in', 'puid', 'active_tv_gift', 'has_plus',
    'tandem_connection_state', 'tandem_device_id'
]

STRM_WATCHING_LOG_FIELDS = [
    'device_id', 'session_id', 'manufacturer', 'model', 'clid1', 'clid100010', 'app_version',
    'channel_type', 'board', 'build', 'firmware_version', 'platform', 'diagonal', 'resolution',
    'fielddate', 'monetization_model', 'channel_name', 'player_session_id', 'view_type',
    'view_source', 'content_type', 'test_buckets', 'carousel_name', 'carousel_position',
    'content_card_position', 'tv_screen', 'tv_parent_screen', 'is_logged_in',
    'content_id', 'puid', 'parent_id', 'tandem_connection_state', 'tandem_device_id'
]
