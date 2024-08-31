# encoding: utf-8
import re


MULTI_INTENT_KEYS = {
    "alarm", "get_weather", "image_search", "market", "radio", "reminder", "taxi", "timer", "todo"
}

MULTI_INTENT_KEY_REMAP = {
    # я, честное слово, не знаю, откуда у меня появилась лишняя s при написании маппинга, но очень не хочется пересчитывать все логи из-за этого :(
    "image_what_is_this": "images_what_is_this",
    "image_search": "images_what_is_this"
}

MM_MULTI_SCENARIOS = {
    "quasar", "sound", "player", "navi"
}

ONBOARDING_EXCEPTIONS = {
    "games_onboarding"
}

MM_SCENARIO_EXCEPTIONS = {
    "alice.vins", "Vins", "Wizard", "Search"
}

MM_PRODUCT_SCENARIO_EXCEPTIONS = {
    "alice.vins", "Vins"
}

SEARCH_PLACEHOLDERS_EXCEPTIONS = {
    "anaphoric", "factoid_src", "factoid_call", "map_search_url", "map_search_url_anaphoric"
}

# main_part -> action to grep in str -> mapping
ACTION_EXCEPTIONS = {
    "dialogovo": [{
        "actions": {"skill_recommendation"},
        "scenario": "onboarding"
    }],
    "video_general_scenario": [{
        "actions": {"open_current_video", "payment_confirmed", "select_video_from_gallery"},
        "scenario": "video_commands"
    }],
    "video": [{
        "actions": {"open_current_video", "payment_confirmed", "select_video_from_gallery"},
        "scenario": "video_commands"
    }],
    "general_conversation": [
        {
            "actions": ["external_skill_gc", "pure_general_conversation_deactivation"],
            "scenario": "external_skill_gc"
        },
        {
            "actions": ["gc_feedback"],
            "scenario": "feedback"
        }
    ],
    "handcrafted": [
        {
            "actions": {"cancel", "fast_cancel"},
            "scenario": "stop"
        },
        {
            "actions": {"feedback"},
            "scenario": "feedback"
        }
    ],
    "quasar": [
        {
            "actions": {"open_current_video", "payment_confirmed", "select_video_from_gallery"},
            "scenario": "video_commands"
        },
        {
            "actions": {"goto_video_screen"},
            "scenario": "video"
        }
    ],
    "market": [{
        "actions": {"beru_order"},
        "scenario": "beru"
    }],
    "music": [
        {
            "actions": {
                "meditation", "morning_show", "music_ambient_sound", "music_fairy_tale", "music_podcast", "music_what_is_playing"
            }
        }
    ]
}

