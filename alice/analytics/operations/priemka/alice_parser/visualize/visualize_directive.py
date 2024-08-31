# coding: utf-8

from __future__ import division
from builtins import str
from past.utils import old_div
import json
import re
import os
from collections import OrderedDict
from alice.analytics.utils.json_utils import get_path
from urllib import unquote

from .visualize_music import get_music_entity

from .utils import get_device_state_data, get_slots_answer
from .visualize_video import (
    get_action_by_video_objects,
    get_action_by_mordovia_path,
    tv_show_processing,
    VIDEO_SEARCH_GALLERY_PATTERN,
    SHOW_TV_GALLERY_PATTERN,
    FIRST_ITEMS,
    DEFAULT_ITEM_NAME,
)
from .visualize_music import (
    get_music_description,
    get_radio_action,
)
from .visualize_alarms_timers import (
    alarms_processing,
    get_time_str,
    _get_cur_date,
    _get_tz_from_record,
)
from .visualize_multiroom import get_multiroom_devices
from .visualize_geo import get_route_for_state
from .visualize_open_uri import get_open_uri_action


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_directive')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


DIRECTIVES_ONLY_SINGLE = {
    'audio_stop',
    'clear_queue',
    'player_pause'
}

VOLUME_LEVEL_STR = _('уровень громкости')


def get_short_column_name(column_name):
    return column_name[:column_name.find(' (')]


def get_calls_human_readable(analytics_info):
    if analytics_info:
        for scenario_name in analytics_info:
            scenario_analytics_info = get_path(analytics_info, [scenario_name, 'scenario_analytics_info'],
                                               default={})
            actions = scenario_analytics_info.get('actions', [{}])
            if not actions:
                return None
            result = '. '.join([action.get('human_readable', '') for action in actions]) + ' '

            objects = scenario_analytics_info.get('objects', [{}])
            result += '. '.join([get_short_column_name(object.get('human_readable', '')) for object in objects])

            return result

    return None


def find_by_id(dicts, id_):
    for dict_ in dicts:
        if dict_.get('id') == id_:
            return dict_
    return None


def get_human_readable(analytics_info, default=None):
    """
    Возвращает human_readable - человекочитаемое действие сценария, которое сценарий пишет в analytics_info
    """
    if not analytics_info:
        return default

    if os.environ.get('LANGUAGE') and os.environ.get('LANGUAGE') != 'ru':
        # human_readable есть только для русского языка
        return default

    for scenario_name in analytics_info:
        scenario_analytics_info = get_path(analytics_info, [scenario_name, 'scenario_analytics_info'],
                                           default={})
        actions = scenario_analytics_info.get('actions', [{}])
        if not actions:
            return default
        if not any('human_readable' in action for action in actions):
            return default

        result = '. '.join([action.get('human_readable') for action in actions if 'human_readable' in action and re.search(u'[\u0400-\u04FF]', action.get('human_readable').decode('utf-8'))])
        if not result:
            return default

        objects = scenario_analytics_info.get('objects', [{}])
        first_track = find_by_id(objects, id_='music.first_track_id')
        if first_track:
            first_track_id = first_track.get('first_track', {}).get('id')
            if first_track_id:
                url = 'https://music.yandex.ru/track/' + first_track_id
                result += ': ' + url
            track_title = first_track.get('human_readable')
            if track_title:
                result += ' ({})'.format(track_title)

        return result


