# -*- coding: utf-8 -*-

MUSIC_PLAY_DICT = {
    "Первый трек, который включится": "_track",
    "Включается музыкальный трек": "_track",
    "Воспроизводится альбом": "_album",
    "Воспроизводится плейлист": "_playlist",
    "Включается трек": "_track",
    "other": "_failure_music_action"
}

VIDEO_PLAY_DICT = {
    "Включается видео": "_video",
    "Включается сериал": "_series",
    "Открывается галерея из": "_gallery",
    "Открывается описание фильма": "_movie_description",
    "Открывается описание сериала": "_series_description",
    "Открывается список серий": "_series_list",
    "Показывается экран оплаты фильма": "_movie_pay",
    "Показывается экран оплаты сериала": "_series_pay",
    "Включается телеканал": "_channel",
    "other": "_failure_video_action"
}

SEARCH_FAILURE_DICT = {
    "Мне кажется, меня уронили. Спросите ещё раз попозже, пожалуйста.": "_failure_searh_action",
    "У меня нет ответа на такой запрос": "_failure_searh_action",
    "Я пока не умею отвечать на такие запросы": "_failure_searh_action",
    "Я не могу на это ответить": "_failure_searh_action",
    "Извините, у меня нет хорошего ответа": "_failure_searh_action",
    "Простите, я не знаю что ответить": "_failure_searh_action",
    "Какие сложные вопросы вы задаете": "_failure_searh_action",
    "Я не могу открывать сайты и приложения на этом устройстве": "_failure_searh_action",
    "Очень хочу, но не могу: это устройство не поддерживает подобную функцию": "_failure_searh_action",
    "Я бы с радостью, но на этом устройстве не могу, увы": "_failure_searh_action",
    "other": "_other_search_answer"
}

RADIO_PLAY_DICT = {
    "Включается радио": "_radio",
    "other": "_failure_radio_action"
}

SWITCH_PLAY_DICT = {
    "Включается музыка после паузы": "_continue_music",
    "Включается видео после паузы": "_continue_video",
    "Включается следующий музыкальный трек": "_next_track",
    "Включается предыдущий музыкальный трек": "_previous_track",
    "Играет трек или видео еще раз c начала": "_replay",
    "Музыкальный трек проигрывается еще раз c начала": "_replay",
    "Воспроизведение происходит в случайном порядке": "_shuffle",
    "Включается режим повтора": "_repeat",
    "Музыка и/или видео ставятся на паузу, если проигрывались": "_pause",
    "Радио ставится на паузу, если проигрывалось": "_pause",
    "Перематывает": "_rewind",
    "other": "_failure_switch_play_action"
}

SET_VOLUME_DICT = {
    "Устанавливается уровень громкости, равный": "_set_volume",
    "Увеличивается уровень громкости": "_louder",
    "Уменьшается уровень громкости": "_quiter",
    "other": "_failure_set_volume_action"
}

STOP_DICT = {
    "Музыка и/или видео ставятся на паузу, если проигрывались": "_pause",
    "Радио ставится на паузу, если проигрывалось": "_pause",
    "Воспроизведение аудио ставится на паузу, если что-либо проигрывалось": "_pause",
    "other": "_failure_pause_action"
}

ALARM_DICT = {
    "Теперь установлены следующие будильники": "_alarm",
    "Выключается будильник": "_alarm_turn_off",
    "Теперь никаких будильников не установлено": "_remove_alarm",
    "other": "_other_alarm_action"
}

FAIRY_TALE_DICT = {
    "Воспроизводится плейлист \"Детские сказки\"": "_fairy_tale",
    "Включается трек": "_fairy_tale_certain",
    "Воспроизводится альбом": "_fairy_tale",
    "other": "_failure_fairy_tale_action"
}

TRANSLATE_DICT = {
    "Увы, не поняла, можно еще раз?": "_failure_translate_action",
    "А что именно перевести?": "_failure_translate_action",
    "Не поняла.": "_failure_translate_action",
    "other": "_translation"
}