INTENT_TO_SCENARIO_MAP = {
    "alarm": {
        "alarm_ask_sound", "alarm_ask_time", "alarm_cancel", "alarm_fast_snooze", "alarm_how_long", "alarm_how_to_set_sound", "alarm_reset_sound",
        "alarm_set", "alarm_set_sound", "alarm_set_with_sound", "alarm_show", "alarm_snooze", "alarm_snooze_abs", "alarm_snooze_rel",
        "alarm_stop_playing", "alarm_what_sound_is_set"
    },
    "beru": {
        "market_beru", "market_beru_beru", "market_beru_native", "market_beru_my_bonuses_list", "market_native_beru", "market_orders_status", "recurring_purchase"
    },
    "bluetooth": {"bluetooth_off", "bluetooth_on"},
    "commands_other": {  # сюда попали все специфичные для устройств команды
        "auto", "automotive", "stroka", "quasar", "internal", "other",
        "browser_read_page", "browser_read_page_continue", "browser_read_page_pause",
        "connect_named_location_to_device", "remember_named_location"  # команды для запоминания локации устройства (станции) и запоминания дом/работа (ПП, браузер)
    },
    "covid": {"covid_base"},
    "dialogovo": {"dialogovo", "external_skill"},
    "external_skill_gc": {
        "pure_general_conversation", "pure_general_conversation_deactivation",
        "pure_general_conversation_phone_call", "pure_general_conversation_what_can_you_do"
    },
    "feedback": {"player_dislike", "player_like"},  # всё, что фидбек, мапится сюда автоматически
    "find_poi": {"findpoi"},  # wrong form_name used in January
    "game_advise": {"game_suggest"},
    "general_conversation": {
        "handcrafted", "general_conversation"
    },
    "how_much": {"market_how_much"},
    "identity_commands": {
        "set_my_name", "voiceprint_enroll", "voiceprint_remove"
    },
    "info_request": {"get_time", "get_my_location", "what_is_my_name", "battery_power_state"},
    "images_what_is_this": {
        "image_what_is_this", "image_what_is_this_barcode", "image_what_is_this_clothes",
        "image_what_is_this_frontal", "image_what_is_this_frontal_similar_people", "image_what_is_this_market",
        "image_what_is_this_similar", "image_what_is_this_similar_artwork", "image_what_is_this_similar_people",
        "image_what_is_this_ocr", "image_what_is_this_ocr_voice", "image_what_is_this_office_lens",
        "image_what_is_this_translate", "onboarding_image_search"
    },
    "inner_events": {
        "demomobile2019_close_trunk", "demomobile2019_close_windows", "demomobile2019_horn",
        "demomobile2019_lock_car", "demomobile2019_off", "demomobile2019_on", "demomobile2019_open_trunk",
        "demomobile2019_open_windows", "demomobile2019_unlock_car", "demomobile2019_blink",
        "demoaurus2019_close_blinds", "demoaurus2019_close_right_window", "demoaurus2019_close_windows",
        "demoaurus2019_open_blinds", "demoaurus2019_open_right_window", "demoaurus2019_open_windows"
    },
    "iot_do": {"iot", "io_t"},
    "market": {
        "market_native", "market_product_details"
    },
    "movie_advise": {"movie_suggest"},
    "music": {
        "music_play", "music_play_anaphora", "music_play_less", "music_play_more", "hardcoded_music", "music_sing_song"
    },
    "nav_url": {"open_site_or_app", "nav_url"},
    "navi_commands": {
        "show_on_map", "switch_layer", "add_point", "navi"
    },
    "onboarding": {
        # игры отнесла сюда по аналогии с https://a.yandex-team.ru/arc/trunk/arcadia/statbox/abt/metrics/statbox_abt_metrics/metrics/alice/scenario_common.py?rev=6080516#L190
        "skill_recommendation", "relevant_skills", "skill_discovery", "skills_discovery", "skill_discovery_ru",
        "onboarding", "what_can_you_do", "games_onboarding"
    },
    "ontofacts": {"factoid", "entity_name", "calculator", "object", "film_gallery"},
    "placeholders": {
        "test_effectful_scenario", "zerotesting", "zero_testing", "123", "goodwin_debug",  # тестовые сценарии
        "ether_show", "sim_sim", "simsim_open", "simsim_gold",  # мордовийные тестовые сценарии
        "do_not_understand",  # выглядит как заглушка в авто на команды, которые Алиса не может или не хочет выполнять
        "messaging", "teach_me", "common"
    },
    "player_commands": {
        "player_continue", "player_replay", "player_rewind", "player_shuffle",
        "player_next_track", "player_pause", "player_previous_track", "player_repeat",
    },
    "promo": {"hny"},
    "radio": {"radio_play"},
    "random_number": {"random_num", "hw_random_number"},
    "reminder": {"alarm_reminder", "create_reminder", "list_reminders"},
    "route": {"show_route", "reset_route"},  # обсуждаемо. Не уверена, что сброс маршрута тоже к маршрутам стоит добавлять
    "serp": {
        "image_serp",  # переход на серп по клику в саджесте
        "serp",
        "serp_gallery"  # это был ux с показом результатов внутри ассистента
    },
    "search": {"websearch"},
    "search_commands": {
        "search_filter_how", "search_filter_set_family", "search_filter_set_no_filter", "search_filter_reset", "search_filter_get"
    },
    "shopping_list": {
        "shopping_list_add", "shopping_list_delete_all", "shopping_list_delete_item",
        "shopping_list_login", "shopping_list_show"
    },
    "sleep_timer": {"sleep_timer_set", "sleep_timer_time_left", "sleep_timer_how_long"},
    "sound_commands": {
        "sound_get_level", "sound_louder", "sound_mute", "sound_quiter",
        "sound_set_level", "sound_unmute", "sound"
    },
    "taxi": {
        "taxi_cancel", "taxi_new_after_order_actions", "taxi_new_call_to_driver", "taxi_new_call_to_driver_internal",
        "taxi_new_call_to_support", "taxi_new_cancel", "taxi_new_disabled", "taxi_new_open_app", "taxi_new_order",
        "taxi_new_show_driver_info", "taxi_new_show_legal", "taxi_new_status", "taxi_new_status_address", "taxi_order"
    },
    "timer": {"timer_cancel", "timer_pause", "timer_resume", "timer_set", "timer_show", "timer_stop_playing", "timer_how_long"},
    "todo": {"create_todo", "list_todo", "cancel_todo"},
    "translate": {"translate", "translate_stub"},  # второе - это какой-то редирект в переводчик, который актуален только до 04.2019, дальше в логах срабатываний не вижу
    "video": {"video_play", "video_general_scenario", "video_play_free", "video_play_paid"},
    "video_commands": {"ether"},
    "weather": {"get_weather", "get_weather_nowcast"},
}

