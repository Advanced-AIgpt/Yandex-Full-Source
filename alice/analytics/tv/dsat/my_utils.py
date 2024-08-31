# -*- coding: utf-8 -*-

from datetime import datetime, timedelta

from constants import (
    DICTIONARY,
    SESSION_INFO_PROPERTIES,
    SESSION_INFO_TIMESPENT_PROPERTIES,
    SCREENS_NUMBERS,
    RECOMMENDATION_SCREENS,
)


# ===================================== FOR PROPERTIES_EXCEL =====================================


def yson_to_excel_string(yson_data):
    def to_string(data, indents=0):
        result = ''
        if isinstance(data, dict):
            if data:
                result += '\n'     # '{\n'
                for key, value in data.items():
                    result += '\t' * (indents + 1) + key + ': ' + to_string(value, indents + 1)
                # result += '\t' * indents + '}'
            else:
                result += '{}'
        elif isinstance(data, list):
            if data:
                result += '\n'     # '[\n'
                for elem in data:
                    result += '\t' * (indents + 1) + to_string(elem, indents + 1)
                # result += '\t' * indents + ']'
            else:
                result += '[]'
        elif isinstance(data, tuple):
            # result += '('
            for i in range(len(data)):
                result += str(data[i])
                if i < len(data) - 1:
                    result += ', '
            # result += ')'
        else:
            result += str(data)

        # if indents > 0:
        #     result += ','
        result += '\n'

        return result

    return to_string(yson_data)


# ======================================== TOOLS FOR MAIN.PY ========================================


def add_view_time_to_native_player_opened(event_name, event_value, vsid, view_time):
    if event_name == 'native_player_opened' and vsid is not None and view_time is not None:
        event_value['view_time'] = view_time
    return event_value


def get_parent_id(event_name, event_value):
    if event_name in ['card_click', 'card_show']:
        if event_value.get('carousel_name'):
            return None
        elif event_value.get('parent_id'):
            return event_value['parent_id']
    return None


def add_carousels_titles(event_value, carousel_title):
    if carousel_title is not None:
        event_value['carousel_name'] = carousel_title
    return event_value


def get_content_id(event_name, event_value):
    if event_name in ['card_click', 'card_show'] and event_value.get('content_id'):
        return event_value['content_id']
    if event_name == 'serp_shown' and event_value.get('content_card_id'):
        return event_value['content_card_id']
    if event_name in ['vh_content_card_opened', 'search_content_card_opened', 'native_player_opened']:
        if event_value.get('title'):
            return None
        elif event_value.get('content_id'):
            return event_value['content_id']
    return None


def add_content_titles(event_name, event_value, title1, title2):
    if title1 or title2:
        if event_name == 'serp_shown':
            event_value['content_card_name'] = title1 or title2
        elif event_name in ['card_click', 'card_show', 'vh_content_card_opened', 'search_content_card_opened',
                            'native_player_opened']:
            event_value['title'] = title1 or title2
        else:
            raise RuntimeError('Event {} has joined title'.format(event_name))
    return event_value


def get_content_id_for_root_and_real_content_id(event_name, event_value):
    if event_name in ['card_show', 'card_click', 'native_player_opened']:
        return event_value.get('content_id')
    return None


def add_root_or_real_content_id(event_value, root_id1, real_id1, root_id2, real_id2):
    if root_id1 or root_id2 or real_id1 or real_id2:
        # it is very important below that root is followed by real !
        event_value['root_or_real_content_id'] = root_id1 or root_id2 or real_id1 or real_id2
    return event_value


def define_action_type(event_name):
    if event_name in ['session_init', 'session_end', 'voice_request']:
        return event_name
    elif event_name in ['account_login', 'account_change', 'account_logout']:
        return 'account'
    elif event_name in ['app_launch', 'app_install', 'app_install_page_clicked']:
        return 'apps'
    elif event_name in ['serp_shown', 'serp_opened', 'collection_opened']:
        return 'search'
    elif event_name in ['settings_opened', 'settings_changed']:
        return 'settings'
    elif event_name in ['card_click', 'card_show', 'screen_changed', 'vh_content_card_opened',
                        'search_content_card_opened']:
        return 'navigation'
    elif event_name in ['native_player_opened', 'native_player_action']:
        return 'native_watching'
    elif event_name in ['youtube_player_opened', 'youtube_player_action', 'youtube_player_heartbeat']:
        return 'youtube_watching'
    elif event_name in ['tv_player_opened', 'tv_player_heartbeat', 'tv_content_changed',
                        'tv_channel_changed', 'tv_player_action']:
        return 'tv_watching'
    elif event_name in ['webview_player_opened', 'webview_player_action', 'webview_player_heartbeat']:
        return 'webview_watching'
    elif event_name in ['hdmi_opened', 'hdmi_heartbeat']:
        return 'hdmi_watching'
    elif event_name == ' profiles_screen_opened':
        return 'profiles_screen_opened'
    elif event_name == 'screensaver_stats':
        return 'screensaver'
    elif event_name == 'music_app_launch':
        return 'music_player'
    else:
        return None


