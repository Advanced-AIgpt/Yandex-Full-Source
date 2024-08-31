# coding: utf-8

from future import standard_library
standard_library.install_aliases()
import re

from urllib.parse import unquote

from .utils import get_slots_answer, get_slots


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_music')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


SCENARIOS_WITH_MUSIC_ENTITIES = {
    'music',
    'music_fairy_tale',
    'music_ambient_sound',
    'music_podcast',
    'meditation',
    'alice_show',
    'morning_show'
}


def get_music_description(music_meta, alarm_set_sound=False, is_fairy_tales=False):
    music_description = ''
    if music_meta.get('type') == 'album' and music_meta.get('title'):
        music_description += _('Воспроизводится альбом "{}"').format(music_meta['title'])
        if music_meta.get('artists') and len(music_meta['artists']) == 1:
            music_description += _(' исполнителя ') + music_meta['artists'][0]['name']
        elif music_meta.get('artists'):
            music_description += _(' исполнителей ') + ', '.join([artist['name'] for artist in music_meta['artists']])
        music_description += ' ' + music_meta['uri'].replace('/?from=alice&mob=0', '') + ' . '

    if music_meta.get('type') == 'artist' and music_meta.get('name'):
        music_description += _("Воспроизводятся треки исполнителя {} ").format(music_meta['name']) + \
            music_meta['uri'].replace('/?from=alice&mob=0', '') + ' . '

    if music_meta.get('type') == 'playlist' and music_meta.get('title'):
        music_description += _('Воспроизводится плейлист "{}". ').format(music_meta['title'])
        if is_fairy_tales:
            return music_description
        # Не показываем ссылку из-за логина юзера + music_meta['uri'].replace("/?from=alice&mob=0", "") + " . "

    if music_meta.get('type') == 'track' or music_meta.get('first_track', {}).get('uri'):
        if music_meta.get('type') == 'track':
            if music_meta.get('uri'):
                url = music_meta['uri'].replace('/?from=alice&mob=0', '')
            else:
                url = 'https://music.yandex.ru/track/' + music_meta['id']
            music_description += _('Включается трек ') + url
            track_info = music_meta
        else:
            music_description += _('Первый трек, который включится: ') + \
                music_meta['first_track']['uri'].replace('/?from=alice&mob=0', '')
            track_info = music_meta['first_track']
        additional_info = ''
        if track_info.get('artists'):
            additional_info += ', '.join([artist['name'] for artist in track_info['artists']]) + ' – '
        if track_info.get('album', {}).get('title'):
            additional_info += _('альбом "{}", ').format(track_info['album']['title'])
        if track_info.get('title'):
            additional_info += _('трек "{}" ').format(track_info['title'])
        if additional_info:
            music_description += ' (' + additional_info.strip() + ')'
    elif music_meta.get('first_track_uri', {}):
        music_description += _('Первый трек, который включится: ') + \
            music_meta['first_track_uri'].replace('/?from=alice&mob=0', '')

    if alarm_set_sound:
        # TODO: проверить на арабском
        if music_description.startswith(_('Первый трек, который включится: ')):
            return _('В качестве мелодии на будильник устанавливается трек') + \
                music_description[music_description.find(':') + 1:]
        if music_description.startswith(_('Воспроизводится')) or music_description.startswith(_('Включается трек')):
            return _('В качестве мелодии на будильник устанавливается') + music_description[music_description.find(' '):]
        if music_description.startswith(_('Воспроизводятся')):
            return _('В качестве мелодии на будильник устанавливаются') + music_description[music_description.find(' '):]
    return music_description


def get_radio_parameters(slots):
    genre_name, title = None, None
    s = get_slots_answer(slots)
    try:
        if s.get('title'):
            title = s['title']
        if s.get('station') and s['station'].get('name'):
            genre_name = s['station']['name']
    except Exception:
        return None, None
    return genre_name, title


def _parse_radio_url(uri):
    """Достаёт ссылку на Радио, куда ведёт uri в директиве"""
    if uri is None:
        return None

    try:
        url = unquote(re.search('browser_fallback_url=.+', unquote(uri)).group(0)[21:])
        if url.find('?from=alice') != -1:
            url = url[:url.find('?from=alice')]
        url = url.rstrip('/')
        return url
    except:
        return None


