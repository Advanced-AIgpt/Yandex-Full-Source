# coding: utf-8

from future import standard_library  # noqa
standard_library.install_aliases()  # noqa

import re
import json
import urllib.parse
from datetime import datetime
from collections import OrderedDict
from alice.analytics.utils.json_utils import get_path_str


def select_location(app, generic_scenario, exact_location, location_by_coordinates, location_by_region_id, location_by_client_ip):
    if (app in ['navigator', 'auto'] or generic_scenario in ('find_poi', 'route')) and exact_location is not None and exact_location != '':
        return exact_location
    if location_by_coordinates is not None:
        return location_by_coordinates
    if location_by_region_id is not None:
        return location_by_region_id
    if location_by_client_ip is not None:
        return location_by_client_ip


def prepare_voice_url(voice_url):
    if voice_url:
        scheme, netloc, path, params, query, fragment = list(urllib.parse.urlparse(voice_url))
        path = path.replace('getfile', 'getaudio')
        old_query_dict = dict(urllib.parse.parse_qsl(query))
        new_query_dict = OrderedDict({'norm': '1'})
        new_query_dict.update(old_query_dict)
        query = urllib.parse.urlencode(new_query_dict)  # updating query component, always saved at position [4]
        return urllib.parse.urlunparse(map(str, (scheme, netloc, path, params, query, fragment)))


def get_voice_url_from_mds_key(mds_key):
    """builds voice_url from mds_key, recognizes asr_key based on substring
       uses latest domain `speechbase.voicetech.yandex-team.ru`"""
    voice_url_template = 'https://speechbase.voicetech.yandex-team.ru/getaudio/{}?norm=1'
    if 'asr-logs' in mds_key:
        voice_url_template += '&storage-type=s3&s3-bucket=voicelogs'
    return voice_url_template.format(mds_key)


def to_seconds(dt):
    return int((dt - datetime(1970, 1, 1)).total_seconds())


def parse_downloader_client_time(client_time):
    if client_time:
        dt = datetime.strptime(client_time, '%Y%m%dT%H%M%S')
        return to_seconds(dt)


def parse_slot_value(slot):
    JSON_TYPES = ['geo', 'forecast', 'sys.datetime']
    value = get_path_str(slot, 'typed_value.string')
    if value == 'null':
        return value

    accepted_types = slot.get('accepted_types', [])
    if accepted_types:
        if 'string' in accepted_types:
            return value
        if 'num' in accepted_types:
            return int(value)
        for slot_type in JSON_TYPES:
            if slot_type in accepted_types:
                return json.loads(value)
    return value


MDS_KEY_PATTERN = re.compile(r'screenshots/([\w-]+?)\.png')


def extract_key_from_mds_filename(filename):
    if not filename:
        return None
    matches = MDS_KEY_PATTERN.search(filename)
    if matches:
        return matches.group(1)
    return filename


def split_sessions_by_duplicates(records_list):
    """
    Разбивает сессию из запросов пользователя на несколько сессий
    Если среди запросов есть дубли по req_id, то для дубликата создаётся отдельная сессий
    Дублирование req_id проверяется только для последнего (оцениваемого) запроса
    Полагается, что в списке `records_list` находятся упорядоченные запросы для одного session_id
    :param list[dict] records_list:
    :return Iterator[list[dict]]:
    """
    if len(records_list) >= 2 and records_list[-1]['req_id'] == records_list[-2]['req_id']:
        yield records_list[:-1]  # всё, кроме последнего запроса
        yield records_list[:-2] + [records_list[-1]]  # всё, кроме предпоследнего запроса
    else:
        # по умолчанию, если дубликатов нет, возвращаем запросы as is
        yield records_list


def get_childness(bioresponse):
    if not bioresponse:
        return 'adult'
    bioresponse = json.loads(bioresponse)
    for result in bioresponse['directive']['payload']['classification_results']:
        if result['tag'] == 'children':
            return result['classname']
    return 'adult'


def get_open_uri_url(directives):
    if not directives:
        return None
    """Возвращает ссылку на которую ведёт директива open_uri. Или None, если такой директивы не было"""
    for directive in directives:
        if directive.get('name') == 'open_uri' and 'payload' in directive:
            return get_path_str(directive, 'payload.uri')
    return None


def get_shortcut_nav_url(analytics_info):
    """Возвращает действие, которое выполняет шорткат. Или None, если OpenAppsFixlist отсутсвует"""
    data = get_path_str(analytics_info, 'analytics_info.OpenAppsFixlist', {})

    for frame in data.get('matched_semantic_frames', []):
        if 'alice.apps_fixlist' in frame.get('name', ''):
            for slot in frame.get('slots', []):
                if slot.get('name') == 'app_data' and slot.get('type') == 'custom.app_data':
                    try:
                        slot_value = json.loads(slot['value'])
                        return get_path_str(slot_value, 'nav.url._')
                    except:
                        return None

    return None
