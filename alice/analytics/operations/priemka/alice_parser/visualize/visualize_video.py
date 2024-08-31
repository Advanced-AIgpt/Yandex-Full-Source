# coding: utf-8

from builtins import str
import re
from alice.analytics.utils.json_utils import get_path
from .utils import get_device_state_data
from .visualize_alarms_timers import get_time_str


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_video')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


SCREENS = {
    'description': _('экран описания'),
    'gallery': _('галерея видео'),
    'main': _('главный экран'),
    'music_player': _('музыкальный плеер'),
    'payment': _('экран оплаты'),
    'radio_player': _('радио плеер'),
    'season_gallery': _('галерея сезона сериала'),
    'splash': _('экран заставки'),
    'tv_gallery': _('галерея телеканалов'),
    'video_player': _('экран видеоплеера'),
    'navi_route': _('маршрут или точка на карте'),
    'navi_route_conformation': _('маршрут или точка на карте, ожидается подтверждение'),
    'mordovia_webview': _('галерея видео'),
}

WEBVIEW_SCREEN_MAPPING = {
    'filmsSearch': 'gallery',
    'videoSearch': 'gallery',
    'channels': 'tv_gallery',
    'videoEntity': 'description',
    'videoEntity/Description': 'description',
    'videoEntity/Carousel': 'description',
    'videoEntity/RelatedCarousel': 'gallery',
    'videoEntity/Seasons': 'season_gallery',
}

MORDOVIA_SCREENS = {
    'ether': _('главный экран'),
    'carousel': _('галерея видео'),
    'onboarding': _('экран возможностей устройства с Алисой'),
}

DEFAULT_SCREEN = _('главный экран')
DEFAULT_ITEM_NAME = _('(название не определено)')

FIRST_ITEMS = _('. Первые 3 элемента списка:') + '\n'
SHOW_TV_GALLERY_PATTERN = _('Открывается галерея из {} телеканалов по запросу пользователя') + FIRST_ITEMS
VIDEO_SEARCH_GALLERY_PATTERN = _('Открывается галерея из {} видео/фильмов/сериалов по запросу пользователя')

WEBVIEW_TAB_MAPPING = {
    'series': _('Сериалы'),
    'movie': _('Фильмы'),
    'kids': _('Мультики'),
    'blogger': _('Видео'),
    'channels': _('ТВ'),
    'sport_hockey_league_nhl': _('НХЛ'),
}

VIDEO_ENTITIES = {
    'video': _('видео'),
    'movie': _('фильм'),
    'tv_stream': _('телеканал'),
    'tv_show_episode': _('сериал'),
    'episode': _('серия'),
    'season': _('сезон'),
}


def get_action_by_video_objects(analytics_info):
    # TODO: use analytics_info for items everywhere
    # https://wiki.yandex-team.ru/users/atsepeleva/vebvju-jekrany-video-v-stancii/
    action = None
    video_obj = get_path(analytics_info, ['Video', 'scenario_analytics_info', 'objects'], []) or \
        get_path(analytics_info, ['alice.vins', 'scenario_analytics_info', 'objects'], [])
    if video_obj:
        if 'video_search_gallery' in video_obj[0]:
            items = get_path(video_obj[0], ['video_search_gallery', 'items'], [])
            if len(items) <= 3:
                action = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + ':\n'
            else:
                action = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + FIRST_ITEMS

            for idx, item in enumerate(items[:3]):
                action += '{}. "{}"\n'.format(str(idx + 1), item.get('name', DEFAULT_ITEM_NAME))
        elif 'video_season_gallery' in video_obj[0]:
            items = get_path(video_obj[0], ['video_season_gallery', 'episodes'], [])
            season = get_path(video_obj[0], ['video_season_gallery', 'season_number'], 1)
            name = get_path(video_obj[0], ['video_season_gallery', 'parent', 'name'], DEFAULT_ITEM_NAME)
            if len(items) <= 3:
                action = _('Открывается список серий {} сезона сериала "{}":').format(season, name) + '\n' + '\n'.join(items[:3])
            else:
                action = _('Открывается список серий {} сезона сериала "{}". Первые 3 элемента списка:').format(season, name) + '\n' + '\n'.join(items[:3])
        elif 'video_description_screen' in video_obj[0]:
            item_name = get_path(video_obj[0], ['video_description_screen', 'item', 'name'], DEFAULT_ITEM_NAME)
            action = _('Открывается страница описания следующего элемента: "{}"').format(item_name) + '\n'
    return action


def get_action_by_mordovia_path(path=''):
    """
    Possible values: https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/mordovia_video_selection/mordovia_tabs.h?rev=7857044
    TODO: remove temporary crutch for channels
    """
    supertag = re.findall(r'supertag=\w+', path)
    if supertag:
        supertag = supertag[0].split('=')[-1]
    else:
        supertag = 'channels' if 'channels' in path else None
    tab_name = WEBVIEW_TAB_MAPPING.get(supertag)
    if tab_name:
        action = _('Табик экрана меняется на: {}. ').format(tab_name)
    else:
        action = _('Открывается домашний экран Станции. ')
    action += _('На экране отображаются рекомендованные фильмы/сериалы/мультфильмы/каналы.') + '\n'
    return action


