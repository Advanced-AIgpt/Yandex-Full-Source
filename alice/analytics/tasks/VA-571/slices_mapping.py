# -*-coding: utf8 -*-
import re

# список срезов, использующихся на графиках с указанием относящихся к ним толокерским интентам (поле toloka_intent).
# для станции и пп/браузера/строки преобразования не отличаются
# (в корзине пп/браузера/строки некоторые из нижеперечисленных интентов могут отсутствовать)
SLICES_MAPPING = {
    "music": {  # музыка
        "action.app.music.genre",
        "action.app.music.radio",
        "action.app.music.yandex",
        "action.app.player.playlist.less",
        "action.app.player.playlist.more",
        "search.content.nature",
        "search.content.music.general",
        "search.content.music.genre",
        "search.content.music.listen",
        "search.content.podcasts",
        "search.content.radio.yandex",
        "search.music.find.album",
        "search.music.find.artist",
        "search.music.find.general",
        "search.music.find.genre",
        "search.music.find.other",
        "search.music.find.playlist",
        "search.music.find.song",
        "search.music.find.track",
        "search.music.info",
        "search.music.listen.album",
        "search.music.listen.artist",
        "search.music.listen.general",
        "search.music.listen.genre",
        "search.music.listen.other",
        "search.music.listen.playlist",
        "search.music.listen.song",
        "search.music.listen.track",
        "search.music.ringtone",
        "simple.recognize.music",
        "simple.what_track"
    },
    "radio": {  # радио
        "search.content.radio.fm",
        "action.app.music.fm"
    },
    "alarms_timers": {  # будильники и таймеры
        "action.time.alarm.cancel",
        "action.time.alarm.other",
        "action.time.alarm.setup",
        "action.time.alarm.show_list",
        "action.time.timer.cancel",
        "action.time.timer.pause",
        "action.time.timer.setup",
        "action.time.timer.show_list",
        "action.time.timer.unpause"
    },
    "video": {  # видео
        "action.app.video",
        "action.app.player.video.open_current_video",
        "action.app.player.video.payment_confirmed",
        "action.app.player.video.select_video_from_gallery",
        "search.content.video.not_porno",
        "search.content.film.find",
        "search.content.film.play",
        "search.content.live.tv_stream",
        "search.porno.video"
    },
    "weather": {  # погода
        "search.news_and_weather.weather",
        "search.news_and_weather.get_weather_nowcast"
    },
    # в болталку определяются запросы, имеющие VINS-овый интент regexp "general_conversation" и
    # толокерский интент из этого списка
    "toloka_gc": {
        "general_conversation.dialog",
        "general_conversation.feedback.negative.criticism",
        "general_conversation.feedback.negative.insult_general",
        "general_conversation.feedback.negative.insult_swearing",
        "general_conversation.feedback.positive",
        "other.insignificant.ok",
        "other.insignificant.other",
        "other.insignificant.thanks",
        "other.nonsense",
        "simple.what_you_can.general"
    },
    "translate": {  # перевод
        "action.translate",
        "translate"
    },
    "geo": {  # гео
        "action.app.maps",
        "search.geo.address.app",
        "search.geo.address.general",
        "search.geo.navi.distance",
        "search.geo.navi.route_general",
        "search.geo.navi.traffic_general",
        "search.geo.org.general"
    },
    "search": {
        "search.other",
        "search.porno.general"
    },
    "iot": {
        "action.iot_do"
    },
    "news": {
        "search.news_and_weather.news",
    },
    "timetables": {
        "search.timetable.cinema",
        "search.timetable.events",
        "search.timetable.other",
        "search.timetable.transport",
        "search.timetable.tv",
        "search.timetable.tv.sport",
        "search.timetable.tv.common",
        "search.timetable.tv.channel"
    },
}

