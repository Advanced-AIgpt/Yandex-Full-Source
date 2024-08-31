# -*- coding: utf-8 -*-

# ============================================ PATHS ============================================


TVANDROID_SESSIONS_PATH = '//home/smarttv/logs/tvandroid_sessions/1d/{{{start_date}..{end_date}}}'
TVANDROID_SESSIONS_DIR = '//home/smarttv/logs/tvandroid_sessions/1d'
STRM_FILTERED_PATH = '//home/smarttv/logs/strm/1d/{{{start_date}..{end_date}}}'
STRM_FILTERED_DIR = '//home/smarttv/logs/strm/1d'
CONTENT_INFO_PATH = '//home/sda/cubes/tv/recommendations/sources/content_info'
CAROUSELS_INFO_PATH = '//home/sda/cubes/tv/recommendations/sources/carousel_data_clean'
PREPARED_LOGS_EXPBOXES_PATH = '//home/alice/dialog/prepared_logs_expboxes/{{{start_date}..{end_date}}}'
PREPARED_LOGS_EXPBOXES_DIR = '//home/alice/dialog/prepared_logs_expboxes'


# ============================= YQL FUNCTION TO PARSE CAROUSELS IDS =============================


HH_PARENT_ID_YQL_UDF = '''
    YQL::Udf(
        AsAtom("Protobuf.TryParse"),
        Void(), Void(),
        AsAtom(@@{
            "name": "NVideoRecom.TCarouselId",
            "lists": {"optional": false},
            "meta": "H4sICI3Fel8AAzEA49rByCWYnFiUX1qcmhOfmaJXUJRfki/E7ReWmZKaH5SanJ+rNJWRizvEGarGM0VIhIvVObEkNV2CUYFRgzMIwhGS4GIPSi0sTS0ukWACi8O4IPUhiemeKRLMEPVgjpASF49zYl5waVJxclFmUqoEC1CSIwhFTEiGi9Oz2D8tLSczL1WCFawAIQAAgJxztrsAAAA="
        }@@)
    )(String::Base64Decode($p0)).Categ
'''


# ============================== ACTION_TYPE TO PROPERTIES MAPPING ==============================


PROPERTIES_MAPPING_FOR_WATCHING_EVENTS = {
    'native_player_opened': [
        'title',
        'duration_sec',
        'view_time',
        'genres'
    ],
    'youtube_player_opened': [
        'title',
        'artist',
        'album',
        'duration_sec'
    ],
    'webview_player_opened': [
        'title',
        'url',
        'duration_sec'
    ],
    'tv_player_opened': [],   # it is really an event with no useful information
    'hdmi_opened': [
        'user_port_name',
        'device_name',
        'from'
    ],
    'tv_content_changed': [
        'method',
        'channel_name',
        'channel_category',
        'content_name',
        'play_type'
    ],
    'tv_channel_changed': [
        'method',
        'channel_name',
        'channel_category',
        'content_name',
        'play_type'
    ]
}


# ==================================== SESSION_INFO PROPERTIES ====================================


SESSION_INFO_PROPERTIES = ['Первое событие сессии', 'Последнее событие сессии', 'Общее время сессии',
                           'Навигация по рекомендациям', 'Навигация', 'Настройки', 'Приложения',
                           'Просмотр рекомендательного контента', 'Просмотр on-demand контента', 'HDMI',
                           'Просмотр ТВ каналов', 'Youtube', 'Музыкальный плеер', 'Скринсэйвер']

SESSION_INFO_TIMESPENT_PROPERTIES = ['Навигация по рекомендациям', 'Навигация', 'Настройки', 'Приложения',
                                     'Просмотр рекомендательного контента', 'Просмотр on-demand контента', 'HDMI',
                                     'Просмотр ТВ каналов', 'Youtube', 'Музыкальный плеер', 'Скринсэйвер']


# ================================== SCREENS TO NUMBERS MAPPING ==================================


SCREENS_NUMBERS = {
    'Поиск': 1,
    'Главная': 2,
    'ТВ': 3,
    'Фильмы': 4,
    'Сериалы': 5,
    'Блогеры': 6,
    'Мультфильмы': 7,
    'Музыка': 8
}


# ================================ FOR RECOMMENDATION NAVIGATION ================================