def tv_show_processing(item):
    name = item.get('name', '')
    add_info = []
    for el in ['season', 'episode']:
        if item.get(el):
            add_info.append(str(item[el]) + VIDEO_ENTITIES[el])
    return '"{}" ({})'.format(name, ', '.join(add_info))


def _get_visualized_state_screen_extra_states(record):
    """
    Возвращает массив extra state — с названиями фильмов/сериалов/..., которые показаны на экране
    В Толоке в интерфейсе шаблона Видео — поле "На экране" https://nda.ya.ru/t/G3A2xbvz3jn4aL
    :param record:
    :return List[dict]:
    """

    video = get_device_state_data(record, 'video', {})
    is_tv_plugged_in = get_device_state_data(record, 'is_tv_plugged_in', True)
    if not is_tv_plugged_in:
        return []

    extra_states = []
    current_screen = video.get('current_screen', 'main')
    screen_state = video.get('screen_state')

    # listing screen contents
    if current_screen == 'gallery':
        if screen_state.get('visible_items'):
            state_st1 = _('{} видео/фильмов/сериалов под соответствующими номерами:').format(str(len(screen_state['items']))) + '\n'
            visible_items = set(screen_state['visible_items'])
            for idx, item in enumerate(screen_state['items']):
                if idx in visible_items:
                    if item.get('name'):
                        state_st1 += '{}. "{}"\n'.format(str(idx + 1), item['name'])
        else:
            state_st1 = str(len(screen_state['items'])) + _(' видео/фильмов/сериалов') + '\n'
        extra_states.append({
            'type': 'На экране',
            'content': state_st1
        })

    elif current_screen == 'season_gallery':
        state_st2 = _('Серии {} сезона сериала "{}".').format(str(screen_state['season']), screen_state['tv_show_item']['name']) + '\n'
        if screen_state.get('visible_items'):
            visible_items = set(screen_state['visible_items'])
            for idx, item in enumerate(screen_state['items']):
                if idx in visible_items:
                    state_st2 += '{}. "{}"\n'.format(str(idx + 1), tv_show_processing(item))
        extra_states.append({
            'type': _('На экране'),
            'content': state_st2
        })

    elif current_screen == 'tv_gallery':
        state_st3 = _('{} телеканалов под соответствующими номерами. Первые 3 элемента списка:').format(str(len(screen_state['items']))) + '\n'
        for idx, item in enumerate(screen_state['items'][:3]):
            state_st3 += '{}. "{}"'.format(str(idx + 1), item['name'])
            if item.get('tv_episode_name'):
                state_st3 += _(', телепередача - "{}". ').format(item['tv_episode_name'])
            state_st3 += '\n'
        extra_states.append({
            'type': _('На экране'),
            'content': state_st3.strip(' ')
        })

    elif current_screen == 'mordovia_webview':
        screen_states = get_path(video, ['view_state', 'sections']) or get_path(video, ['page_state', 'sections']) or [{}]
        screen_state = screen_states[0]
        if screen_state:
            items = screen_state.get('items', [])
            visible_items = [i for i in items if i.get('active') is True]
            item = screen_state.get('current_item') or screen_state.get('current_tv_show_item') or {}
            state_st1 = ''

            if visible_items:
                state_st1 = _('{} видео/фильмов/сериалов под соответствующими номерами:').format(str(len(visible_items))) + '\n'
                for vi in visible_items:
                    state_st1 += '{}. "{}"\n'.format(str(vi.get('number', 0)), vi.get('title', DEFAULT_ITEM_NAME))
            elif items:
                state_st1 = str(len(items)) + _(' видео/фильмов/сериалов') + '\n'
            elif item:
                state_st1 = _('Описание фильма/сериала "{}"').format(item.get('title', DEFAULT_ITEM_NAME))
                if item.get('main_trailer_uuid'):
                    state_st1 += _(' со ссылкой на трейлер')

            extra_states.append({
                'type': _('На экране'),
                'content': state_st1
            })

    elif current_screen == 'description':
        item = screen_state['item']
        state_st4 = _('Описание ')
        if item.get('type') == 'tv_show':
            state_st4 += _('сериала ')
        elif item.get('type') == 'movie':
            state_st4 += _('фильма ')
        else:
            state_st4 += _('видео ')
        state_st4 += '"{}".'.format(item['name'])
        if 'rating' in item and item.get('rating'):
            state_st4 += _('Рейтинг: {}. ').format(item['rating'])
        extra_states.append({
            'type': _('На экране'),
            'content': state_st4.rstrip()
        })

    elif current_screen == 'payment':
        item = screen_state['item']
        state_st5 = _('Оплата ')
        if screen_state.get('tv_show_item'):
            tv_show_item = screen_state['tv_show_item']
            state_st5 += _('сериала "{}".').format(tv_show_item['name'])
            if item:
                state_st5 += _('Серия "{}"').format(item['name'])
                if ('season' in item and item['season']) or ('episode' in item and item['episode']):
                    state_st5 += ' ('
                    if 'season' in item and item['season']:
                        state_st5 += str(item['season']) + _(' сезон ')
                    if 'episode' in item and item['episode']:
                        state_st5 += str(item['episode']) + _(' серия')
                    state_st5 += ')'
        elif item:
            state_st5 += _('фильма "{}"').format(item['name'])
        extra_states.append({
            'type': _('На экране'),
            'content': state_st5
        })

    return extra_states