AUDIOBOOK_GENRES = {
    "actionandadventure",
    "audiobooks",
    "audiobooksinenglish",
    "biographyandmemoirs",
    "booksnotinrussian",
    "community",
    "crimeandmystery",
    "dramaliterature",
    "fantasyliterature",
    "fiction",
    "historicalfiction",
    "hls",
    "horrorandthrillers",
    "nonfictionliterature",
    "popularsciencebooks",
    "psychologyandphilosophy",
    "religionandspirituality",
    "romancenovel",
    "sciencefiction",
    "selfdevelopment",
    "spoken",
    "technologies",
    "work",
}

PODCAST_GENRES = {
    "comedypodcasts",
    "familypodcasts",
    "filmpodcasts",
    "historypodcasts",
    "musicpodcasts",
    "podcasts",
}

MUSIC_GENRES = {
    "meditation": "meditation",
    "fairytales": "music_fairy_tale",
    "naturesounds": "music_ambient_sound",
}

SKILL_ID_MAPPING = {"689f64c4-3134-42ba-8685-2b7cd8f06f4d": "litres"}
MAPPED_SKILLS_SCENARIOS = set(SKILL_ID_MAPPING.values())
# https://stackoverflow.com/questions/1175208/elegant-python-function-to-convert-camelcase-to-snake-case
FIRST_CAP_RE = re.compile("(.)([A-Z][a-z]+)")
ALL_CAP_RE = re.compile("([a-z0-9])([A-Z])")


def convert_camel2snake(string):
    s1 = FIRST_CAP_RE.sub(r"\1_\2", string)
    return ALL_CAP_RE.sub(r"\1_\2", s1).lower()


def get_music_scenario(genre):
    if genre in AUDIOBOOK_GENRES:
        return "music_audiobooks"
    if genre in PODCAST_GENRES:
        return "music_podcast"
    return MUSIC_GENRES.get(genre, "music")


def get_pa_intent(intent):
    if intent.startswith("auto"):
        return intent.replace("auto", "personal_assistant", 1)
    if intent.startswith("alice"):
        return intent.replace("alice", "personal_assistant\tscenarios", 1)
    return intent


