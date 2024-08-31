# -*-coding: utf8 -*-
from nile.api.v1 import Record, extractors as ne
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

import json
from random import choice
from collections import defaultdict


ITEMS_COUNT = {
    'film_narrow': 6,
    'film_wide': 6,
    'video': 6
}


ITEM_CONTAINERS = [
    "video_search_gallery",
    "video_season_gallery",
    "video_description_screen"
]


PERS_CHANNELS = {
    "4461546c4debdcffbab506fd75246e19": "405601881369c62f82ba8026435cde1f",
    "4a507a704bb19b8689e36f998d002b05": "4eb953a89507c2378cf9b3b9e893782a",
    "4ee88709c215d7ab87f90eb5aedfd4f7": "405601881369c62f82ba8026435cde1f",
    "4120568776da2701896774fb798702d7": "47b32251664c4440906aca0fadf5ef38",
    "4fce9961f05336cf963ea91fccb45791": "4ae1ba2934cb649084b8de237330ea58",
    "4d124f60eb2c0703967c3ce082ffbb5b": "43cbcaa41ecd1f37926fdedb43bdccf5",
    "40d987737b3ae9c880ae68e91d35db42": "47800fe72184a87581da2197cc85c677"
}


def get_path(data, path, default=None):
    try:
        for item in path:
            data = data[item]
        return data
    except (KeyError, TypeError, IndexError):
        return default


def random_hash():
    return ''.join(map(lambda x: choice("0123456789abcdef"), xrange(16)))


def get_response(VinsResponse):
    if VinsResponse:
        return json.loads(VinsResponse)['directive']['payload']
    return 'EMPTY_VINS_RESPONSE'


def get_provider_from_url(url):
    provider = 'yavideo'
    if 'kinopoisk' in url:
        provider = 'kinopoisk'
    elif 'youtube' in url:
        provider = 'youtube'
    return provider


def get_url(item):
    url = url = item.get('url')
    uuid = item.get('provider_item_id')
    if item.get('debug_info') and item['debug_info'].get('web_page_url'):
        url =  item['debug_info']['web_page_url']
    elif item.get('provider_name') == 'youtube' and uuid:
        url = 'https://www.youtube.com/watch?v=' + uuid
    elif item.get('provider_name') == 'kinopoisk' and uuid:
        url = 'https://frontend.vh.yandex.ru/player/' + uuid
    elif item.get('provider_name') == 'strm' and uuid:
        url_uuid = PERS_CHANNELS.get(uuid) or uuid
        url = 'https://frontend.vh.yandex.ru/player/' + url_uuid
    elif item.get('provider_info') and 'http' in item['provider_info'][0].get('provider_item_id', ''):
        url = item['provider_info'][0]['provider_item_id']
    elif 'http' in item.get('play_uri', ''):
        url = item['play_uri']
    return url


