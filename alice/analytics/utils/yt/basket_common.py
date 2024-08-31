# -*-coding: utf8 -*-
import base64
import random
from datetime import datetime
from collections import OrderedDict


BASS_OPTIONS = {'client_ip', 'filtration_level', 'region_id'}

# Наборы app'ов и app_preset'ов, по которым происходят фильтрации по группам поверхностей
# Используются:
#   * в выпадушке ue2e app_type: quasar/general/navi/tv для релизов ASR
#   * в ue2e relevance сплите по поверхностям
#   * в скриптах сбора больших корзин

QUASAR_APPS = {
    # apps + app_presets:
    "quasar", "small_smart_speakers",

    # app_presets:
    "yandexmini", "yandexmidi", "yandexmax",
    "yandexmicro_beige", "yandexmicro_red", "yandexmicro_green",
    "yandexmicro_purple", "yandexmicro_yellow", "yandexmicro_pink",
    "dexp",
}

GENERAL_APPS = {
    # apps + app_presets:
    "search_app_prod", "search_app_beta",
    "browser_prod", "browser_alpha", "browser_beta",
    "yabro_prod", "yabro_beta",
    "stroka", "launcher",
    "iot_app",

    # app_presets:
    "search_app_ios", "search_app_ipad",
    "browser_prod_ios",
    "iot_app_prod_ios", "iot_app_prod_android",
}

NAVI_APPS = {
    # apps + app_presets:
    "navigator", "auto",
    "yandexmaps_prod", "yandexmaps_dev",

    # app_presets:
    "maps",
}

TV = {
    # apps + app_presets:
    "tv",

    # app_presets:
    "module_2"
}

# тикет https://st.yandex-team.ru/ATAN-388
TO_MUSIC_PERCENT = {"10"}

BAD_FLAGS = {'context_load_apply', 'websearch_bass_music_cgi_rearr', 'websearch_cgi_rearr'}

FILTRATIONS = {
    0: 'without',
    1: 'medium',
    2: 'children',
    3: 'safe',
}

CHILD_DEFAULT = 'children'
ADULT_DEFAULT = 'medium'


IOT_DEVICES_EXCEPTIONS = {
    'devices.types.hub',  # Пульт
    'devices.types.media_device.dongle.yandex.module'  # Модуль
}


SMART_SPEAKER_DEVICES_PREFIX = 'devices.types.smart_speaker'
TV_DEVICE_TYPE = 'devices.types.media_device.tv'
QUASAR_SKILL_ID = 'Q'


def get_path(data, path, default=None):
    try:
        for item in path:
            data = data[item]
        return data
    except (KeyError, TypeError, IndexError):
        return default


def get_apps_by_type(app_type):
    """
    Возвращает список app'ов для заданного типа поверхностей
    :param str app_type:
    :return list:
    """
    if app_type == 'quasar':
        return list(QUASAR_APPS)
    if app_type == 'general':
        return list(GENERAL_APPS)
    if app_type == 'navi_auto':
        return list(NAVI_APPS)
    if app_type == 'tv':
        return list(TV)
    if app_type == 'all':
        return list(QUASAR_APPS) + list(GENERAL_APPS) + list(NAVI_APPS)
    raise Exception('app_type must be in: all, quasar, general, navi, tv')


def get_basket_schema():
    from qb2.api.v1 import typing as qt

    return OrderedDict([
        ("mds_key", str),
        ("device_state", qt.Json),
        ("location", qt.Json),
        ("client_time", str),
        ("timezone", str),
        ("voice_url", str),
        ("fetcher_mode", str),
        ("vins_intent", str),
        ("app_preset", str),
        ("asr_text", str),
        ("text", str),
        ("real_reqid", str),
        ("real_uuid", str),
        ("request_id", str),
        ("real_session_id", str),
        ("session_id", str),
        ("session_sequence", int),
        ("reversed_session_sequence", int),
        ("experiments", qt.Json),
        ("request_source", str),
        ("toloka_intent", str),
        ("asr_options", qt.Json),
        ("additional_options", qt.Json)
    ])