def get_timestamp_ms(event_name, event_value, ts, event_datetime):
    # define ts_ms
    if event_name == 'screensaver_stats':
        if event_value.get('client_timestamp_ms'):
            ts_ms = event_value['client_timestamp_ms']
        elif event_value.get('end_timestamp'):
            ts_ms = event_value['end_timestamp']
        else:
            ts_ms = ts * 1000
    elif event_name == 'music_app_launch':   # exclude when solve problem of music_app_launch timestamps
        try:
            ts_ms = datetime_to_timestamp(event_datetime) * 1000
        except:
            ts_ms = ts * 1000
    else:
        if event_value.get('client_timestamp_ms'):
            ts_ms = event_value['client_timestamp_ms']
        else:
            ts_ms = ts * 1000

    # correct timestamps of some events
    if event_name == 'music_app_launch':
        return ts_ms + 2000
    if event_name == 'session_end':
        return ts_ms + 2000
    if event_name == 'session_init':
        return ts_ms - 2000
    return ts_ms


def get_event_value_for_voice_request(generic_scenario, music_answer_type, music_genre, query, reply, is_muted):
    event_value = {}
    if generic_scenario:
        event_value['generic_scenario'] = generic_scenario
    if music_answer_type:
        event_value['music_answer_type'] = music_answer_type
    if music_genre:
        event_value['music_genre'] = music_genre
    if query:
        event_value['query'] = query
    if reply:
        event_value['reply'] = reply
    if is_muted:
        event_value['is_muted'] = is_muted
    return event_value


# ============================ TIME FUNCTIONS AND OTHER TOOLS FOR REDUCERS.PY ============================


def time_string_to_timedelta(timestr):
    try:
        if timestr.find('days') != -1:
            tmp = timestr.split(' days, ')
        elif timestr.find('day') != -1:
            tmp = timestr.split(' day, ')
        else:
            tmp = (0, timestr)

        days = int(tmp[0])
        hours, minutes, seconds = map(int, tmp[1].split(':'))
        total_seconds = days * 24 * 3600 + hours * 3600 + minutes * 60 + seconds
        return timedelta(seconds=total_seconds)

    except:
        raise RuntimeError('{} is incorrect time string'.format(timestr))


def seconds_to_time_string(seconds):
    return str(timedelta(seconds=seconds))


def datetime_to_timestamp(datetime_str):
    # python2 datetime.datetime has no attibute timestamp :(
    current_time = datetime.strptime(datetime_str, '%Y-%m-%d %H:%M:%S')
    zero_timestamp = datetime.strptime('1970-01-01 00:00:00', '%Y-%m-%d %H:%M:%S')
    return int((current_time - zero_timestamp - timedelta(hours=3)).total_seconds())


def timestamp_to_datetime(ts):
    return str(datetime.fromtimestamp(ts) + timedelta(hours=3))


def get_time_of_record(record):
    try:
        time = record.event_datetime[11:]
    except:
        time = ''
    return time


def get_datetime_of_record(record):
    if record.event_datetime:
        return record.event_datetime
    return ''


def correct_time_with_new_event(results, rec, events_counter):
    dt = get_datetime_of_record(rec)

    if results['action_type'] == 'screensaver':
        if events_counter == 1:   # if rec is the first screensaver event
            # define start_datetime
            try:
                start_ts = int(rec.event_value['start_timestamp']) / 1000
                start_datetime = timestamp_to_datetime(start_ts)
            except:
                start_datetime = ''

            results['time']['start'] = start_datetime

        # always define end_datetime
        try:
            end_ts = int(rec.event_value['end_timestamp']) / 1000
            end_datetime = timestamp_to_datetime(end_ts)
        except:
            end_datetime = dt   # event_datetime == event_value['end_timestamp'] for screensaver_stats events

        results['time']['end'] = end_datetime

    else:
        if events_counter == 1:   # if rec is the first event of such action_type
            results['time']['start'] = dt
        results['time']['end'] = dt   # always update end time

    return results