def _get_visualized_state_for_currently_playing_item(record):
    """
    Возвращает массив extra state — с названием текущего проигрываемого видео
    :param dict record:
    :return List[dict]:
    """
    video = get_device_state_data(record, 'video', {})
    is_tv_plugged_in = get_device_state_data(record, 'is_tv_plugged_in', True)
    now = video.get('currently_playing', {})
    item = now.get('item') or now.get('tv_show_item')
    if not is_tv_plugged_in or not item:
        return []

    extra_states = []
    default_name = _('видео')
    entity = VIDEO_ENTITIES.get(item.get('type'), default_name)
    tv_show_item = now.get('tv_show_item')
    if tv_show_item and tv_show_item.get('name'):
        name = tv_show_item['name']
        additional = _(', Серия ') + tv_show_processing(item)
    else:
        name = item.get('name', default_name)
        additional = _(', Телепередача "{}"').format(item['tv_episode_name']) if item.get('tv_episode_name') else ''

    ret = {
        'type': _('Последнее просмотренное: {}').format(entity),
        'content': '"{}"{}'.format(name, additional)
    }

    current_screen = video.get('current_screen', 'main')
    if current_screen == 'video_player' and not now.get('paused', True):
        ret['playback'] = _('Сейчас это видео воспроизводится ')
    else:
        ret['playback'] = _('Сейчас воспроизведение поставлено на паузу ')
    ret['playback'] += 'на ' + get_time_str(now['progress']['played'], on=True)

    if item.get('type') == 'tv_show_episode' and now.get('next_item'):
        ret['playback'] += '\n' + _('Следующая серия в очереди: ') + tv_show_processing(now['next_item'])

    extra_states.append(ret)

    avail_audio = item.get('audio_streams', [])
    if avail_audio:
        extra_states.append({
            'type': _('У воспроизводимого видео доступны аудио-дорожки'),
            'content': ', '.join([i.get('title', '') for i in avail_audio])
        })

    audio = now.get('audio_language')
    if audio:
        extra_states.append({
            'type': _('Выбрана аудио-дорожка'),
            'content': audio
        })

    avail_sub = item.get('subtitles')
    if avail_sub:
        extra_states.append({
            'type': _('У воспроизводимого видео доступны субтитры'),
            'content': ', '.join([i.get('title', '') for i in avail_sub])
        })

    sub = now.get('subtitles_language')
    if sub:
        extra_states.append({
            'type': _('Выбраны субтитры'),
            'content': sub
        })

    return extra_states


def _get_visualized_state_screen_name(record):
    """
    Возвращает строку с описанием экрана — какой экран показан
    В Толоке в интерфейсе шаблона Видео — поле "Экран" https://nda.ya.ru/t/G3A2xbvz3jn4aL
    :param dict record:
    :return str:
    """
    navigator = get_device_state_data(record, 'navigator', {})
    if navigator:
        if navigator.get('current_route'):
            return SCREENS['navi_route']

        if navigator.get('states'):
            if 'waiting_for_route_confirmation' in navigator['states']:
                return SCREENS['navi_route_conformation']

    is_tv_plugged_in = get_device_state_data(record, 'is_tv_plugged_in', True)
    if is_tv_plugged_in:
        video = get_device_state_data(record, 'video', {})
        current_screen = video.get('current_screen', 'main')
        screen_state = video.get('screen_state')

        # screen naming
        if current_screen == 'mordovia_webview':
            webview_screen = get_path(video, ['view_state', 'currentScreen'])
            if webview_screen in WEBVIEW_SCREEN_MAPPING:
                screen = WEBVIEW_SCREEN_MAPPING[webview_screen]
                return SCREENS[screen]
            else:
                screen_scenario = screen_state.get('scenario') if screen_state else None
                return MORDOVIA_SCREENS.get(screen_scenario, DEFAULT_SCREEN)
        else:
            return SCREENS.get(current_screen, DEFAULT_SCREEN)
    else:
        # TODO: проверить ПП
        return _('не подключено к телевизору')