def get_items(records):
    for rec in records:
        record = rec.to_dict()
        vins_response = record.get('vins_response')

        if vins_response:
            if vins_response == 'EMPTY_VINS_RESPONSE':
                record["result"] = 'empty_hyp'
            else:
                hyp = get_path(vins_response, ['megamind_analytics_info', 'original_utterance'])
                reply = get_path(vins_response, ['voice_response', 'output_speech', 'text'])
                directives = get_path(vins_response, ['response', 'directives'], [])
                analytics_info = get_path(vins_response, ['megamind_analytics_info', 'analytics_info', 'Video', 'scenario_analytics_info'], {}) or \
                    get_path(vins_response, ['megamind_analytics_info', 'analytics_info', 'alice.vins', 'scenario_analytics_info'], {})

                record["intent"] = analytics_info.get("intent")
                result_items = []

                for directive in directives:
                    if directive['name'] == "tts_play_placeholder":
                            continue
                    if directive['type'] == 'client_action':
                        record['ya_video_request'] = get_path(directive, ['payload', 'debug_info', 'ya_video_request'])
                        record['debug_url'] = get_path(directive, ['payload', 'debug_info', 'url'])
                        items = []

                        # season gallery for webview not supported
                        # ya_video_request & debug_url for webview not supported
                        # provider_name for webview not supported (currently crutched)

                        directive_name = directive['name']
                        if directive_name == 'mordovia_show':
                            if 'https://yandex.ru/portal/station' in get_path(directive, ['payload', 'url']):
                                record['display_type'] = directive_name
                            else:
                                video_obj = get_path(analytics_info, ['objects'], [])
                                if len(video_obj) > 0:
                                    for item_container in ITEM_CONTAINERS:
                                        if item_container in video_obj[0]:
                                            if get_path(video_obj[0], [item_container, 'item'], []):
                                                items = [get_path(video_obj[0], [item_container, 'item'], [])]
                                            else:
                                                items = get_path(video_obj[0], [item_container, 'items'], [])
                                            types = [item.get('type', 'video').lower() for item in items]
                                            max_type = max(set(types), key=types.count) if types else 'video'
                                            record['display_type'] = item_container.replace('video', max_type)

                        else:
                            record['display_type'] = directive_name
                            items = get_path(directive, ['payload', 'items'], []) or [get_path(directive, ['payload', 'item'])]

                        if not items:
                            continue

                        if not record.get('query_type') or record.get('query_type') not in ITEMS_COUNT:
                            continue

                        for item in items[:ITEMS_COUNT[record['query_type']]]:
                            if not item:
                                continue
                            tv_show = directive['payload'].get('tv_show_item')
                            item_obj = {}
                            item_obj["name"] = item.get('name', '')
                            if item.get("provider_item_id"):
                                item_obj['video_uuid'] = item.get("provider_item_id")
                            else:
                                item_obj['video_uuid'] = item.get("kinopoisk_id")
                            item_obj["video"] = get_url(tv_show) if tv_show else get_url(item)
                            item_obj["comment"] = 'Cезон {} - Cерия {}'.format(item.get('season', ''), item.get('episode', '')) if tv_show else ''
                            item_obj['provider_name'] = item.get('provider_name') or get_provider_from_url(item_obj["video"])
                            item_obj['result'] = None if item_obj.get('video') else 'url_error'

                            result_items.append(item_obj)

                record['items'] = result_items
                if not result_items:
                    record["result"] = 'not_video'
        else:
            record["result"] = 'vins_error'
        del record['vins_response']
        yield Record(**record)


def get_host(url):
    return url.split('/')[2]


def get_stats(groups):
    for key, records in groups:
        stats = {
            'display_types': defaultdict(int),
            'providers': defaultdict(int),
            'hosts': defaultdict(int),
            'intents': defaultdict(int),
            'results': defaultdict(int)
        }
        for record in records:
            if record.get('result'):
                stats['results'][record['result']] += 1
            if record.get('intent'):
                stats['intents'][record['intent']] += 1
            if record.get('display_type'):
                stats['display_types'][record['display_type']] += 1
            if record.get('items'):
                for item in record['items']:
                    if item.get('provider_name'):
                        stats['providers'][item['provider_name']] += 1
                    if item.get('video'):
                        stats['hosts'][get_host(item['video'])] += 1
        yield Record(**stats)


def main(uniproxy_output, basket, output_table=None, stats_table=None, pool=None, tmp_path=None):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(pool=pool,
                             templates=templates)
    job = cluster.job()

    output_table = output_table or "//tmp/robot-voice-qa/quasar_video_tasks_" + random_hash()
    stats_table = stats_table or "//tmp/robot-voice-qa/quasar_video_stats_" + random_hash()

    results = job.table(uniproxy_output) \
        .project(request_id='RequestId', vins_response=ne.custom(get_response, 'VinsResponse')) \
        .join(job.table(basket), type='right', by='request_id') \
        .project(
            'request_id',
            'text',
            'query_type',
            'scenario_type',
            'voice_url',
            'vins_response'
        ) \
        .map(get_items) \
        .put(output_table)

    results.groupby().reduce(get_stats).put(stats_table)
    job.run()

    return [
        {"cluster": "hahn", "table": output_table},
        {"cluster": "hahn", "table": stats_table}
    ]


if __name__ == '__main__':
    call_as_operation(main)
