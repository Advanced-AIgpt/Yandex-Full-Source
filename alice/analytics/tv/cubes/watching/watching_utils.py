import hashlib

RECOMMENDATION_VIEW_SOURCES = frozenset(['ott', 'blogger'])
RECOMMENDATION_TV_SCREENS = frozenset(['main', 'movie', 'series', 'kids', 'blogger', 'special_event'])


def check_new_app_version(app_version):
    if app_version is not None:
        return '@' in app_version
    return False


def get_appmetrica_view_type(event_name, app_version, event_value, is_new_app_version):
    try:
        if event_name == 'youtube_player_heartbeat':
            return 'youtube'
        elif event_name == 'webview_player_heartbeat':
            return 'webview'
        elif is_new_app_version and event_value['channel_type'] is None:
            return 'null_app_version'
        elif is_new_app_version and event_value['channel_type'] != 'vh':
            return 'tv'
        elif is_new_app_version and event_value['channel_type'] == 'vh':
            return 'metrika_yaefir'
        elif not is_new_app_version and event_name == 'tv_player_heartbeat':
            return 'tv_and_yaefir'
        else:
            return 'undefined'
    except KeyError:
        return 'undefined'


def get_strm_view_type(channel_type, content_type, view_type, is_new_app_version):
    if channel_type == 'ott' and content_type == 'native' and view_type == 'vod':
        return 'ott'
    elif channel_type == 'other_vh' and content_type == 'native':
        return 'blogger'
    elif is_new_app_version and channel_type == 'other_vh' and content_type == 'tv':
        return 'yandex_efir'
    elif channel_type == 'other_vh' and content_type == 'tv':
        return 'old_app_ya_efir_strm'
    elif content_type == 'my_efir':
        return 'my_efir'
    else:
        return 'undefined'


def get_player_session_id(event_value, id_type, session_id, url):
    try:
        if id_type in ('tv', 'tv_and_yaefir') or (id_type == 'webview' and 'player_session_id' in event_value):
            return event_value['player_session_id']
        elif id_type == 'webview' and session_id and url:
            return hashlib.md5('_'.join([session_id, url])).hexdigest()
        else:
            return None
    except KeyError:
        return None


def check_recommendation_watching(view_source, tv_screen, tv_parent_screen):
    if view_source in RECOMMENDATION_VIEW_SOURCES:
        if tv_screen in RECOMMENDATION_TV_SCREENS or (tv_screen == 'show_more_items' and tv_parent_screen in RECOMMENDATION_TV_SCREENS):
            return True

    return False


def get_login_status(puid):
    if puid is not None and puid != '0':
        return True
    return False