def prepare_record_before_yield(results, session_info, key, next_event_rec=None):
    action_type = results['action_type']

    start = results['time']['start']
    end = results['time']['end']

    # correct end datetime of event
    if next_event_rec is not None and action_type in ['music_player', 'settings', 'tv_watching', 'webview_watching',
                                                      'native_watching', 'youtube_watching', 'hdmi_watching', 'apps']:
        end = get_datetime_of_record(next_event_rec)   # next event is the end

    # set 'timedelta' field
    if start and end:
        if start > end:
            results['timedelta'] = '00:00:00'
        else:
            try:
                start_dt = datetime.strptime(start, '%Y-%m-%d %H:%M:%S')
                end_dt = datetime.strptime(end, '%Y-%m-%d %H:%M:%S')
                results['timedelta'] = str(end_dt - start_dt)
            except:
                results['timedelta'] = 'undefined'
    else:
        results['timedelta'] = 'undefined'

    # set 'date' and 'time' fields
    try:
        start_date = start[:10]
        start_time = start[11:]
        end_date = end[:10]
        end_time = end[11:]

        if start_date == end_date:
            results['date'] = start_date
            results['time'] = start_time + '  -  ' + end_time
        else:
            results['date'] = start_date + '  -  ' + end_date
            results['time'] = start + '  -  ' + end
    except:
        results['time'] = 'undefined'
        results['date'] = 'undefined'

    if action_type.find('watching') != -1:
        # correct tvt data of watching events
        heartbeats_data = []
        for recourse, tvt in results['properties'][0].items():
            heartbeats_data.append(seconds_to_time_string(tvt) + ' - ' + recourse)

        if heartbeats_data:
            results['properties'][0] = {'TVT': heartbeats_data}
        else:
            results['properties'][0] = {'TVT': results['timedelta']}

        # remove empty data
        if not results['properties'][-1]['Действия']:
            results['properties'][-1].pop('Действия')
        if not results['properties'][-1]:
            results['properties'].pop()

    # define and translate final action_type
    if action_type in ['native_watching', 'webview_watching']:
        if session_info['is_recommendation_content']:
            results['action_type'] = 'recommendation_watching'
        else:
            results['action_type'] = 'ondemand_watching'
    elif action_type == 'navigation' and session_info['is_recommendation_navigation']:
        results['action_type'] = 'recommendation_navigation'
    results['action_type'] = translate(results['action_type'])

    # create 'properties_excel' field
    results['properties_excel'] = yson_to_excel_string(results['properties'])

    # create 'user_info' field
    results['user_info'] = key.to_dict()
    for field in ['is_logged_in', 'has_plus', 'has_gift']:
        results['user_info'][field] = results[field]
        results.pop(field)

    return results


def translate(word):
    if word in DICTIONARY:
        return DICTIONARY[word]
    return word


# =============================== TOOLS FOR SESSION_INFO IN REDUCERS.PY ===============================