def get_main_intent_part(intent):
    intent = get_pa_intent(intent).lower()
    # compatibility with alice backends
    intent_parts = intent.split("\t") if "\t" in intent else intent.split(".")

    if intent_parts[0] == "mm":
        intent_parts = intent_parts[1:]

    if len(intent_parts) < 2:
        return intent

    if intent_parts[1] == "scenarios":

        if (
            intent_parts[2] in ("search", "vinsless") and len(intent_parts) > 3
            and intent_parts[3] not in SEARCH_PLACEHOLDERS_EXCEPTIONS
        ):
            return intent_parts[3]
        elif (
            len(intent_parts) > 4 and intent_parts[3] == "map_search_url"
            and intent_parts[4] not in SEARCH_PLACEHOLDERS_EXCEPTIONS
        ):
            return intent_parts[4]
        else:
            return intent_parts[2]
    return intent_parts[1]


def get_main_scenario_part(mm_scenario):
    parts = mm_scenario.split(".")

    # часть сценариев заплодили на какое-то количество интентов (правда, это пока не в проде)
    # см. сюда: https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/library/scenarios/defs/names.h?blame=true&rev=6142146#L12-29
    if len(parts) > 2 and parts[-2] in MM_MULTI_SCENARIOS:
        main_part = parts[-2]
    else:
        main_part = parts[-1]

    if main_part.startswith("Hollywood"):
        main_part = main_part.replace("Hollywood", "")

    # most MegaMind scenarios are CamelCase, we are used to snake_case
    main_part = convert_camel2snake(main_part)

    return main_part


def get_generic_scenario(intent,
                         mm_scenario=None,
                         product_scenario_name=None,
                         music_genre=None,
                         skill_id=None,
                         is_trash=False,
                         music_answer_type=None,
                         filters_genre=None):
    if is_trash:
        return "side_speech"
    if product_scenario_name and mm_scenario not in MM_PRODUCT_SCENARIO_EXCEPTIONS:
        if product_scenario_name == "music":
            # https://st.yandex-team.ru/VA-2284
            # todo: https://st.yandex-team.ru/MEGAMIND-2895
            played_genre = filters_genre if music_answer_type == "filters" else music_genre
            return get_music_scenario(played_genre)
        if product_scenario_name == "dialogovo" and skill_id in SKILL_ID_MAPPING:
            return SKILL_ID_MAPPING[skill_id]
        return product_scenario_name

    if mm_scenario and mm_scenario not in MM_SCENARIO_EXCEPTIONS:
        main_intent = get_main_scenario_part(mm_scenario)
    else:
        main_intent = get_main_intent_part(intent)

    # typical intent is formed like smth or smth_anaphoric
    if main_intent.endswith("_anaphoric"):
        main_intent = main_intent.replace("_anaphoric", "")

    # there are intents with _onboarding part like music_podcast_onboarding answering the question of what podcasts do you have
    # decided with @talamable, that they should be the same scenario as without suffix
    if main_intent.endswith("_onboarding") and main_intent not in ONBOARDING_EXCEPTIONS:
        main_intent = main_intent.replace("_onboarding", "")

    # исключения, для которых приходится заглядывать дальше сценария
    if main_intent in ACTION_EXCEPTIONS:
        for el in ACTION_EXCEPTIONS[main_intent]:
            for action in el["actions"]:
                if action in intent:
                    return el.get("scenario", action)

    for scenario, intents in INTENT_TO_SCENARIO_MAP.items():
        if main_intent in intents:
            return scenario

    # mapping not found
    if mm_scenario and mm_scenario not in MM_SCENARIO_EXCEPTIONS:
        return main_intent
    else:
        for multi_key in MULTI_INTENT_KEYS:
            if multi_key in main_intent:
                return MULTI_INTENT_KEY_REMAP.get(multi_key, multi_key)

    return main_intent