EXTERNAL_SKILL_DICT = {
    "Запускаю навык": "_external_skill",
    "other": "_failure_external_skill_action"
}

TIMER_DICT = {
    "Установлен таймер на": "_set_timer",
    "Удален текущий таймер": "_delete_timer",
    "Выключается таймер": "_stop_timer",
    "other": "_failure_timer_action"
}

FEEDBACK_DICT = {
    "Трек отмечается как понравившийся": "_like_track",
    "Трек отмечается как непонравившийся": "_dislike_track",
    "other": "_failure_like_action"
}

REMINDER_DICT = {
    "Поставила напоминание": "_set_reminder",
    "О чём нужно напомнить?": "_question_reminder",
    "Что нужно напомнить?": "_question_reminder",
    "Что вам нужно напомнить?": "_question_reminder",
    "Что вам напомнить?": "_question_reminder",
    "О чём вам напомнить?": "_question_reminder",
    "О чём вам нужно напомнить?": "_question_reminder",
    "other": "_failure_reminder_action"
}

FIND_POI_DICT = {
    "Ничего не нашлось.": "_failure_find_poi_action",
    "Я бы и рада помочь, но без карты никак.": "_failure_find_poi_action",
    "Боюсь, что ничего не нашлось.": "_failure_find_poi_action",
    "Без карты я не справлюсь. Увы.": "_failure_find_poi_action",
    "К сожалению, ничего не удалось найти.": "_failure_find_poi_action",
    "Для этого мне нужны Яндекс.Карты, а здесь их нет.": "_failure_find_poi_action",
    "Какое место вы хотите найти?": "_question_find_poi",
    "Какую организацию найти?": "_question_find_poi",
    "other": "_find_poi"
}

SHOW_ROUTE_DICT = {
    "Не знаю, что за место такое": "_failure_show_route_action",
    "Впервые о таком слышу": "_failure_show_route_action",
    "Извините, я ничего не знаю о": "_failure_show_route_action",
    "Куда": "_question_show_route_action",
    "other": "_show_route"
}

TV_STREAM_DICT = {
    "Открывается галерея из": "_gallery_tv_stream_action",
    "Включается телеканал": "_tv_stream_action",
    "other": "_failure_tv_stream_action"
}

MUSIC_WHAT_IS_DICT = {
    "Сейчас играет": "_music_is",
    "Это": "_music_is",
    "other": "_other_music_action"
}

SHOW_TRAFFIC_DICT = {
    "Извините, я не могу понять, где это": "_failure_show_traffic_action",
    "Увы, но я не могу понять, где это": "_failure_show_traffic_action",
    "Извините, я не знаю, где это": "_failure_show_traffic_action",
    "К сожалению, у меня нет информации о": "_failure_show_traffic_action",
    "К сожалению, я не могу ответить на вопрос о": "_failure_show_traffic_action",
    "other": "_show_traffic"
}

NEWS_DICT = {
    "В эфире Яндекс Новости": "_news_action",
    "Новости сами себя не прочтут": "_news_action",
    "Всегда мечтала стать ведущей, вот последние новости": "_news_action",
    "Вот последние новости": "_news_action",
    "В эфире главные новости": "_news_action",
    "other": "_failure_news_action"
}

WEATHER_DICT = {
    "Сейчас в": "_now",
    "Завтра": "_tomorrow",
    "Послезавтра": "_day_after_tomorrow",
    "Сегодня": "_today",
    "В настоящее время": "_now",
    "В ближайшие 24 часа": "_today",
    "По моим данным, в ближайшие 24 часа": "_today",
    "В ближайшие сутки": "_today",
    "other": "_weather_action"
}