def _get_visualized_directive(directive, record, is_quasar=True):
    analytics_info = get_path(record, ['analytics_info', 'analytics_info'], {})

    if 'payload' in directive:
        dname = directive['name']
        sub_name = directive.get('sub_name', '')
        payload = directive['payload']
        if dname == 'mordovia_show':
            tab_path = get_path(payload, ['url'], '')
            return get_action_by_video_objects(analytics_info) or get_action_by_mordovia_path(path=tab_path)

        if dname == 'mordovia_command' and payload.get('command') == 'change_path':
            tab_path = get_path(payload, ['meta', 'path'], '')
            return get_action_by_video_objects(analytics_info) or get_action_by_mordovia_path(path=tab_path)

        if dname == 'show_gallery':
            if 'items' not in payload:
                return _('Открывается галерея видео/фильмов/сериалов по запросу ') + '\"' + payload['debug_info']['ya_video_request'] + '\"'

            if len(payload['items']) <= 3:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(payload['items'])) + ':\n'
            else:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(payload['items'])) + FIRST_ITEMS
            for idx, item in enumerate(payload['items'][:3]):
                action_str += '{}. {}\n'.format(str(idx + 1), item.get('name', DEFAULT_ITEM_NAME))
            return action_str

        if dname == 'show_tv_gallery':
            action_str = SHOW_TV_GALLERY_PATTERN.format(len(payload['items']))
            for idx, item in enumerate(payload['items']):
                if idx >= 3:
                    break
                action_str += str(idx + 1) + '. ' + '"' + item['name'] + '" '
                if item.get('tv_episode_name'):
                    action_str += _(', телепередача - "') + item['tv_episode_name'] + '"'
                action_str += '\n'
            return action_str

        if dname == 'show_season_gallery':
            action_str = _('Открывается список серий {} сезона сериала "{}"').format(str(payload['season']), payload['tv_show_item']['name'])
            if len(payload['items']) <= 3:
                action_str += ':\n'
            else:
                action_str += FIRST_ITEMS

            for idx, item in enumerate(payload['items'][:3]):
                action_str += str(idx + 1) + '. ' + tv_show_processing(item) + '\n'
            return action_str

        if dname == 'show_description':
            item = payload['item']
            action_str = _('Открывается описание ')
            if item.get('type') == 'tv_show':
                action_str += _('сериала ')
            elif item.get('type') == 'movie':
                action_str += _('фильма ')
            else:
                action_str += _('видео ')
            action_str += '"{}".'.format(item.get('name', DEFAULT_ITEM_NAME))
            if item.get('rating'):
                action_str += _('Рейтинг: {}.').format(str(item['rating']))
            return action_str

        if dname == 'video_play':
            action_str = ''
            item = payload['item']
            if payload.get('tv_show_item'):
                tv_show_item = payload['tv_show_item']
                action_str = _('Включается сериал "{}".').format(tv_show_item['name'])
                if item:
                    action_str += _(' Серия ') + tv_show_processing(item)
                if payload.get('next_item'):
                    action_str += '\n' + _('Следующая серия в очереди: {}').format(tv_show_processing(payload['next_item']))
            else:
                if item:
                    if item.get('type') == 'tv_stream':
                        action_str = _('Включается телеканал "{}"').format(item['name'])
                        if item.get('tv_episode_name'):
                            action_str += _(', телепередача "{}".').format(item['tv_episode_name'])
                    else:
                        action_str = _('Включается видео "{}".').format(item.get('name', DEFAULT_ITEM_NAME))
                next_item_name = get_path(payload, ['next_item', 'name'])
                if next_item_name:
                    action_str += '\n' + _('Следующее видео в очереди: "{}".').format(next_item_name)
            action_str = action_str.strip()
            return action_str

        if dname == 'web_os_launch_app_directive':
            app_id = payload['app_id']
            if app_id == 'tv.kinopoisk.ru':
                params = json.loads(payload['params_json'])
                page_id = params['pageId']
                if page_id == 'musicPlayer':
                    entity = get_music_entity(record.get('analytics_info'), record.get('generic_scenario'), get_music_answer_uri=True)
                    if 'selected_web_document_url' in entity.keys():
                        return get_radio_action('', record, entity['selected_web_document_url'])
                    if 'music_answer_uri' in entity.keys():
                        return get_radio_action('', record, entity['music_answer_uri'])
                elif page_id in ['series', 'film', 'player', 'payment']:
                    return get_human_readable(record.get('analytics_info'))
                elif page_id == 'library':
                    return _('Открывается приложение Кинопоиск.')
            elif app_id == 'youtube.leanback.v4':
                utterance = unquote(re.search(r"(?:\?|\&)v\=(.*)(?:\&|$)", params['contentTarget'])[1])
                if utterance:
                    return _('В приложении Youtube открываются результаты поиска по запросу "{}".'.format(utterance))
                else:
                    return _('Открывается приложение Youtube.')
            elif app_id == 'yandex.alice':
                return _('Открывается экран с описанием умений Алисы.')

        if dname == 'web_os_show_gallery_directive':
            items = payload['items_json']
            if len(items) <= 3:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + ':\n'
            else:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + FIRST_ITEMS
            for n, item in enumerate(items[:3], start=1):
                action_str += '{}. {}\n'.format(n, json.loads(item)['title'])
            return action_str

        if dname == 'tv_open_search_screen':
            objs = get_path(analytics_info, ['Video', 'scenario_analytics_info', 'objects'], [])
            default_text = _('Открывается галерея видео/фильмов/сериалов по запросу "{}"').format(payload['search_query'])
            if not objs:
                return default_text
            items = get_path(objs[0], ['video_search_gallery', 'items'], [])
            if not items:
                return default_text
            if len(items) <= 3:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + ':\n'
            else:
                action_str = VIDEO_SEARCH_GALLERY_PATTERN.format(len(items)) + FIRST_ITEMS
            for n, item in enumerate(items[:3], start=1):
                action_str += '{}. {}\n'.format(n, item['name'])
            return action_str

        if dname == 'tv_open_details_screen':
            action_str = _('Открывается описание ')
            if payload['content_type'] == 'TV_SERIES':
                action_str += _('сериала')
            elif payload['content_type'] == 'MOVIE':
                action_str += _('фильма')
            else:
                action_str += _('видео')
            name = get_path(payload, ['data', 'name'], '')
            action_str += ' "{}"'.format(name) if name else _(', название неизвестно')
            return action_str

        if dname == 'tv_open_person_screen':
            action_str = _('Открывается карточка человека')
            name = get_path(payload, ['data', 'name'], '')
            action_str += ': "{}"'.format(name) if name else _(', имя неизвестно')
            return action_str

        # FIXME couldn't find schema or example
        if dname == 'tv_open_collection_screen':
            action_str = _('Открывается коллекция ')
            return action_str

        if dname == 'open_screensaver':
            action_str = _('Открывается экранная заставка')
            return action_str

        if dname == 'music_play':
            action_str = get_human_readable(analytics_info)
            if action_str:
                return action_str

            slot_answer = None
            action_str = ''

            if analytics_info:
                for scenario_name in analytics_info:
                    slots = get_path(analytics_info, [scenario_name, 'semantic_frame', 'slots'])
                    slot_answer = get_slots_answer(slots)

            if slot_answer:
                action_str = get_music_description(
                    slot_answer,
                    is_fairy_tales=record.get('generic_scenario') == 'music_fairy_tale'
                )
            elif payload.get('first_track_id'):
                if payload['first_track_id'].startswith('https://music.yandex.ru'):
                    action_str = _('Включается ') + payload['first_track_id']
                else:
                    url = 'https://music.yandex.ru/track/' + payload['first_track_id']
                    action_str = _('Первый трек, который включится: ') + url
            elif record.get('generic_scenario') in ('morning_show', 'alice_show'):
                action_str = _('Включается шоу Алисы')

            if not (
                record.get('generic_scenario') == 'music_fairy_tale'
                and slot_answer and slot_answer.get('type') == 'playlist'
            ):
                # если включился Плейлист сказок, то не добавляем информацию о треке
                objects = get_path(analytics_info, ['HollywoodMusic', 'scenario_analytics_info', 'objects'], [{}])
                human_readable = objects[0].get('human_readable')
                if human_readable:
                    action_str += " ({})".format(human_readable)

            if action_str:
                return action_str

        if dname == 'audio_play':
            action_str = get_human_readable(analytics_info)
            if action_str:
                return action_str

            if payload.get('metadata', {}).get('title') and payload.get('metadata', {}).get('subtitle'):
                return _('Аудио, которое включится: {} - {}').format(
                    payload['metadata']['title'], payload['metadata']['subtitle']
                )

        if dname == '@@mm_stack_engine_get_next':
            action_str = get_human_readable(analytics_info)
            if action_str:
                return action_str

        if dname == '@@mm_semantic_frame':
            slot_answer = None
            action_str = ''

            # In case of redirect to thin client Vins returns mm_semantic_frame and
            # fills slot_answer

            VINS_SCENARIO_NAME = 'Vins'
            if analytics_info and VINS_SCENARIO_NAME in analytics_info:
                slots = get_path(analytics_info, [VINS_SCENARIO_NAME, 'semantic_frame', 'slots'])
                slot_answer = get_slots_answer(slots)

            if slot_answer:
                action_str = get_music_description(
                    slot_answer,
                    is_fairy_tales=record.get('generic_scenario') == 'music_fairy_tale'
                )
            return action_str

        if dname == 'fill_cloud_ui':
            return _('Действие Алисы сопровождается фразой "{}", отображаемой над иконкой голосового ассистента'.format(get_path(payload, ['text'])))

        if dname == 'show_promo':
            return _('Показывается баннер с предложением осуществить действие с помощью другого устройства или приложения с Алисой')

        if dname == 'start_multiroom':
            device_ids = get_path(directive, ['room_device_ids']) or get_path(payload, ['room_device_ids'])
            room_id = get_path(payload, ['room_id'])
            iot_config = get_path(record, ['analytics_info', 'iot_user_info'])
            murtiroom_devices = get_multiroom_devices(device_ids, room_id, iot_config)
            if murtiroom_devices:
                return _('Начинается воспроизведение на устройствах: ') + murtiroom_devices
            else:
                return _('Включается музыка в режиме Мультирум')

        if dname == 'radio_play':
            return _('Включается радио "{}"').format(payload['title'])

        if dname == 'open_uri':
            if record.get('generic_scenario') == 'call' and not sub_name:
                return get_calls_human_readable(analytics_info)
            if sub_name in ('music_yaradio_play', 'music_app_or_site_play', 'radio_app_or_site_play'):
                return get_radio_action(payload['uri'], record)

            if sub_name == 'smart_tv_switch_tv_channel':
                action_str = get_human_readable(analytics_info)
                if action_str:
                    return action_str

                return _('Производится переключение канала')

            if sub_name == 'add_smart_speaker_screen':
                return _('Открывается страница настройки устройств')

            navi_names = {'navi_show_point_on_map', 'navi_build_route_on_map', 'navi_map_search'}
            if sub_name in navi_names or sub_name.startswith('personal_assistant.scenarios.find_poi'):
                action_str = get_route_for_state(record)
                if action_str:
                    return action_str

            return get_open_uri_action(payload['uri'], sub_name, record, is_quasar=is_quasar)

        if dname == 'yandexnavi':
            if sub_name == 'yandexnavi_map_search':
                if 'params' in payload and 'text' in payload['params']:
                    text = payload['params']['text']
                    return _('Открывается поиск на карте в навигаторе по запросу <{}>').format(text)
                else:
                    return _('Осуществляется поиск на карте в навигаторе по заданному запросу')
            if sub_name in ('yandexnavi_build_route_on_map', 'yandexnavi_map_search'):
                action_str = get_route_for_state(record)
                if action_str:
                    return action_str
            # TODO: понять, могут ли тут быть ещё разновидности sub_name

        if dname == 'messenger_call':
            if payload.get('decline_current_call'):
                return _('Алиса завершила текущий звонок')
            elif payload.get('accept_call'):
                return _('Снимается трубка во входящем звонке')
            elif payload.get('decline_incoming_call'):
                return _('Сбрасывается входящий вызов')

        if dname == 'sound_set_level':
            # TODO: по хорошему хотим парсить на каком-то уровне, что Алиса отвечает шепотом
            is_whisper_sound_applied = get_path(record, ['analytics_info', 'modifiers_analytics_info', 'whisper', 'is_sound_set_level_directive_applied'])
            if is_whisper_sound_applied:
                return

            # get with default zero because of ADI-102
            return _('Устанавливается уровень громкости, равный {}').format(payload.get('new_level', 0))

        if dname == 'sound_louder':
            return _('Увеличивается уровень громкости на 1')

        if dname == 'sound_quiter':
            return _('Уменьшается уровень громкости на 1')

        if dname == 'sound_mute':
            return _('Включение беззвучного режима')

        if dname == 'sound_unmute':
            return _('Выключение беззвучного режима')

        if dname == 'alarm_set_sound' and payload.get('sound_alarm_setting'):
            sound_alarm_setting = payload['sound_alarm_setting']
            if sound_alarm_setting.get('type') == 'music' and sound_alarm_setting.get('info'):
                action_str = get_music_description(sound_alarm_setting['info'], alarm_set_sound=True)
                if action_str == '':
                    action_str = _('На будильник устанавливается выбранная музыка')
                return action_str
            elif sound_alarm_setting.get('type') == 'radio' and sound_alarm_setting.get('info'):
                action_str = _('В качестве мелодии на будильник устанавливается радио "{}"').format(
                    sound_alarm_setting['info'].get('title', '')
                )
                if sound_alarm_setting['info'].get('frequency'):
                    action_str += _(', частота - ') + sound_alarm_setting['info']['frequency']
                return action_str

        if dname == 'alarm_reset_sound':
            return _('Сбрасывается музыка на будильнике')

        if dname == 'alarm_stop':
            return _('Выключается будильник')

        if dname == 'alarms_update':
            all_alarms = alarms_processing(payload['state'], _get_tz_from_record(record), _get_cur_date(record))
            if all_alarms:
                return _('Теперь установлены следующие будильники:') + '\n' + all_alarms
            else:
                return _('Теперь никаких будильников не установлено')

        if dname == 'set_timer':
            if payload.get('duration'):
                return _('Установлен таймер на ') + get_time_str(payload['duration'])
            else:
                return _('Установлен таймер')

        if dname == 'cancel_timer':
            return _('Удален текущий таймер')

        if dname == 'pause_timer':
            return _('Текущий таймер поставлен на паузу')

        if dname == 'resume_timer':
            return _('Текущий таймер снят с паузы')

        if dname == 'timer_stop_playing':
            return _('Выключается таймер')

        if dname == 'player_pause':
            device_ids = get_path(directive, ['room_device_ids'])
            room_id = get_path(payload, ['room_id'])
            iot_config = get_path(record, ['analytics_info', 'iot_user_info'])
            murtiroom_devices = get_multiroom_devices(device_ids, room_id, iot_config)
            if murtiroom_devices:
                return _('Останавливается воспроизведение на устройствах: ') + murtiroom_devices

            music = get_device_state_data(record, 'music', {})
            video = get_device_state_data(record, 'video', {})

            if music or video != {'current_screen': 'main'}:
                action_str = _('Музыка и/или видео ставятся на паузу, если проигрывались')
                if payload == {'listening_is_possible': True}:
                    action_str += _('. Алиса слушает речь пользователя')
                return action_str

            radio = get_device_state_data(record, 'radio', {})
            if radio:
                return _('Радио ставится на паузу, если проигрывалось')

        if dname in ('audio_stop', 'clear_queue'):
            return _('Воспроизведение аудио ставится на паузу, если что-либо проигрывалось')

        if dname == 'show_video_settings':
            return _('Показываются доступные дорожки аудио/субтитров')

        if dname == 'change_audio':
            return _('Аудио-дорожка меняется на: {}').format(payload['title'])

        if dname == 'change_subtitles':
            return _('Дорожка субтитров меняется на: {}').format(payload['title'])

        if dname == 'player_continue' and payload['player'] == 'video':
            return _('Включается видео после паузы (или просто продолжается воспроизведение)')

        if dname == 'player_continue' and payload['player'] == 'music':
            return _('Включается музыка после паузы (или просто продолжается воспроизведение)')

        if dname == 'player_next_track' and payload.get('player') == 'video':
            return _('Включается следующее видео')

        if dname == 'player_next_track' and payload.get('player') == 'music':
            return _('Включается следующий музыкальный трек')

        if dname == 'player_previous_track' and payload['player'] == 'video':
            return _('Включается предыдущее видео')

        if dname == 'player_previous_track' and payload['player'] == 'music':
            return _('Включается предыдущий музыкальный трек')

        if dname == 'player_rewind':
            if payload['type'] == 'forward':
                return _('Перематывает вперед на ') + get_time_str(payload['amount'])
            elif payload['type'] == 'backward':
                return _('Перематывает назад на ') + get_time_str(payload['amount'])
            elif payload['type'] == 'absolute':
                return _('Перематывает на ') + get_time_str(payload['amount'], when=True)

        if dname == 'audio_player_rewind':
            action_str = get_human_readable(analytics_info)
            if action_str:
                return action_str

            amount = get_time_str(old_div(payload['amount_ms'], 1000))
            if payload['type'] == 'forward':
                return _('Перематывает вперед на ') + amount
            elif payload['type'] == 'backward':
                return _('Перематывает назад на ') + amount
            elif payload['type'] == 'absolute':
                return _('Перематывает на ') + amount

        if dname == 'player_replay':
            return _('Играет трек или видео еще раз c начала')

        if dname == 'player_repeat':
            return _('Включается повтор плейлиста с начала')

        if dname == 'player_like':
            return _('Трек отмечается как понравившийся')

        if dname == 'player_dislike':
            return _('Трек отмечается как непонравившийся')

        if dname == 'player_shuffle':
            return _('Воспроизведение происходит в случайном порядке')

        if dname == 'player_order':
            return _('Отключается проигрывание текущего плейлиста в случайном порядке')

        if dname == 'start_music_recognizer':
            return _('Голосовой помощник определяет, какая музыка играет')

        if dname == 'go_home':
            return _('Переходит на главный экран')

        if dname == 'go_backward':
            return _('Осуществляется переход на предыдущий экран')

        if dname == 'go_forward':
            return _('Осуществляется переход на следующий экран')

        if dname == 'go_down':
            return _('Осуществляется переход на экран ниже')

        if dname == 'go_up':
            return _('Осуществляется переход на экран выше')

        if dname == 'onboarding_play':
            return _('Проигрывается видео с обзором возможностей Станции')

        if dname == 'onboarding_skip':
            return _('Отменяется показ видео с обзором возможностей Станции')

        if dname == 'go_to_the_beginning':
            return _('Осуществляется переход в начало списка')

        if dname == 'go_to_the_end':
            return _('Осуществляется переход в конец списка')

        if dname == 'start_bluetooth':
            return _('Включается bluetooth на устройстве')

        if dname == 'stop_bluetooth':
            return _('Выключается bluetooth на устройстве')

        if dname == 'show_pay_push_screen':
            action_str = _('Показывается экран оплаты ')
            item = payload['item']
            if payload.get('tv_show_item'):
                action_str += _('сериала "{}".').format(payload['tv_show_item']['name'])
                if item:
                    action_str += _('Серия ') + tv_show_processing(item)
            elif item:
                action_str += _('фильма "{}".').format(item['name'])
            return action_str

        if dname == 'send_bug_report':
            return _('Отправляется багрепорт ')

        if dname == 'save_voiceprint':
            return _('Сохраняется голос говорящего ')

        if dname == 'screen_on':
            return _('Включается внешний телеэкран, подключенный по HDMI')

        if dname == 'screen_off':
            return _('Выключается внешний телеэкран, подключенный по HDMI')

        if dname == 'start_image_recognizer':
            action_str = get_human_readable(analytics_info)
            if action_str:
                return action_str

        if dname == 'show_clock':
            return _('Включается LED-экран с отображением текущего времени')

        if dname == 'hide_clock':
            return _('LED-экран с часами выключается')

        if dname == 'iot_discovery_start':
            return _('Алиса начинает поиск новых устройств Умного Дома.')

        if dname == 'send_android_app_intent':
            action = payload.get('action')
            if action == 'com.yandex.tv.action.TANDEM_SETUP':
                return _('Открывается экран настройки тандема')