SAMPLING_MAPPING = {
    "alarms_timers": {  # всё околобудильничное вместе
        "action.time.calendar",  # напоминания
        "action.time.stopwatch"  # секундомер
    },
    "fairytales": {
        "general_conversation.tell.fairytale"
    },
    "games_skills": {
        "simple.random_num",
        "general_conversation.game",
        "simple.what_you_can.games"
    },
    "geo": {
        "parking.find_parking",
        "route.calc.address",
        "route.calc.org",
        "route.calc.parking",
        "route.calc.pub_transport",
        "route.calc.somewhere",
        "route.calc.work",
        "route.faster",
        "search_on_map.coordinates",
        "search_on_map.other",
        "search_on_map.parking",
        "search_on_map.refuel",
        "search_on_map.toponym",
        "simple.go_home",
        "simple.remember_address"
    },
    "info_request": {
        "action.time.what_date",
        "action.time.what_time",
        "simple.where",
        "route.calc.timing",
        "route.info.how_long_to_drive",
        "route.info.other",
        "route.info.when_we_get_there",
        "search_on_map.home_work",
        "search_on_map.show_me.current",
        "search_on_map.show_me.history",
        "traffic.how_long_traffic_jam"
    },
    "market": {
        "search.market.commodity",
        "search.market.how_much",
        "search.market.service"
    },
    "nav_url": {
        "action.app.browser.general.open_default_browser",
        "action.app.browser.general.open_ya_browser",
        "action.app.browser.open_site",
        "action.app.maps",
        "action.app.maps.navi",
        "action.app.maps.yandexnavi",
        "action.app.messenger",
        "action.app.other_app"
    },
    "new_functionality": {  # такого сейчас нет в алисе, хотим чтоб они отранжировались между собой
        "action.app.music.other",
        "action.phone.call",
        "action.phone.message",
        "general_conversation.settings",
        "search.avia",
        "search.content.audiobooks",
        "search.content.audiobooks.general",
        "search.content.film.download",
        "search.content.other",
        "search.cook",
        "search.music.lyrics",
        "search.music.karaoke",
        "search.music.other_site_or_app",
        "simple.recognize.picture",
    },
    "ontofacts": {
        "search.content.film.info",
        "search.fact",
        "search.currency",
        "action.calc",
        "auto_facts.other",
        "auto_facts.fines",
        "search.geo.navi.time",
        "traffic.overall",
        "traffic.why"
    },
    "taxi": {
        "action.taxi.general.cancel",
        "action.taxi.general.confirm",
        "action.taxi.general.general",
        "action.taxi.general.order",
        "action.taxi.general.payment",
        "action.taxi.general.specify",
        "action.taxi.general.status",
        "action.taxi.general.support",
        "action.taxi.other.general",
        "action.taxi.other.order",
        "action.taxi.yandex.additional",
        "action.taxi.yandex.cancel",
        "action.taxi.yandex.confirm",
        "action.taxi.yandex.general",
        "action.taxi.yandex.order",
        "action.taxi.yandex.payment",
        "action.taxi.yandex.specify",
        "action.taxi.yandex.status",
        "action.taxi.yandex.support",
        "action.taxi.yandex.tariff",
        "simple.what_you_can.taxi"
    },
    "tell_something": {
        "general_conversation.tell.joke",
        "general_conversation.tell.other",
        "general_conversation.tell.poem",
        "general_conversation.tell.sing_song.general",
        "general_conversation.tell.sing_song.other",
        "general_conversation.tell.sing_song.own"

    }
}

GEO_DETAILED_MAPPING = {
    "org": {
        "parking.find_parking",
        "route.calc.org",
        "route.calc.parking",
        "search.geo.org.general",
        "search_on_map.parking",
        "search_on_map.refuel"
    },
    "toponym": {
        "route.calc.address",
        "search.geo.address.general",
        "search_on_map.coordinates",
        "search_on_map.toponym"
    },
    "geo_other": {
        "route.calc.pub_transport",
        "route.calc.somewhere",
        "route.calc.work",
        "route.faster",
        "search.geo.address.app",
        "search.geo.navi.distance",
        "search.geo.navi.route_general",
        "search.geo.navi.traffic_general",
        "search_on_map.other",
        "simple.go_home",
        "simple.remember_address"
    }
}

