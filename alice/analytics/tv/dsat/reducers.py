# -*- coding: utf-8 -*-

from datetime import timedelta

from nile.api.v1 import with_hints, Record

from qb2.api.v1 import typing as qt

from constants import (
    PROPERTIES_MAPPING_FOR_WATCHING_EVENTS,
    SCREENS_NUMBERS,
)

from my_utils import (
    get_time_of_record,
    seconds_to_time_string,
    prepare_record_before_yield,
    correct_time_with_new_event,
    translate,
    update_session_info_with_new_event,
    update_session_info_with_results,
    prepare_session_info_before_yield,
    create_header_and_title,
    is_another_action_type,
)


def add_event(results, rec, events_counter):
    # set 'action_type' field
    results['action_type'] = rec.action_type

    # correct 'time' field
    results = correct_time_with_new_event(results, rec, events_counter)

    # correct 'has_gift', 'has_plus', 'is_logged_in' fields
    results['has_gift'] = rec.has_gift or results['has_gift']
    results['has_plus'] = rec.has_plus or results['has_plus']
    results['is_logged_in'] = rec.is_logged_in or results['is_logged_in']

    # correct 'properties' field depending on action_type
    props = results['properties']
    time = get_time_of_record(rec) or 'undefined_time'
    if results['action_type'] in ['session_init', 'session_end']:
        props = {}
        if 'apps_versions' in rec.event_value and rec.event_value['apps_versions']:
            props['Версии приложений'] = rec.event_value['apps_versions']
        if rec.board:
            props['board'] = rec.board
        if rec.build:
            props['build'] = rec.build
        if rec.diagonal:
            props['diagonal'] = rec.diagonal
        if rec.geo_id:
            props['geo_id'] = rec.geo_id
        if rec.manufacturer:
            props['manufacturer'] = rec.manufacturer
        if rec.platform:
            props['platform'] = rec.platform
        if rec.resolution:
            props['resolution'] = rec.resolution
        if rec.model:
            props['model'] = rec.model

    elif results['action_type'] == 'voice_request':
        props.append(time)
        props.append(rec.event_value)

    elif results['action_type'] == 'account':
        status = rec.event_value.get('result', '') or 'undefined_status'
        props.append((time, translate(rec.event_name), status))

    elif results['action_type'] == 'apps':
        app_name = rec.event_value.get('visible_name', '') or 'undefined_name'
        package_name = rec.event_value.get('package_name', '') or 'undefined_package_name'
        if rec.event_name in ['app_launch', 'app_install_page_clicked']:
            props.append((time, translate(rec.event_name), app_name, package_name))
        elif rec.event_name == 'app_install':
            status = rec.event_value.get('status', '')
            error = rec.event_value.get('status_message', '')
            if error:
                props.append((time, translate(rec.event_name), status, app_name, package_name, 'ERROR: ' + error))
            else:
                props.append((time, translate(rec.event_name), status, app_name, package_name))
        else:
            raise RuntimeError('{} is unknown event_name of \'apps\' action_type'.format(rec.event_name))

    elif results['action_type'] == 'search':
        if rec.event_name == 'serp_shown':
            request_string = rec.event_value.get('request_string', '') or 'undefined_request_string'
            method = rec.event_value.get('request_forming_method', '') or 'undefined_method'

            if method == 'from_content_card':
                card_name = rec.event_value.get('content_card_name', '') or 'undefined_card'
                props.append((time, 'запрос: ' + request_string, 'способ: Клик по карточке \"' + card_name + '\"'))
            else:
                props.append((time, 'запрос: ' + request_string, 'способ: ' + translate(method)))
        elif rec.event_name == 'serp_opened':
            props.append((time, translate(rec.event_name)))
        elif rec.event_name == 'collection_opened':
            collection_name = rec.event_value.get('collection_name', '') or 'undefined_collection'
            props.append((time, translate(rec.event_name), collection_name))
        else:
            raise RuntimeError('{} is unknown event_name of \'search\' action_type'.format(rec.event_name))

    elif results['action_type'] == 'settings':
        if rec.event_name == 'settings_opened':
            reason = rec.event_value.get('reason', '')
            if reason:
                props.append((time, translate(rec.event_name), 'причина: ' + translate(reason)))
            else:
                props.append((time, translate(rec.event_name)))

        elif rec.event_name == 'settings_changed':
            group = rec.event_value.get('group', '') or 'unknown_group'
            setting = rec.event_value.get('setting', '') or 'unknown_setting'
            value1 = rec.event_value.get('from', '') or 'unknown_value'
            value2 = rec.event_value.get('to', '') or 'unknown_value'

            props.append((time, translate(rec.event_name), group + '/' + setting + ':  ' + value1 + ' -> ' + value2))

        else:
            raise RuntimeError('{} is unknown event_name of \'settings\' action_type'.format(rec.event_name))

    elif results['action_type'] == 'navigation':
        if rec.event_name == 'screen_changed':
            f = translate(rec.event_value.get('from', '') or 'undefined')
            f = str(SCREENS_NUMBERS[f]) + ') ' + f if f in SCREENS_NUMBERS else f

            t = translate(rec.event_value.get('to', '') or 'undefined')
            t = str(SCREENS_NUMBERS[t]) + ') ' + t if t in SCREENS_NUMBERS else t

            method = translate(rec.event_value.get('method', '') or 'undefined_method')

            if method == 'интерфейс':
                props.append(time + ', ' + f + ' -> ' + t)
            else:
                props.append(time + ', ' + f + ' -> ' + t + ', ' + method)

        elif rec.event_name in ['vh_content_card_opened', 'search_content_card_opened']:
            props.append((time, 'Карточка контента', rec.event_value.get('title', '') or 'undefined_title'))

        elif rec.event_name in ['card_show', 'card_click']:
            header, title = create_header_and_title(rec)
            if props and isinstance(props[-1], list) and props[-1][0] == header:
                props[-1].append(title)
            else:
                props.append([
                    header,
                    '=' * len(header.decode('utf-8')),
                    title
                ])

        else:
            raise RuntimeError('{} is unknown event of \'navigation\' action_type'.format(rec.event_name))

    elif results['action_type'] in ['native_watching', 'youtube_watching', 'tv_watching', 'webview_watching',
                                    'hdmi_watching']:
        if events_counter == 1:
            props = [{}, {'Действия': []}]   # first dict for TVT data, second - watching dict

        if rec.event_name.find('heartbeat') != -1:
            period = rec.event_value.get('period', 0) or 30
            if rec.event_name == 'tv_player_heartbeat':
                channel_name = rec.event_value.get('channel_name', '') or 'unknown_channel'
                props[0][channel_name] = props[0].get(channel_name, 0) + period
            elif rec.event_name == 'webview_player_heartbeat':
                url = rec.event_value.get('url', '') or 'undefined_recourse'
                props[0][url] = props[0].get(url, 0) + period
            elif rec.event_name == 'youtube_player_heartbeat':
                artist = rec.event_value.get('artist', '') or 'undefined_artist'
                title = rec.event_value.get('title', '') or 'undefined_title'
                movie = 'Канал: ' + artist + ' / Видео: ' + title
                props[0][movie] = props[0].get(movie, 0) + period
            elif rec.event_name == 'hdmi_heartbeat':
                props[0]['hdmi'] = props[0].get('hdmi', 0) + period
            else:
                raise RuntimeError('{} is unknown heartbeat event_name'.format(rec.event_name))
        elif rec.event_name.find('action') != -1:
            action = translate(rec.event_value.get('action', '') or 'undefined_action')
            method = translate(rec.event_value.get('method', '') or 'undefined_method')

            if method == 'интерфейс':
                props[-1]['Действия'].append((time, action))
            else:
                props[-1]['Действия'].append((time, action, method))
        elif rec.event_name.find('opened') != -1 or rec.event_name.find('changed') != -1:
            # remove empty data
            if not props[-1]['Действия']:
                props[-1].pop('Действия')
            if not props[-1]:
                props.pop()

            if rec.event_name == 'hdmi_opened':
                props.append((time, translate(rec.event_name)))
            elif rec.event_name in ['native_player_opened', 'tv_player_opened', 'webview_player_opened',
                                    'youtube_player_opened']:
                props.append((time, 'Проигрывание'))
            elif rec.event_name in ['tv_content_changed', 'tv_channel_changed']:
                channel = rec.event_value.get('channel_name', '') or 'undefined_channel'
                props.append((time, 'Переключение канала', channel))
            else:
                raise RuntimeError('{} is unknown event_name of watching action_type'.format(rec.event_name))

            props.append({'Действия': []})
        else:
            raise RuntimeError('{} is unknown event_name of \'watching\' action_type'.format(rec.event_name))

        # if rec is a special watching event
        if rec.event_name in PROPERTIES_MAPPING_FOR_WATCHING_EVENTS:

            # add properties in watching dict
            properties_names = PROPERTIES_MAPPING_FOR_WATCHING_EVENTS[rec.event_name]
            for prop_name in properties_names:
                if prop_name in rec.event_value and rec.event_value[prop_name]:
                    if prop_name in ['from', 'method']:
                        props[-1][translate(prop_name)] = translate(rec.event_value[prop_name])
                    else:
                        props[-1][translate(prop_name)] = rec.event_value[prop_name]

            # add native player information in TVT data
            if rec.event_name == 'native_player_opened' and 'Время просмотра' in props[-1]:
                view_time = int(props[-1]['Время просмотра'])
                title = props[-1].get('Название', '') or 'undefined_title'
                props[0][title] = props[0].get(title, 0) + view_time

            # get needed format for view_time and duration_sec
            if 'Время просмотра' in props[-1]:
                view_time = int(props[-1]['Время просмотра'])
                props[-1]['Время просмотра'] = seconds_to_time_string(view_time)
            if 'Продолжительность видео' in props[-1]:
                duration = int(props[-1]['Продолжительность видео'])
                props[-1]['Продолжительность видео'] = seconds_to_time_string(duration)

    elif results['action_type'] == 'profiles_screen_opened':
        available_puids = rec.event_value.get('available_puids', [])
        props.append(time + ' - количество доступных аккаунтов: ' + str(len(available_puids)))

    elif results['action_type'] == 'screensaver':
        period = rec.event_value.get('period')
        if period is not None:
            period = str(timedelta(seconds=int(period)))
        props.append('Длительность скринсэйвера: ' + (period or 'undefined'))

    elif results['action_type'] == 'music_player':
        props.append((time, 'Открытие музыкального плеера'))

    else:
        raise RuntimeError('{} is unknown action_type'.format(results['action_type']))

    results['properties'] = props

    return results