SCENARIO_MAP = {
    "music": MUSIC_PLAY_DICT,
    "music_ambient_sound": MUSIC_PLAY_DICT,
    "radio": RADIO_PLAY_DICT,
    "sound_commands": SET_VOLUME_DICT,
    "music_fairy_tale": FAIRY_TALE_DICT,
    "timer": TIMER_DICT,
    "sleep_timer": TIMER_DICT,
    "feedback": FEEDBACK_DICT,
    "translate": TRANSLATE_DICT,
    "find_poi": FIND_POI_DICT,
    "route": SHOW_ROUTE_DICT,
    "reminder": REMINDER_DICT,
    "music_what_is_playing": MUSIC_WHAT_IS_DICT,
    "show_traffic": SHOW_TRAFFIC_DICT,
    "search": SEARCH_FAILURE_DICT,
    "alarm": ALARM_DICT,
    "video": VIDEO_PLAY_DICT,
    "video_commands": VIDEO_PLAY_DICT,
    "tv_broadcast": VIDEO_PLAY_DICT,
    "stop": STOP_DICT,
    "commands_other": {},
    "dialogovo": EXTERNAL_SKILL_DICT,
    "tv_stream": TV_STREAM_DICT,
    "get_news": NEWS_DICT,
    "weather": WEATHER_DICT
}

ACTION_SCENARIOS = {"music", "music_ambient_sound", "radio", "sound_commands", "music_fairy_tale", "timer",
                    "sleep_timer", "feedback", "alarm", "video", "video_commands", "tv_broadcast", "stop", "tv_stream",
                    "commands_other"}

ACTION_ALIASES_TO_EXTEND = {"_track", "_radio", "_set_volume", "_fairy_tale_certain",
                            "_rewind", "_set_timer", "_rewind", "_alarm"}

VIDEO_ON_DEMAND_SET = {"_video", "_series", "_movie_description", "_series_description",
                       "_movie_pay", "_series_pay"}

ANSWER_ALIASES_TO_EXTEND = {"_set_reminder", "_music_is", "_other_search_answer", "_weather_action"}


def get_alias_for_scenario(action, action_dict):
    if not action_dict:
        return ""
    for phrase, alias in action_dict.items():
        if action.startswith(phrase):
            return alias
    return action_dict["other"]


def get_first_sentence(s, punct="."):
    return s.split(punct)[0]


def get_alias(generic_scenario, action, answer):
    alias = ""
    if generic_scenario == "player_commands":
        switch_dicts = (VIDEO_PLAY_DICT, MUSIC_PLAY_DICT, FEEDBACK_DICT, SWITCH_PLAY_DICT)

        for switch_dict in switch_dicts:
            alias = get_alias_for_scenario(action, switch_dict)
            if alias != switch_dict["other"]:
                return alias
        return alias

    if generic_scenario not in SCENARIO_MAP:
        return ""

    scenario_dict = SCENARIO_MAP[generic_scenario]

    if generic_scenario in ACTION_SCENARIOS:
        return get_alias_for_scenario(action, scenario_dict)

    return get_alias_for_scenario(answer, scenario_dict)


def get_extended_info(generic_scenario, action, answer, alias):
    if generic_scenario == "player_commands" and alias != "_rewind":
        return ""

    if alias in ACTION_ALIASES_TO_EXTEND or alias in VIDEO_ON_DEMAND_SET:
        return "_" + action

    if alias in ANSWER_ALIASES_TO_EXTEND:
        return "_" + answer

    if alias in ("_channel", "_tv_stream_action"):
        return "_" + get_first_sentence(action, ",")

    if alias == "_external_skill":
        return "_" + get_first_sentence(answer)

    return ""


def answer_standard(generic_scenario, action, answer, app):
    """
    Приводит ответ Алисы к стандартному виду для сравнения ответов между собой.
    """

    # TODO: поддержать answer_standard для ПП и Навика
    if app not in ("quasar", "small_smart_speakers"):
        return None

    generic_scenario = generic_scenario or ""
    action = action or ""
    answer = answer or ""

    alias = get_alias(generic_scenario, action, answer)
    extended_info = get_extended_info(generic_scenario, action, answer, alias)

    if alias == "_pause":
        generic_scenario = "PAUSE"

    if extended_info:
        return generic_scenario + extended_info

    if alias:
        return generic_scenario + alias

    if not generic_scenario and not action and not answer:
        return None

    return generic_scenario + "_" + action + "_" + answer