def update_session_info_with_new_event(session_info, rec):
    if session_info['Первое событие сессии'] is None:
        session_info['Первое событие сессии'] = rec.event_datetime
    session_info['Последнее событие сессии'] = rec.event_datetime

    # for recommendation/ondemand tvt
    if rec.event_name == 'native_player_opened':
        if rec.event_value.get('root_or_real_content_id') in session_info['shown_recommendation_content']:
            session_info['is_recommendation_content'] = True
        else:
            session_info['is_recommendation_content'] = False
    elif rec.event_name == 'webview_player_opened':
        if rec.event_value.get('startup_place', {}).get('parent_id') in session_info['recommendation_serps']:
            session_info['is_recommendation_content'] = True
        else:
            session_info['is_recommendation_content'] = False
    elif rec.event_name == 'serp_shown':
        if is_recommendation_serp(rec.event_value.get('request_string', '')):
            if 'serp_id' in rec.event_value and rec.event_value['serp_id'] not in ['', '/', '//', '///']:
                session_info['recommendation_serps'].add(rec.event_value['serp_id'].strip('/'))
    elif rec.event_name in ['card_show', 'card_click']:
        place = rec.event_value.get('place', '')
        parent_id = rec.event_value.get('parent_id', '').strip('/')
        root_or_real_content_id = rec.event_value.get('root_or_real_content_id', '')
        if place != 'search' or parent_id in session_info['recommendation_serps']:
            if root_or_real_content_id:
                session_info['shown_recommendation_content'].add(root_or_real_content_id)

    # set recommendation flag and update metrics for navigation
    if rec.action_type == 'navigation':
        # set is_recommendation_navigation flag
        if is_recommendation_navigation(rec) == 1:
            session_info['is_recommendation_navigation'] = True
        elif is_recommendation_navigation(rec) == -1:
            session_info['is_recommendation_navigation'] = False

        # update navigation metrics
        if session_info['last_navigation_event']:
            try:
                prev_event_ts = datetime_to_timestamp(session_info['last_navigation_datetime'])
                cur_event_ts = datetime_to_timestamp(get_datetime_of_record(rec))
                if cur_event_ts >= prev_event_ts:
                    nav_time = cur_event_ts - prev_event_ts
                else:
                    nav_time = 0
            except:
                nav_time = 0

            if session_info['last_navigation_event'] in ['card_show', 'card_click', 'screen_changed']:
                nav_time = min(nav_time, 5)
            elif session_info['last_navigation_event'] in ['search_content_card_opened', 'vh_content_card_opened']:
                nav_time = min(nav_time, 60)

            if session_info['is_recommendation_navigation']:
                session_info['Навигация по рекомендациям'] += nav_time
            else:
                session_info['Навигация'] += nav_time

        # update navigation data in session_info
        session_info['last_navigation_event'] = rec.event_name
        session_info['last_navigation_datetime'] = get_datetime_of_record(rec)

    # update session_info fields
    session_info['has_gift'] = rec.has_gift or session_info['has_gift']
    session_info['has_plus'] = rec.has_plus or session_info['has_plus']
    session_info['is_logged_in'] = rec.is_logged_in or session_info['is_logged_in']

    return session_info


def update_session_info_with_results(session_info, results):
    action_type = results['action_type']

    # add timedelta or TVT to aggregation metrics
    if results['timedelta'] != 'undefined' and action_type in ['Настройки', 'Приложения', 'Музыкальный плеер',
                                                               'Скринсэйвер']:
        time = time_string_to_timedelta(results['timedelta'])
        session_info[action_type] += int(time.total_seconds())
    elif action_type in ['Youtube', 'Просмотр ТВ каналов', 'HDMI', 'Просмотр рекомендательного контента',
                         'Просмотр on-demand контента']:
        tvt_data = results['properties'][0]['TVT']
        if isinstance(tvt_data, str) and tvt_data != 'undefined':
            watching_time = time_string_to_timedelta(tvt_data)
            session_info[action_type] += int(watching_time.total_seconds())
        elif isinstance(tvt_data, list):
            for elem in tvt_data:
                time_string = elem.split(' - ')[0]
                watching_time = time_string_to_timedelta(time_string)
                session_info[action_type] += int(watching_time.total_seconds())
        else:
            raise RuntimeError('tvt_data of unknown type: {}'.format(type(tvt_data)))

    # reset navigation data
    session_info['is_recommendation_navigation'] = False
    session_info['last_navigation_event'] = ''
    session_info['last_navigation_datetime'] = ''

    return session_info


