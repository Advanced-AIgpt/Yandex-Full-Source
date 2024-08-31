# encoding: utf-8

from cofe.projects.alice.heartbeats.mappings import METRIKA_EVENT_MAPPING

from cofe.projects.alice.timespent.mappings import (
    TLT_TVT_SCENARIOS
)

"""
Маппинг получен ориентируясь на задачу https://st.yandex-team.ru/SDA-61
Предлагается ввести понятие об источнике смотрения. Список источников:
tv - антенное смотрение (dvbc / dvbs / dvbt / atv / ...)
webview - смотрение в webview из поиска
ott - сериалы и фильмы из КП
blogger - блоггеры из Я.VH
yandex_efir - лайв стримы и записи из Я.Эфирных каналов
my_efir - смотрение из персонального раздела Мой Эфир
tv_and_yaefir - до какого-то момента не были разделены tv и эфир, маппинг остался на всякий случай
"""

APPS_WITH_DEVICE_MODELS = {'quasar', 'small_smart_speakers'}

VIEW_SOURCE_TO_SCENARIO_MAPPING = {
    "tv" : "tv_channels",
    "webview": "video",
    "ott" : "video",
    "blogger": "video",
    "yandex_efir": "tv_stream",
    "my_efir": "tv_stream",
    "tv_and_yaefir": "tv_channels",
    "youtube": "youtube"
}

HEARTBEAT_EPS = 35.
HEARTBEAT_EPS_MS = 35000.
NOT_JOINED_MUSIC_SCENARIO = "unknown_music"
NOT_IN_TLT_TVT_MUSIC_SCENARIO = "other_music"


def get_age_category(child_confidence):
    CHILD_THRESHOLD = 0.8
    if child_confidence is not None:
        return 'child' if child_confidence >= CHILD_THRESHOLD else 'adult'
    return 'unknown'


def get_scenario(generic_scenario, music_genre, event_name=None):
    scenario = METRIKA_EVENT_MAPPING.get(event_name)
    if scenario == "multiroom":
        return scenario
    if (generic_scenario in TLT_TVT_SCENARIOS and scenario) or METRIKA_EVENT_MAPPING.get(event_name) == "music" or not scenario:
        scenario = generic_scenario
    if generic_scenario is None and METRIKA_EVENT_MAPPING.get(event_name) == "music":
        scenario = NOT_JOINED_MUSIC_SCENARIO
    return scenario


def ifnull(x, default):
    if x is not None:
        return x
    return default


def get_scenario_from_view_source(view_source):
    return VIEW_SOURCE_TO_SCENARIO_MAPPING.get(view_source, "empty")