@with_hints(output_schema=dict(
    action_type=qt.Optional[qt.String],
    date=qt.Optional[qt.Yson],
    time=qt.Optional[qt.String],
    timedelta=qt.Optional[qt.String],
    properties=qt.Optional[qt.Yson],
    properties_excel=qt.Optional[qt.String],
    user_info=qt.Optional[qt.Yson]
))
def main_reducer(groups):
    for key, records in groups:
        session_info = {
            # properties
            'Первое событие сессии': None,
            'Последнее событие сессии': None,
            'Общее время сессии': None,
            'Навигация по рекомендациям': 0,
            'Навигация': 0,
            'Настройки': 0,
            'Приложения': 0,
            'Просмотр рекомендательного контента': 0,
            'Просмотр on-demand контента': 0,
            'HDMI': 0,
            'Просмотр ТВ каналов': 0,
            'Youtube': 0,
            'Музыкальный плеер': 0,
            'Скринсэйвер': 0,

            # fields
            'has_gift': False,
            'has_plus': False,
            'is_logged_in': False,

            # info for recommendation/ondemand content
            'is_recommendation_content': None,
            'recommendation_serps': set(),
            'shown_recommendation_content': set(),

            # info for navigation
            'is_recommendation_navigation': False,
            'last_navigation_event': '',
            'last_navigation_datetime': ''
        }
        results = {
            'action_type': '',
            'time': {
                'start': '',
                'end': ''
            },
            'properties': [],
            'has_gift': False,
            'has_plus': False,
            'is_logged_in': False
        }
        events_counter = 0

        for rec in records:
            if is_another_action_type(rec, results, session_info):
                if events_counter > 0:
                    results = prepare_record_before_yield(results, session_info, key, rec)
                    session_info = update_session_info_with_results(session_info, results)
                    yield Record(**results)

                results = {
                    'action_type': '',
                    'time': {
                        'start': '',
                        'end': ''
                    },
                    'properties': [],
                    'has_gift': False,
                    'has_plus': False,
                    'is_logged_in': False
                }
                events_counter = 1

                results = add_event(results, rec, events_counter)
                session_info = update_session_info_with_new_event(session_info, rec)

            else:
                events_counter += 1
                results = add_event(results, rec, events_counter)
                session_info = update_session_info_with_new_event(session_info, rec)

        results = prepare_record_before_yield(results, session_info, key)
        session_info = update_session_info_with_results(session_info, results)
        yield Record(**results)

        session_info = prepare_session_info_before_yield(session_info, key)
        yield Record(**session_info)