def get_radio_action(uri, state, url=''):
    slots = get_slots(state)
    genre_name, title = get_radio_parameters(slots)
    if not url:
        url = _parse_radio_url(uri)

    DEFAULT_RADIO_ACTION = _('Включется персональная подборка рекомендаций на Яндекс.Музыке')

    if uri.startswith('intent://radio/user/onyourwave') or uri.startswith('https://radio.yandex.ru/user/onyourwave') or \
            (url is not None and url.startswith('https://radio.yandex.ru/user/onyourwave')):
        return DEFAULT_RADIO_ACTION
    if uri.startswith('https://radio.yandex.ru'):  # yandex auto
        if uri.find('?from=alice') != -1:
            if uri[:uri.find('?from=alice')] == 'https://radio.yandex.ru/user/bass.testing.analyst' or uri.startswith('https://radio.yandex.ru/user'):
                return DEFAULT_RADIO_ACTION
            return _('Включается подборка музыки ') + uri[:uri.find('?from=alice')]
    if uri.startswith('http://music.yandex.ru/fm/'):
        return _('Включается радио "{}"').format(uri)

    if url is None:
        # fallback на случай, если не сработали правила на uri, но при этом не удалось распарсить browser_fallback_url
        return DEFAULT_RADIO_ACTION

    genre_name_str = ' <' + genre_name + '>' if genre_name else ''
    title_str = ' <' + title + '>' if title else ''

    if url == 'https://music.yandex.ru/radio':
        return DEFAULT_RADIO_ACTION
    if url.startswith('https://music.yandex.ru/users/music-blog'):
        return _('Включается плейлист ') + url
    if url.startswith('https://music.yandex.ru/users'):
        return _('Включается подборка{} в Яндекс.Музыке').format(title_str)
    if url.startswith('https://music.yandex.ru/album') and 'track' not in url:
        return _('Включается альбом ') + url
    if url.startswith('https://radio.yandex.ru/genre'):
        return _('Включается подборка музыки жанра{} {}').format(genre_name_str, url)
    if url.startswith('https://radio.yandex.ru/activity'):
        return _('Включается подборка музыки для занятия{} {}').format(genre_name_str, url)
    if url.startswith('https://radio.yandex.ru/epoch'):
        return _('Включается подборка музыки эпохи{} {}').format(genre_name_str, url)
    if url.startswith('https://music.yandex.ru/artist'):
        return _('Включается подборка музыки по исполнителю ') + url
    if url.startswith('https://radio.yandex.ru/mood'):
        return _('Включается подборка музыки по настроению{} {}').format(genre_name_str, url)
    if url.startswith('https://music.yandex.ru/playlist/daily'):
        return _('Включается плейлист дня ') + url
    if url.startswith('https://music.yandex.ru/playlist'):
        return _('Включается плейлист ') + url
    if 'track' in url:
        return _('Включаются рекомендации музыки по треку ') + url

    return DEFAULT_RADIO_ACTION


def remove_music_url_params(url):
    import urllib.parse
    remove_types = ['play', 'lang']
    parsed = urllib.parse.urlparse(url)
    if not parsed.query:
        return url

    try:
        qs = urllib.parse.parse_qs(parsed.query, keep_blank_values=True)
    except:
        try:
            qs = urllib.parse.parse_qs(parsed.query.decode('utf-8'), keep_blank_values=True)
        except:
            qs = {}

    left_qs = sorted([(q, val) for q, val in list(qs.items()) if q not in remove_types])
    parsed = list(parsed)
    parsed[4] = str(urllib.parse.urlencode(left_qs, doseq=True))
    return urllib.parse.urlunparse(map(str, parsed))


def get_music_entity(analytics_info, generic_scenario, get_music_answer_uri=False):
    ignore_slot_types = {'music_result', 'action_request', 'string'}
    music_entity = {}
    if analytics_info and analytics_info.get('analytics_info') and generic_scenario in SCENARIOS_WITH_MUSIC_ENTITIES:
        for key, value in list(analytics_info['analytics_info'].items()):
            if value.get('scenario_analytics_info') and value['scenario_analytics_info'].get('events'):
                for event in value['scenario_analytics_info']['events']:
                    if event.get('selected_web_document_event') and event['selected_web_document_event'].get('document_url'):
                        document_url = event['selected_web_document_event']['document_url']
                        music_entity['selected_web_document_url'] = remove_music_url_params(document_url)
                    if event.get('music_event') and event['music_event'].get('answer_type'):
                        music_entity['music_answer_type'] = event['music_event']['answer_type']
                        if get_music_answer_uri:
                            music_entity['music_answer_uri'] = event['music_event']['uri'] if 'uri' in event['music_event'].keys() else ''
            if value.get('semantic_frame') and value['semantic_frame'].get('slots'):
                music_filters = []
                for slot in value['semantic_frame']['slots']:
                    if slot.get('value') and slot.get('type') and slot['type'] not in ignore_slot_types:
                        music_filters.append({'type': slot['type'], 'value': slot['value']})
                if music_filters:
                    music_entity['music_filters'] = music_filters
    return music_entity