RECOMMENDATION_SCREENS = ['main', 'movie', 'series', 'kids', 'blogger', 'show_more_items', 'section_grid']


# ======================================= FOR TRANSLATION =======================================


DICTIONARY = {
    # action types
    'session_init': 'Включение телевизора',
    'session_end': 'Выключение телевизора',
    'voice_request': 'Голосовой запрос',
    'account': 'Аккаунт',
    'profiles_screen_opened': 'Экран профиля',
    'session_info': 'Информация о сессии',
    'youtube_watching': 'Youtube',
    'tv_watching': 'Просмотр ТВ каналов',
    'hdmi_watching': 'HDMI',
    'recommendation_watching': 'Просмотр рекомендательного контента',
    'ondemand_watching': 'Просмотр on-demand контента',
    'apps': 'Приложения',
    'settings': 'Настройки',
    'navigation': 'Навигация',
    'recommendation_navigation': 'Навигация по рекомендациям',
    'screensaver': 'Скринсэйвер',
    'music_player': 'Музыкальный плеер',
    # 'search' in screen names

    # screen names
    'main': 'Главная',
    'tv': 'ТВ',
    'movie': 'Фильмы',
    'series': 'Сериалы',
    'blogger': 'Блогеры',
    'kids': 'Мультфильмы',
    'music': 'Музыка',
    'search': 'Поиск',
    'search_collection_page': 'Экран коллекции из поисковой выдачи',
    'profiles': 'Экран профилей',
    'film_content_card': 'Карточка фильма',
    'episode_content_card': 'Карточка эпизода',
    'search_content_card': 'Карточка контента из поисковой выдачи',
    'native_player': 'Плеер',
    'webview_player': 'Плеер',
    'tv_player': 'ТВ плеер',
    'show_more_items': 'Показать еще',
    'installed_apps': 'Приложения',
    'details': 'Подробности',
    'app_details': 'Экран приложения',
    'home': 'Главная',
    'home_recent': 'Главная (карусель "Недавние")',
    'pult_tv': 'Запуск кнопкой на пульте',
    'section_grid': 'Экран с подборкой контента',

    # event_names
    'account_login': 'Вход в аккаунт',
    'account_logout': 'Выход из аккаунта',
    'account_change': 'Смена аккаунта',
    'app_install': 'Установка приложения',
    'app_launch': 'Запуск приложения',
    'app_install_page_clicked': 'Открытие установочной странички приложения',
    'settings_opened': 'Открытие настроек',
    'settings_changed': 'Изменение настроек',
    'serp_opened': 'Открытие результатов запроса',
    'collection_opened': 'Открытие коллекции',
    'hdmi_opened': 'Открытие HDMI',

    # properties
    'duration_sec': 'Продолжительность видео',
    'view_time': 'Время просмотра',
    'title': 'Название',
    'genres': 'Жанры',
    'artist': 'Канал',
    'album': 'Плэйлист',
    'user_port_name': 'Отображаемое название порта',
    'device_name': 'Имя устройства',
    'from': 'Экран запуска',
    'channel_name': 'Канал',
    'channel_category': 'Категория',
    'content_name': 'Название передачи',
    'play_type': 'Тип контента',
    'method': 'Способ',

    # player actions
    'start': 'старт',
    'play': 'плэй',
    'pause': 'пауза',
    'stop': 'стоп',
    'rew': 'перемотка назад',
    'fwd': 'перемотка вперед',
    'next': 'следующее видео',
    'prev': 'предыдущее видео',
    'rubricator_opened': 'Открытие списка каналов',
    'dialpad_opened': 'Открытие панели набора',

    # methods
    'pult': 'пульт',
    'ui': 'интерфейс',
    'voice': 'голосовой запрос',
    'input': 'ввод через экранную клавиатуру',
    'historical_suggest': 'саджест (история)',
    'autocomplete_suggest': 'саджест (автодополнение)',
    'launch': 'Включение ТВ плеера',
    'rubricator': 'Выбрано из списка каналов (рубрикатора)',
    'dialpad': 'Введение названия канала в панель набора',
    'pult_back': 'Кнопка "Назад" на пульте',
    'pult_home': 'Кнопка "Домой" на пульте',

    # other
    'no internet': 'нет интернета',
    'no_internet': 'нет интернета',
}