def get_filtration_level(device_state, additional_options, childness='adult'):
    filtration_level = get_path(additional_options, ['bass_options', 'filtration_level'])
    content_settings_info = 'medium'
    if childness == 'adult':
        content_settings_info = get_path(device_state, ['device_config', 'content_settings'], ADULT_DEFAULT)
    if childness == 'child':
        content_settings_info = get_path(device_state, ['device_config', 'child_content_settings'], CHILD_DEFAULT)
    if filtration_level and FILTRATIONS[filtration_level] == 'safe' or content_settings_info == 'safe':
        return 'safe'
    if filtration_level and FILTRATIONS[filtration_level] == 'children' or content_settings_info == 'children':
        return 'children'
    if filtration_level and FILTRATIONS[filtration_level] == 'without' or content_settings_info == 'without':
        return 'without'
    return 'medium'


def random_part(i):
    return ''.join(map(lambda x: random.choice("0123456789abcdef"), range(i)))


def generate_id(req_id=None):
    return ''.join(['ffffffff-ffff-ffff-', random_part(4), '-', random_part(12)])


def get_session_ids(groups):
    from nile.api.v1 import Record

    for key, records in groups:
        req_ids = []
        for record in records:
            rec = record.to_dict()
            req_ids.append((rec['new_req_id'], rec['client_time']))
        req_ids = [x[0] for x in sorted(req_ids, key=lambda x: x[1])]
        if len(req_ids) > 1:
            req_ids = [req_ids[0]] + [req_ids[-1]]
        yield Record(main_req_id=key.main_req_id, new_session_id='__'.join(req_ids))


def get_session_sequence(ss, ss_last, session_id, reverse=False):
    if ss != ss_last:
        return 0 if not reverse else 1
    if len(session_id.split('__')) == 1:
        return 0
    return 1 if not reverse else 0


def leave_experiments_for_context(experiments, main_req_id, current_reqid, reply, generic_scenario):
    """
    Оставляет эксперименты только для запросов-контекстов (для максимальной похожести на то, что когда-то было в логах)
    В основном запросе в сессии — экспериментальных флагов не должно быть
    Фильтрует плохие эксперименты по белому списку
    """
    if main_req_id != current_reqid:
        experiments = dict(experiments)
        if generic_scenario in ("general_conversation", "external_skill_gc"):
            experiments = patch_experiments_for_gc(experiments, reply)
        experiments = filter_bad_flags(experiments, BAD_FLAGS)
        return experiments
    return None


def get_flag_for_text(t):
    return "hw_gc_mocked_reply={}".format(base64.b64encode(t).decode("utf-8"))


def patch_experiments_for_gc(experiments, reply):
    """
    fix context for general_conversation using reply from logs
    """
    if not experiments:
        experiments = {}
    experiments["mm_scenario=GeneralConversation"] = "1"
    experiments[get_flag_for_text(reply)] = "1"
    return experiments


def filter_bad_flags(experiments, flags_to_filter):
    """
    Удаляет плохие флаги из dict experiments
    """
    if experiments:
        for exp in list(experiments.keys()):
            if any([exp and flag in exp for flag in flags_to_filter]):
                del experiments[exp]

    return experiments


#  форсируем текстовое простукивание для запросов контекста
def get_fetcher_mode(input_type, main_req_id, current_reqid):
    if main_req_id != current_reqid:
        return "text"
    return input_type


