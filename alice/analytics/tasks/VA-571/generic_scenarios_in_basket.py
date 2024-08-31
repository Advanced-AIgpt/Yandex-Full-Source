# -*-coding: utf8 -*-

BAN_SET = {
    "", "empty", # не очень понятно, что туда попадает, не хочется такое брать
    "iot_do", # Умный дом всегда в рамках свежей/потоковой корзины
    "beru" # тут могут быть приватные данные и вообще это как бы часть маркета и он уже представлен в корзине
}


QUASAR_UE2E_SCENARIOS = {
    "alarm",
    "bluetooth",
    "call",
    "cancel_todo",
    "commands_other",
    "convert",
    "dialogovo",
    "external_skill_gc",
    "feedback",
    "find_poi",
    "general_conversation",
    "get_news",
    "identity_commands",
    "images_what_is_this",
    "info_request",
    "market",
    "music",
    "music_ambient_sound",
    "music_fairy_tale",
    "music_podcast",
    "music_what_is_playing",
    "nav_url",
    "onboarding",
    "ontofacts",
    "placeholders",
    "player_commands",
    "radio",
    "random_number",
    "reminder",
    "repeat",
    "repeat_after_me",
    "route",
    "search",
    "shopping_list",
    "show_traffic",
    "sleep_timer",
    "sound_commands",
    "stop",
    "taxi",
    "theremin",
    "timer",
    "todo",
    "translate",
    "tv_broadcast",
    "tv_stream",
    "video",
    "video_commands",
    "weather"
}


# надо дозаполнить, когда корзина будет собрана
GENERAL_UE2E_SCENARIOS = {}
NAVI_UE2E_SCENARIOS = {}