def prepare_session_info_before_yield(session_info, key):
    # set 'screen_time' field
    try:
        session_init_ts = datetime_to_timestamp(session_info['Первое событие сессии'])
        session_end_ts = datetime_to_timestamp(session_info['Последнее событие сессии'])
        if session_init_ts > session_end_ts:
            session_info['Общее время сессии'] = '00:00:00'
        else:
            session_info['Общее время сессии'] = seconds_to_time_string(session_end_ts - session_init_ts)
    except:
        session_info['Общее время сессии'] = 'undefined'

    # get properties in right order and format
    properties = []
    for prop_name in SESSION_INFO_PROPERTIES:
        # convert tvt fields to format hh:mm:ss
        if prop_name in SESSION_INFO_TIMESPENT_PROPERTIES:
            session_info[prop_name] = seconds_to_time_string(session_info[prop_name])

        properties.append(prop_name + ': ' + session_info[prop_name])

    # get time and date of session_info
    try:
        start_date = session_info['Первое событие сессии'][:10]
        end_date = session_info['Последнее событие сессии'][:10]
        if start_date == end_date:
            date = start_date
            time = session_info['Первое событие сессии'][11:] + '  -  ' + session_info['Последнее событие сессии'][11:]
        else:
            date = start_date + '  -  ' + end_date
            time = session_info['Первое событие сессии'] + '  -  ' + session_info['Последнее событие сессии']
    except:
        time = 'undefined'
        date = 'undefined'

    # create 'user_info' field
    user_info = key.to_dict()
    for field in ['is_logged_in', 'has_plus', 'has_gift']:
        user_info[field] = session_info[field]

    # prepare session_info to yield
    session_info = {
        'action_type': 'Информация о сессии',
        'properties': properties,
        'properties_excel': yson_to_excel_string(properties),
        'date': date,
        'time': time,
        'timedelta': session_info['Общее время сессии'],
        'user_info': user_info
    }
    return session_info


# =============================== TOOLS FOR NAVIGATION ===============================


def is_another_action_type(rec, results, session_info):
    if rec.action_type != results['action_type'] or rec.event_name in ['native_player_opened', 'webview_player_opened']:
        return True
    if rec.action_type == 'navigation':
        if (session_info['is_recommendation_navigation'] and is_recommendation_navigation(rec) == -1 or
                not session_info['is_recommendation_navigation'] and is_recommendation_navigation(rec) == 1):
            return True
        else:
            return False
    return False


# this function has 3 values:
#  0  - event does not affect session_info['is_recommendation_navigation'] flag
# -1  - event must change session_info['is_recommendation_navigation'] to False
#  1  - event must change session_info['is_recommendation_navigation'] to True
def is_recommendation_navigation(rec):
    if rec.event_name in ['card_show', 'card_click']:
        screen = rec.event_value.get('place', '') or 'unknown'
        if screen in RECOMMENDATION_SCREENS:
            return 1
        elif screen == 'unknown':
            return 0
        else:
            return -1
    elif rec.event_name == 'screen_changed':
        to_screen = rec.event_value.get('to', '') or 'unknown'
        if to_screen in RECOMMENDATION_SCREENS or to_screen in ['unknown', 'episode_content_card', 'film_content_card',
                                                                'tv', 'native_player']:
            return 0
        else:
            return -1
    elif rec.event_name in ['vh_content_card_opened', 'search_content_card_opened']:
        return 0
    else:
        raise RuntimeError('{} is unknown event_name of navigation action_type'.format(rec.event_name))


# =============================== TOOLS FOR NAVIGATION ===============================