def add_audio_player(state):
    if state and state.get("music") and not state.get("audio_player"):
        music = state.get("music")
        audio_player = {}

        currently_playing_last_play_timestamp = get_path(music, ['currently_playing', 'last_play_timestamp'])
        currently_playing_track_id = get_path(music, ['currently_playing', 'track_id'])
        track_info_track_id = get_path(music, ['currently_playing', 'track_info', 'id'])
        track_info_title = get_path(music, ['currently_playing', 'track_info', 'title'])
        artist_name = get_path(music, ['currently_playing', 'track_info', 'artists', 0, 'name'])
        track_info_duration = get_path(music, ['currently_playing', 'track_info', 'durationMs'])
        paused = get_path(music, ['player', 'pause']) or False

        audio_player["current_stream"] = {}
        if currently_playing_last_play_timestamp:
            audio_player["current_stream"]["last_play_timestamp"] = currently_playing_last_play_timestamp + 1
        if music.get("last_play_timestamp"):
            audio_player["current_stream"]["last_play_timestamp"] = music["last_play_timestamp"] + 1
        if currently_playing_track_id:
            audio_player["current_stream"]["stream_id"] = currently_playing_track_id
        if track_info_track_id:
            audio_player["current_stream"]["stream_id"] = track_info_track_id
        if track_info_title:
            audio_player["current_stream"]["title"] = track_info_title
        if artist_name:
            audio_player["current_stream"]["subtitle"] = artist_name

        if track_info_duration:
            audio_player["duration_ms"] = track_info_duration
        else:
            audio_player["duration_ms"] = 180000
        if music.get("last_play_timestamp"):
            audio_player["last_play_timestamp"] = music["last_play_timestamp"] + 1

        audio_player["last_stop_timestamp"] = 0 if not paused else (music.get("last_play_timestamp") +
                                                                    3000 if music.get("last_play_timestamp") else 0)
        audio_player["offset_ms"] = 0
        audio_player["played_ms"] = 0
        audio_player["player_state"] = "Stopped" if paused else "Playing"
        audio_player["scenario_meta"] = {
            "owner": "music"
        }
        if artist_name:
            audio_player["scenario_meta"]["what_is_playing_answer"] = \
                artist_name + ', песня \"' + track_info_title + '\"'
        else:
            audio_player["scenario_meta"]["what_is_playing_answer"] = track_info_title

        state["audio_player"] = audio_player
    return state


def patch_device_state(device_state, app='quasar'):
    if device_state and 'device_id' in device_state:
        del device_state['device_id']
    if app in QUASAR_APPS:
        device_state = add_audio_player(device_state)
    return device_state


def get_asr_text(fetcher_mode, query):
    if fetcher_mode == 'voice':
        return query
    return None


def get_app_preset_from_app(app, device=None):
    """
    Маппинг
    * из app из prepared-logs-expboxes https://a.yandex-team.ru/arcadia/alice/wonderlogs/sdk/core/getters.cpp?rev=r9716883#L60
    * в app_preset для ue2e прокачек https://a.yandex-team.ru/arcadia/alice/acceptance/modules/request_generator/lib/app_presets.py?rev=r9677616#L981
    """
    if device:
        if device == "Yandex yandexmidi":
            return "yandexmidi"
        if device == "Yandex Station_2":
            return "yandexmax"
        if device == "Yandex yandexmini":
            return "yandexmini"
        if device == "Apple iPhone":
            if app.startswith("browser"):
                return "browser_prod_ios"
            return "search_app_ios"
        if device == "Apple iPad":
            if app.startswith("browser"):
                return "browser_prod_ios"
            return "search_app_ipad"
        if app == "iot_app" and "Apple" in device:
            return "iot_app_prod_ios"
        if device == "Yandex yandexmodule_2":
            return "module_2"
    if app in ("tr_navigator", "yandexmaps_prod", "yandexmaps_dev"):
        return "navigator"
    if app == "yandex_phone":
        return "launcher"
    if app == "alice_app":
        return "search_app_prod"
    if app == "iot_app":
        return "iot_app_prod_android"
    return app


def clean_additional_options(additional_options):
    if not additional_options:
        return {}
    bass_options = additional_options.get("bass_options", {})
    cleaned_options = {}
    for bass_option in bass_options:
        if bass_option in BASS_OPTIONS:
            if not cleaned_options:
                cleaned_options = {'bass_options': {}}
            cleaned_options["bass_options"][bass_option] = bass_options[bass_option]
    return cleaned_options


def get_common_filters():
    from qb2.api.v1 import filters as qf
    return [
        qf.or_(qf.not_(qf.defined('do_not_use_user_logs')), qf.not_(qf.nonzero('do_not_use_user_logs'))),
        qf.not_(qf.or_(
            qf.startswith('uuid', 'uu/ffffffffffffffff'),
            qf.startswith('uuid', 'uu/deadbeef'),
            qf.startswith('uuid', 'uu/dddddddddddddddd')
        )),
    ]


def get_date_from_ts(ts):
    return datetime.utcfromtimestamp(ts).strftime('%Y-%m-%d')


