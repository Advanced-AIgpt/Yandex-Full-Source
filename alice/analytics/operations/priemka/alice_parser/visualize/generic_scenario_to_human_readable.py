# coding: utf-8

import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('generic_scenario_to_human_readable')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


generic_scenario_to_human_readable = {
    'alarm': _('Настройка будильников'),
    'alice_show': _('Шоу Алисы'),
    'avia': _('Покупка авиабилетов'),
    'beru': _('Заказ/управление заказом на маркетплейсе Беру'),
    'bluetooth': _('Настройка bluetooth'),
    'call': _('Звонок'),
    'cec_commands': _('Управление внешним телеэкраном'),
    'commands': _('Команды'),
    'commands_other': _('Команды управления устройством'),
    'convert': _('Конвертация курсов валют'),
    'count_aloud': _('Счёт вслух'),
    'covid': _('Информация о коронавирусе'),
    'dialogovo': _('Навыки Алисы'),
    'direct_gallery': _('Реклама (галерея объявлений)'),
    'draw_picture': _('Нарисуй картинку'),
    'external_skill_gc': _('Поддержание диалога'),
    'factoid_src': _('Факты'),
    'feedback': _('Обратная связь'),
    'find_poi': _('Поиск организаций'),
    'game_advise': _('Рекомендация игр'),
    'general_conversation': _('Поддержание диалога'),
    'general_conversation_tr': _('Поддержание диалога'),
    'generative_tale': _('Генеративные сказки'),
    'get_date': _('Информация о датах'),
    'get_news': _('Новости'),
    'goods': _('Поиск товаров'),
    'how_much': _('Стоимость товара'),
    'how_to_spell': _('Как пишется'),
    'identity_commands': _('Идентификация пользователя'),
    'image_gallery': _('Галерея картинок'),
    'images_what_is_this': _('Поиск по картинке'),
    'info_request': _('Сообщение информации'),
    'iot_do': _('Умный дом'),
    'led_clock_commands': _('Управление часами'),
    'link_a_remote': _('Настройка пульта'),
    'litres': _('Книги Литрес'),
    'maps_download_offline': _('Загрузка офлайн-карт'),
    'market': _('Покупка на Яндекс.Маркете'),
    'meditation': _('Медитация'),
    'messenger_call': _('Сценарий обработки входящих вызовов'),
    'morning_show': _('Шоу Алисы'),
    'movie_advise': _('Рекомендация фильмов'),
    'music': _('Музыка'),
    'hollywoodmusic': _('Музыка'),
    'music_ambient_sound': _('Звуки природы'),
    'music_audiobooks': _('Аудиокниги'),
    'music_fairy_tale': _('Сказки'),
    'music_podcast': _('Подкасты'),
    'music_what_is_playing': _('Определение играющей музыки'),
    'nav_url': _('Открытие сайта/приложения'),
    'navi_commands': _('Команды Навигатора'),
    'news': _('Новости'),
    'object_search_oo': _('Поиск фактов'),
    'onboarding': _('Рекомендация навыков Алисы'),
    'ontofacts': _('Поиск фактов'),
    'order': _('Статус заказа'),
    'player_commands': _('Управление плеером'),
    'radio': _('Радио'),
    'random_number': _('Случайное число'),
    'reminder': _('Настройка напоминаний'),
    'repeat': _('Повтор последнего ответа Алисы'),
    'repeat_after_me': _('Повторение фразы пользователя'),
    'route': _('Маршруты'),
    'search': _('Поиск информации'),
    'search_commands': _('Управление фильтрами поиска'),
    'serp': _('Переход на страницу результатов поисковой выдачи'),
    'show_gif': _('Показ картинок gif'),
    'settings': _('Настройка Алисы'),
    'shopping_list': _('Управление списком покупок'),
    'show_traffic': _('Уровень пробок'),
    'sleep_timer': _('Таймер сна'),
    'smartspeaker_notifications': _('Уведомления в колонках'),
    'smart_device_external_app': _('Открытие сайта или приложение на умном устройстве'),
    'sound_commands': _('Управление звуком'),
    'subscriptions_manager': _('Управление подпиской'),
    'stop': _('Стоп'),
    'taxi': _('Такси'),
    'theremin': _('Терменвокс'),
    'timer': _('Управление таймерами'),
    'todo': _('Управление списком дел'),
    'transcription': _('Транскрипция слова'),
    'transform_face': _('Изменение лица'),
    'translate': _('Перевод'),
    'tv_controls': _('ТВ-команды'),
    'tv_broadcast': _('ТВ-программа'),
    'tv_stream': _('Яндекс.Эфир'),
    'tv_channels': _('Управление ТВ каналами'),
    'video': _('Видео'),
    'video_commands': _('Видео команды'),
    'voice': _('Модификации голоса Алисы'),
    'voiceprint': _('Запоминание голоса пользователя'),
    'weather': _('Погода'),
    'zen_search': _('Переход в ленту Дзена'),
    'miles': _('Почта'),
    'external_skill_flash_briefing': _('Радионовости'),
    'external_skill_recipes': _('Рецепты')
}