# нельзя объединять в general_intent, т.к. есть пересечения по интентам
# выделен как срез, на котором должна быть точность > 90
COMMANDS = {
    "action.app.maps.close_navi",
    "action.app.maps.close_yandexnavi",
    "action.app.music.play_pause",
    "action.app.browser.bookmarks.open_bookmarks_manager",
    "action.app.browser.general.close_default_browser",
    "action.app.browser.general.close_ya_browser",
    "action.app.browser.go_home",
    "action.app.browser.history.open_history",
    "action.app.browser.history.clear_history",
    "action.app.browser.tabs.close_tab",
    "action.app.browser.tabs.new_tab",
    "action.app.browser.tabs.open_incognito_mode",
    "action.app.player.order.repeat",
    "action.app.player.order.shuffle",
    "action.app.player.order.straight",
    "action.app.player.playlist.dislike",
    "action.app.player.playlist.like",
    "action.app.player.play_pause.continue",
    "action.app.player.play_pause.pause",
    "action.app.player.play_pause.replay",
    "action.app.player.rewind.next",
    "action.app.player.rewind.prev",
    "action.app.player.rewind.rewind_absolute",
    "action.app.player.rewind.rewind_relative",
    "action.app.quasar.go_backward",
    "action.app.quasar.go_forward",
    "action.app.quasar.go_home",
    "action.app.quasar.go_to_the_beginning",
    "action.app.quasar.go_to_the_end",
    "action.time.alarm.cancel",
    "action.time.timer.cancel",
    "action.pc.bluetooth_off",
    "action.pc.bluetooth_on",
    "action.pc.open.open_disk",
    "action.pc.open.open_flash_card",
    "action.pc.open.open_file",
    "action.pc.open.open_folder",
    "action.pc.search_local",
    "action.pc.settings",
    "action.pc.stop.hibernate",
    "action.pc.stop.power_off",
    "action.pc.stop.restart_pc",
    "general_conversation.feedback.positive",
    "other.insignificant.thanks",
    "other.stop",
    "parking.show",
    "point.add.accident",
    "point.add.camera",
    "point.add.comment",
    "point.add.error",
    "point.add.no_type",
    "point.add.other",
    "point.add.road_works",
    "point.hide.camera",
    "point.show.accident",
    "point.show.camera",
    "route.reset.current",
    "route.reset.history",
    "route.select",
    "route.show",
    "save_place.current_position",
    "save_place.poi",
    "settings.change_voice",
    "settings.map.zoom",
    "settings.other",
    "settings.satellite.show",
    "settings.sound.mute",
    "settings.sound.unmute",
    "settings.sound.volume_down",
    "settings.sound.volume_level",
    "settings.sound.volume_up",
    "settings.sound.other",
    "settings.sound.equalizer",
    "traffic.show",
    "traffic.street"
}

CLOSE_TO_VIDEO = {"search.timetable.tv.channel", "search.content.film.info"}
SIMPLE_MUSIC_QUERY = re.compile('^((алиса )?(((включ(и|(ай)))|(играй)) )?(пожалуйста )?музык(у|а)(((включ(и|(ай)))|(играй)) )?(пожалуйста )?(((включ(и|(ай)))|(играй)) )?( алиса)?)$')

# интенты, в которых могут быть запросы про актуальное сейчас. не включаем интенты, в которых могут быть только формулировки с "сегодня"
FRESH_SAMPLING_TOLOKA_INTENTS = {"news", "iot", "timetables", "weather"}


def toloka_intent_to_general_intent(toloka_intent):
    if toloka_intent:
        for general_intent, intents in SLICES_MAPPING.items():
            if toloka_intent in intents:
                return general_intent
    return "other"


def is_video_related_intent(toloka_intent):
    general_intent = toloka_intent_to_general_intent(toloka_intent)
    return toloka_intent in CLOSE_TO_VIDEO or general_intent == "video"


def is_command_intent(toloka_intent):
    return toloka_intent in COMMANDS


def toloka_intent_to_sampling_intent(toloka_intent, split_geo=False):
    if toloka_intent:
        if split_geo:
            for general_intent, intents in GEO_DETAILED_MAPPING.items():
                if toloka_intent in intents:
                    return general_intent
        for general_intent, intents in SAMPLING_MAPPING.items():
            if toloka_intent in intents:
                return general_intent
        for general_intent, intents in SLICES_MAPPING.items():
            if toloka_intent in intents:
                return general_intent
        if toloka_intent in COMMANDS:
            return "commands"
    return "other"


# Базовый способ определения свежих запросов. Работает тупо по толокерским интентам, в которых потенциально есть свежие запросы.
def is_fresh_query(toloka_intent):
    return toloka_intent_to_sampling_intent(toloka_intent) in FRESH_SAMPLING_TOLOKA_INTENTS


# лучше места не придумала :(
def is_good_music_query(query, answer, generic_scenario):
    match = re.search(SIMPLE_MUSIC_QUERY, query)
    return match and generic_scenario == "music" and not ("что-то пошло не так" in answer or answer is None)