def _prepare_directives_list(directives):
    """
    Фильтрует директивы, возвращает список директив, которые нужно визуализировать
    :param list[dict] directives:
    :return list[dict]:
    """
    has_any_non_single_directive = any([True for x in directives if x.get('name') not in DIRECTIVES_ONLY_SINGLE])
    has_audio_play_directive = any([True for x in directives if x.get('name') == 'audio_play'])
    single_directives_count = 0

    for directive in directives:
        if directive.get('name') in DIRECTIVES_ONLY_SINGLE:
            if has_any_non_single_directive:
                # выкидываем директиву стопа, если есть другие существенные директивы
                continue
            if single_directives_count > 0:
                # в случае, если уже была стоп-директива — следующие выкидываем
                continue
            single_directives_count += 1

        if directive.get('name') == '@@mm_stack_engine_get_next' and has_audio_play_directive:
            # выкидываем mm_stack_engine_get_next в случае, если есть audio_play
            continue

        yield directive


def _get_action_human_readable(record):
    """
    Возвращает визуализированное действие, если оно есть в human_readable
    """
    HUMAN_READABLE_BLACKLIST = ['call', 'iot_do']
    if record.get('generic_scenario') in HUMAN_READABLE_BLACKLIST:
        return

    analytics_info = get_path(record, ['analytics_info', 'analytics_info'], {})
    return get_human_readable(analytics_info)