def is_new_iot_user(analytics_info, fielddate):

    def is_valid_device(device_info):
        return device_info.get("created") \
            and not device_info.get("analytics_type", "").startswith(SMART_SPEAKER_DEVICES_PREFIX) \
            and device_info.get("analytics_type", "") not in IOT_DEVICES_EXCEPTIONS \
            and not (device_info.get('analytics_type') == TV_DEVICE_TYPE
                     and device_info.get('skill_id') == QUASAR_SKILL_ID)

    if analytics_info.get("iot_user_info") and analytics_info["iot_user_info"].get("devices"):
        old_devices_types = set()

        for device_info_ in analytics_info["iot_user_info"]["devices"]:
            if is_valid_device(device_info_) and get_date_from_ts(device_info_["created"]) < fielddate:
                old_devices_types.add(device_info_["analytics_type"])

        for device_info_ in analytics_info["iot_user_info"]["devices"]:
            if is_valid_device(device_info_) and get_date_from_ts(device_info_["created"]) == fielddate \
                    and device_info_["analytics_type"] not in old_devices_types:
                return True
    return False


def fix_click_input_type(input_type, query):
    if input_type == 'click' and query:
        return 'text'
    return input_type


def format_client_time(client_time):
    """
    По UTC таймстемпу формирует клиентское время в формате прокачек
    """
    from pytz import timezone
    return datetime.fromtimestamp(client_time, tz=timezone('UTC')).strftime("%Y%m%dT%H%M%S")


def get_basket_fields(do_not_change_ss=False):
    from qb2.api.v1 import extractors as qe, typing as qt

    return [
        qe.log_fields(
            'mds_key',
            'location',
            'voice_url',
            'request_source',
            'toloka_intent',
            'additional_options',
            'iot_config',
        ),
        qe.log_fields(
            'query', 'input_type', 'app', 'req_id',
            'main_req_id', 'new_session_id', 'main_session_sequence'
        ).hide(),
        qe.log_field('experiments').rename('request_experiments').with_type(qt.Optional[qt.Json]).hide(),
        qe.log_field('device_state').rename('request_device_state').hide(),
        qe.log_field('reply').hide(),
        qe.log_field('device').hide(),
        qe.log_field('generic_scenario').hide(),
        qe.log_field('session_sequence').rename('request_ss').hide(),
        qe.log_field('reversed_session_sequence').rename('request_rss').hide(),
        qe.log_field('client_time').rename('ts').hide(),
        qe.log_field('client_tz').rename('timezone'),
        qe.log_field('intent').rename('vins_intent'),
        qe.log_field('query').rename('text'),
        qe.log_field('req_id').rename('real_reqid'),
        qe.log_field('new_req_id').rename('request_id'),
        qe.log_field('session_id').rename('real_session_id'),
        qe.log_field('new_session_id').rename('session_id'),
        qe.custom('client_time', format_client_time, 'ts').with_type(qt.Optional[qt.String]),
        qe.custom('device_state', patch_device_state, 'request_device_state', 'app').with_type(qt.Optional[qt.Json]),
        qe.custom('app_preset', get_app_preset_from_app, 'app', 'device').with_type(qt.Optional[qt.String]),
        qe.custom('asr_text', get_asr_text, 'input_type', 'query').with_type(qt.Optional[qt.String]),
        qe.custom('fetcher_mode', get_fetcher_mode, 'input_type', 'main_req_id', 'real_reqid').with_type(
            qt.Optional[qt.String]),
        qe.custom('session_sequence',
                  lambda x, y, z: x if do_not_change_ss else get_session_sequence(x, y, z),
                  'request_ss', 'main_session_sequence', 'session_id').with_type(qt.Optional[qt.Int64]),
        qe.custom('reversed_session_sequence',
                  lambda x, y, z, r: r if do_not_change_ss else get_session_sequence(x, y, z, reverse=True),
                  'request_ss', 'main_session_sequence', 'session_id', 'request_rss').with_type(qt.Optional[qt.Int64]),
        qe.custom('experiments', leave_experiments_for_context, 'request_experiments', 'main_req_id', 'real_reqid',
                  'reply', 'generic_scenario').with_type(qt.Optional[qt.Json]),
        qe.yql_custom('asr_options', 'Yson::Serialize(Yson::ParseJson(@@{"allow_multi_utt": true}@@))'),
    ]