def create_header_and_title(rec):
    time = get_time_of_record(rec)

    screen = rec.event_value.get('place', '') or 'undefined_screen'
    screen_number = (str(SCREENS_NUMBERS[screen]) + ') ') if screen in SCREENS_NUMBERS else ''

    y = rec.event_value.get('y', -1)
    carousel_name = rec.event_value.get('carousel_name', '') or 'undefined_carousel'

    x = rec.event_value.get('x', -1)
    title = rec.event_value.get('title', '') or 'undefined_title'

    click = '    <- click' if rec.event_name == 'card_click' else ''

    # correct carousel_name
    if screen == 'main' and carousel_name == 'undefined_carousel':
        if rec.event_value.get('parent_id', '') == 'FRONTEND_CATEG_PROMO_MIXED':
            carousel_name = 'Что посмотреть'
        elif rec.event_value.get('parent_id', '') == 'delayed_tvo':
            carousel_name = 'Продолжить просмотр'
        elif rec.event_value.get('parent_id', '') == 'Недавние':
            carousel_name = 'Недавние'
        elif rec.event_value.get('parent_id', '') == 'music_main':
            carousel_name = 'Музыка для вас'
        elif rec.event_value.get('parent_id', '') == 'ya_tv/personal_series':
            carousel_name = 'Рекомендуем сериалы'
        elif rec.event_value.get('parent_id', '') == 'ya_tv/personal_films':
            carousel_name = 'Фильмы для вас'
        elif rec.event_value.get('parent_id', '') == 'ya_tv/top_items_all':
            carousel_name = 'Смотрят сейчас'
        elif rec.event_value.get('parent_id', '') == 'ya_tv/novelties':
            carousel_name = 'Новинки'
        elif rec.event_value.get('parent_id', '') == 'ya_tv/film_list_top_250_films':
            carousel_name = 'Фильмы из Топ-250 КиноПоиска'

    # correct title (and carousel name)
    if title == 'undefined_title':
        content_id = rec.event_value.get('content_id', '')
        if content_id.find('section_carousel_ya_tv') != -1:
            title = 'Карточка с подборкой контента'
        elif content_id.find('Playlist') != -1:
            title = 'Карточка плейлиста'
        elif content_id.find('http') != -1 or content_id.find('.') != -1:
            title = rec.event_value.get('content_id', '') or 'undefined_title'
        elif content_id.find('lst') != -1:
            title = 'Карточка коллекции'
            if carousel_name == 'undefined_carousel':
                parent_id = rec.event_value.get('parent_id', '')
                tmp = parent_id.split('/')
                if len(tmp) == 2 and tmp[1]:
                    carousel_name = tmp[1]
                else:
                    carousel_name = 'Карусель с коллекциями'
        elif carousel_name == 'Недавние':
            title = rec.event_value.get('content_id', '') or 'undefined_title'

    # create header and title
    if screen == 'details' and carousel_name == 'В главных ролях':
        header = 'Подробности, 1) В главных ролях'
        title = 'Карточка актера'
    elif screen == 'details' and carousel_name == 'undefined_carousel':
        header = 'Подробности, Сезоны и серии'
        title = 'Карточка эпизода'
    elif screen == 'section_grid':
        header = 'Экран с подборкой "' + carousel_name + '"'
    elif screen == 'search':
        header = '1) Поиск' + ((', номер карусели: ' + str(y + 1)) if y != -1 else '')
    elif screen == 'native_player':
        header = 'Плеер, Смотрите также'
    elif screen == 'search_collection_page':
        header = 'Экран коллекции'
    else:
        header = screen_number + translate(screen) + ', ' + ((str(y + 1) + ') ') if y != -1 else '') + carousel_name

    if x == -1 or header == 'Подробности, Сезоны и серии':
        title = time + ', ' + title + click
    else:
        title = time + ', ' + str(x + 1) + ') ' + title + click

    header = ' -> ' + header + ' <- '

    return header, title


# =============================== TOOLS FOR SERPS ===============================


def is_recommendation_serp(request_string):
    if request_string.find('Фильмы') != -1 or \
        request_string.find('фильмы') != -1 or \
        request_string.find('Кино') != -1 or \
        request_string.find('кино') != -1 or \
        request_string.find('Сериалы') != -1 or \
        request_string.find('сериалы') != -1 or \
        request_string.find('Видео') != -1 or \
        request_string.find('видео') != -1 or \
        request_string.find('Мультики') != -1 or \
        request_string.find('мультики') != -1 or \
        request_string.find('Мультфильмы') != -1 or \
        request_string.find('мультфильмы') != -1 or \
        request_string.find('Комедии') != -1 or \
        request_string.find('комедии') != -1 or \
        request_string.find('Фантастика') != -1 or \
        request_string.find('фантастика') != -1 or \
        request_string.find('Драма') != -1 or \
        request_string.find('драма') != -1 or \
        request_string.find('Драмы') != -1 or \
        request_string.find('драмы') != -1 or \
        request_string.find('Ужасы') != -1 or \
        request_string.find('ужасы') != -1 or \
        request_string.find('Ужастики') != -1 or \
        request_string.find('ужастики') != -1 or \
        request_string.find('Триллер') != -1 or \
        request_string.find('триллер') != -1 or \
        request_string.find('Аниме') != -1 or \
        request_string.find('аниме') != -1 or \
        request_string.find('Боевик') != -1 or \
        request_string.find('боевик') != -1 or \
        request_string.find('Вестерн') != -1 or \
        request_string.find('вестерн') != -1 or \
        request_string.find('Детективы') != -1 or \
        request_string.find('детективы') != -1 or \
        request_string.find('Мелодрамы') != -1 or \
        request_string.find('мелодрамы') != -1 or \
        request_string.find('Фэнтези') != -1 or \
        request_string.find('фэнтези') != -1 or \
        request_string.find('Про ') != -1 or \
        request_string.find('про ') != -1:

        return True
    return False