def _get_visualized_action(record, is_quasar=True):
    """
    Визуализирует действие, которое выполняет Алиса (по директиве)
    Может визуализировать несколько директив, тогда они будут отрисованы по-порядку, разделенные переносом строки
    :param dict record:
    :return str:
    """
    # VA-1574
    actions_dict = OrderedDict()
    analytics_info = get_path(record, ['analytics_info', 'analytics_info'], {})

    if analytics_info and analytics_info.get('ExternalSkillFlashBriefing'):
        for action in get_path(analytics_info, ['ExternalSkillFlashBriefing', 'scenario_analytics_info', 'actions'],
                               []):
            if action.get('id') == 'external_skill.news.activate':
                actions_dict['external_skill.news.activate'] = action.get('human_readable')

    for directive in _prepare_directives_list(record.get('directives') or []):
        if directive.get('name') in actions_dict:
            continue

        new_action_human_readable = _get_visualized_directive(directive, record, is_quasar)
        if not new_action_human_readable:
            continue
        actions_dict[directive.get('name')] = new_action_human_readable

    actions_list = list(actions_dict.values())
    action_human_readable = '\n'.join(actions_list)
    scenario_human_readable = _get_action_human_readable(record)

    if scenario_human_readable and len(actions_list) < 2:
        # возвращаем human_readable от сценария в случае, если визуализируется только 1 директива
        return scenario_human_readable

    if action_human_readable:
        return action_human_readable

    if record.get('generic_scenario') == 'player_commands':
        action_str = get_human_readable(analytics_info)
        if action_str:
            return action_str

    if record.get('generic_scenario') == 'do_nothing':
        return _('Алиса деликатно промолчала и ничего не сделала')

    if record.get('generic_scenario') == 'voice':
        intent = record.get('intent')
        if intent == 'whisper\tturn_on':
            return _('Включается режим шепота')
        if intent == 'whisper\tturn_off':
            return _('Выключается режим шепота')
        if intent == 'whisper\tsay_something':
            return _('Алиса отвечает шепотом')

    if record.get('intent') == 'voice_discovery':
        # на случай, если не сработала директива iot_discovery_start для этого интента
        return _('Запускается функция подключения новых устройств Умного Дома')

    return None
