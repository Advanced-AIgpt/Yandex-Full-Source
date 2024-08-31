#!/usr/bin/env python
# -*-coding: utf8 -*-
import json
from os import path
from collections import OrderedDict
from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
    filters as nf,
    Record,
    with_hints,
)
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from qb2.api.v1 import typing
from nile.api.v1.datetime import date_range
from usage_fields import extract_intent, get_slot_name, convert_analytics_info_to_proto


WEBVIEW_SCREEN_MAPPING = {
    "filmsSearch": {"webview_type": "movie", "action_type": "gallery"},
    "videoSearch": {"webview_type": "video", "action_type": "gallery"},
    "channels": {"webview_type": "channel", "action_type": "gallery"},
    "videoEntity": {"webview_type": "description", "action_type": "description"},
    "videoEntity/Description": {"webview_type": "description", "action_type": "description"},
    "videoEntity/Carousel": {"webview_type": "description", "action_type": "description"},
    "videoEntity/RelatedCarousel": {"webview_type": "related", "action_type": "gallery"},
    "videoEntity/Seasons": {"webview_type": "season", "action_type": "gallery"},
    'morda/blogger': {"webview_type": "blogger", "action_type": "carousel"},
    'morda/movie': {"webview_type": "movie", "action_type": "carousel"},
    'morda/series': {"webview_type": "series", "action_type": "carousel"},
    'morda/kids': {"webview_type": "kids", "action_type": "carousel"},
    'morda/home': {"webview_type": "home", "action_type": "main"},  # redefined as action_type=carousel for carousels with offset>0
    'morda/channels': {"webview_type": "channel", "action_type": "carousel"},
}


CAROUSEL_TYPES = {
    'promo',
    'recommendation',
    'delayed_view'
}


NAVIGATION_INTENTS = {
    'go_backward',
    'go_forward',
    'go_down',
    'go_up',
    'go_home',
    'go_to_the_beginning',
    'go_to_the_end',
    'ether_show'
}


ITEM_KEYS_INT = {
    "duration",
    "release_year",
    "min_age",
    "view_count"
}


ITEM_KEYS_STRING = {
    'name',
    'type',
    'genre',
    'source',
    'provider_name',
    'provider_item_id',
    'tv_show_item_id'
}


ITEM_KEYS_DOUBLE = {
    'rating'
}

MAX_INT = 2 ** 63 - 1
MIN_INT = 0


def get_schema(keys, value):
    return [(k, typing.Optional[value]) for k in keys]


SCHEMA = OrderedDict([
    ("uuid", typing.Optional[typing.String]),
    ("puid", typing.Optional[typing.String]),
    ("server_time", typing.Optional[typing.Int64]),
    ("action_type", typing.Optional[typing.String]),
    ("webview_type", typing.Optional[typing.String]),
    ("carousel_info", typing.Optional[typing.Yson]),
    ("position", typing.Optional[typing.Int64]),
    ("subscription", typing.Optional[typing.String]),
    ("license", typing.Optional[typing.String]),
    ("iron_type", typing.Optional[typing.String]),
    ("confirmed", typing.Optional[typing.Bool]),
    ("selected_item_id", typing.Optional[typing.String]),
    ("selected_show_id", typing.Optional[typing.String]),
    ("seen_in", typing.Optional[typing.List[typing.String]]),
    ("query", typing.Optional[typing.String]),
    ("url", typing.Optional[typing.String]),
    ("puid_ratings_cnt", typing.Optional[typing.Int64]),
    ("other", typing.Optional[typing.Yson])
]
    + get_schema(ITEM_KEYS_INT, typing.Int64)
    + get_schema(ITEM_KEYS_STRING, typing.String)
    + get_schema(ITEM_KEYS_DOUBLE, typing.Float)
)


SCHEMA_TYPES = OrderedDict([
    ("provider_item_id", typing.Optional[typing.String]),
    ("update_time", typing.Optional[typing.Int64]),
    ("iron_type", typing.Optional[typing.String])
])


def get_path(data, path, default=None):
    try:
        for item in path:
            data = data[item]
        return data
    except (KeyError, TypeError, IndexError):
        return default


def fix_int_type(record_value):
    if record_value is not None and not isinstance(record_value, int):
        try:
            record_value = int(record_value)
        except ValueError:
            record_value = None
    if not MIN_INT <= record_value <= MAX_INT:
        record_value = None

    return record_value


def get_provider_item_id(item):
    return get_path(item, ['metaforback', 'uuid']) or item.get('provider_item_id')


def get_tv_show_item_id(item):
    return get_path(item, ['metaforback', 'serial_id']) or item.get('provider_info', [{}])[0].get('tv_show_item_id')


def get_carousel_info(item):
    pos = get_path(item, ['metaforlog', 'source_carousel_position'])
    int_pos = int(pos) if pos else None
    info = {
        "type":  get_path(item, ['metaforlog', 'source_carousel']),
        "id": get_path(item, ['metaforlog', 'source_carousel_id']),
        "position": int_pos
    }
    return info


def update_record(record, item):
    for item_key in ITEM_KEYS_INT | ITEM_KEYS_STRING | ITEM_KEYS_DOUBLE:
        record[item_key] = item.get(item_key)

        if item_key in ITEM_KEYS_INT:
            record[item_key] = fix_int_type(record[item_key])

    return record


def update_webview_record(record, item):
    record["provider_item_id"] = get_provider_item_id(item)
    record["tv_show_item_id"] = get_tv_show_item_id(item)
    record["position"] = item.get("number")
    record["name"] = item.get('title')
    record["duration"] = fix_int_type(get_path(item, ['metaforback', 'duration']) or get_path(item, ['metaforlog', 'duration']))  # none yet for video_webview
    record["rating"] = get_path(item, ['metaforlog', 'rating_kp'])
    record["min_age"] = fix_int_type(get_path(item, ['metaforlog', 'restriction_age']))
    record["genre"] = get_path(item, ['metaforlog', 'genres'])
    record["view_count"] = fix_int_type(get_path(item, ['metaforlog', 'views_count']))
    record["release_year"] = get_path(item, ['metaforlog', 'release_year'])
    release_year = record["release_year"]
    if release_year and isinstance(release_year, str) and len(release_year) > 0:
        record["release_year"] = int(release_year.split("-")[0])  # There are dates in release_year field sometimes
    record["carousel_info"] = get_carousel_info(item)
    if record["carousel_info"]["position"] > 0:
        record["action_type"] = "carousel"
    record["other"] = {}
    return record


def update_previously_seen(previously_seen, record):
    item_id = record['provider_item_id']
    show_id = record['tv_show_item_id']
    source = record['action_type']
    for track_id in [item_id, show_id]:
        if track_id in previously_seen:
            if previously_seen[track_id][-1] != source:
                previously_seen[track_id].append(source)
        elif track_id:
            previously_seen[track_id] = [source]
    return previously_seen


def get_meta(directives, intent):
    if 'video' in intent and ('select' in intent or 'open' in intent):
        for directive in directives:
            selected_id = get_path(directive, ["payload", "item", "provider_item_id"])
            if selected_id:
                tv_show_item_id = get_path(directive, ["payload", "item", "tv_show_item_id"])
                if tv_show_item_id:
                    return [selected_id, tv_show_item_id]
                else:
                    return [selected_id]
    else:
        suffix = intent.split('\t')[-1]
        if suffix in NAVIGATION_INTENTS:
            return suffix


def get_url_query(directives, request):
    cur_url, cur_query = None, None
    for i in directives:
        if 'gallery' in i.get('name', ''):
            urls = get_path(i, ["payload", "debug_info", "url"], [None])
            cur_url = urls[0] or cur_url
            cur_query = get_path(request, ['utterance', 'text']) or cur_query
        elif i.get('name') == 'mordovia_show':
            webview_url = get_path(i, ["payload", "url"], '')
            for webview_screen in WEBVIEW_SCREEN_MAPPING:
                if webview_screen in webview_url:
                    cur_query = get_path(request, ['utterance', 'text'])
    return cur_url, cur_query


def get_type(onto_type, onto_tags, detailed_tags, ugc_categories, computed_program, computed_channel):
    computed_program = computed_program or ''
    computed_program = computed_program.decode('utf-8').strip().lower()
    computed_channel = computed_channel or ''
    computed_channel = computed_channel.decode('utf-8').strip().lower()
    detailed_tags = detailed_tags or ''
    ugc_categories = ugc_categories or ''
    tp = None
    if onto_type and 'Film/Film' in onto_type:
        tp = "movie"
        if onto_tags and "Animation@on" in onto_tags:
            tp = "anim_" + tp
    elif onto_type and 'Film/Series@on' in onto_type:
        tp = "tv_show"
        if onto_tags and "Animation@on" in onto_tags:
            tp = "anim_" + tp
    elif onto_tags and "TVprogram@on" in onto_tags:
        tp = "tv_stream"
    elif 'sport' in detailed_tags or 'sport' in ugc_categories or u'спорт' in computed_program:
        tp = 'sport'
    elif 'news' in detailed_tags or u'новости' in computed_channel or u'новости' in computed_program:
        tp = 'news'
    elif 'music' in detailed_tags or 'music' in ugc_categories or u'музыка' in computed_channel or u'музыка' in computed_program:
        tp = 'music'
    elif 'kids' in detailed_tags:
        tp = 'kids'
    else:
        tp = 'video'
    return tp


@with_hints(output_schema=SCHEMA_TYPES)
def map_events(records):
    for rec in records:
        iron_type = get_type(rec.onto_type, rec.onto_tags, rec.detailed_tags, rec.ugc_categories, rec.computed_program, rec.computed_channel)
        new_record = {}
        new_record['provider_item_id'] = rec['UUID']
        new_record['update_time'] = rec['UpdateTime']
        new_record['iron_type'] = iron_type
        yield Record(**new_record)


@with_hints(output_schema=SCHEMA)
def reduce_events(groups):
    if not groups:
        yield Record({})

    for key, records in groups:
        previously_seen = {}  # by track
        previously_seen_screens = set()  # by screen (concatenated screen track_ids)
        previously_played = set()  # by track
        if not records:
            continue
        cur_query, cur_url = None, None

        for rec in records:
            request = rec.request
            response = rec.response
            directives = get_path(response, ['directives'], [])
            is_plugged_in = get_path(request, ['device_state', 'is_tv_plugged_in'], False)
            if not is_plugged_in:
                continue

            new_record = {}
            new_record['uuid'] = key['uuid']
            new_record['puid'] = rec['puid']
            new_record['server_time'] = rec['server_time']
            new_record['confirmed'] = False
            video = get_path(request, ['device_state', 'video'], {})
            current_screen = video.get('current_screen') or ''
            slot_name = get_slot_name(rec.form_name or '')
            intent = extract_intent(rec.form_name or '', rec.form or {}, response, rec.callback_args, response.get('cards'), rec.analytics_info,
                                    None, slot_name, {}, convert_analytics_info_to_proto(rec.analytics_info),
                                    json.dumps(rec.callback_args) if rec.callback_args else '{}') or ''
            intent = intent.lower()

            prev_query = cur_query
            prev_url = cur_url
            cur_url, cur_query = get_url_query(directives, request)

            if current_screen == 'video_player':
                new_record['action_type'] = 'play'
                new_record['confirmed'] = True
                item = get_path(video, ['currently_playing', 'item'])
                if not item:
                    continue

                new_record = update_record(new_record, item)

                track_id = new_record["provider_item_id"]
                show_id = new_record["tv_show_item_id"]
                if (track_id and track_id != "" and track_id in previously_played):
                    continue
                previously_played.add(track_id)

                if track_id in previously_seen:
                    new_record['seen_in'] = previously_seen[track_id]
                elif show_id in previously_seen:
                    new_record['seen_in'] = previously_seen[show_id]

                yield Record(**new_record)

            elif current_screen == 'description':
                new_record['action_type'] = 'description'
                new_record['confirmed'] = True
                item = get_path(video, ['screen_state', 'item'])
                if not item:
                    continue
                new_record = update_record(new_record, item)
                yield Record(**new_record)

            elif 'gallery' in current_screen:
                new_record['action_type'] = 'gallery'
                new_record['query'] = prev_query
                new_record['url'] = prev_url
                items = get_path(video, ['screen_state', 'items']) or []
                visible_items = get_path(video, ['screen_state', 'visible_items']) or []
                meta = get_meta(directives, intent)
                for i, item in enumerate(items):
                    if i not in visible_items:
                        continue
                    new_record['position'] = i
                    if meta:
                        new_record['confirmed'] = True
                        if isinstance(meta, list):
                            new_record["selected_item_id"] = meta[0]
                            if len(meta) == 2:
                                new_record["selected_show_id"] = meta[1]
                    new_record = update_record(new_record, item)

                    previously_seen = update_previously_seen(previously_seen, new_record)

                    yield Record(**new_record)

            elif current_screen == 'mordovia_webview':
                if get_path(video, ['screen_state', 'scenario'], '') == 'onboarding':
                    continue

                sections = get_path(video, ['view_state', 'sections']) or get_path(video, ['page_state', 'sections']) or [{}]
                sections = sections[0] if isinstance(sections, list) and len(sections) > 0 else {}

                webview_screen = get_path(video, ['view_state', "currentScreen"], '').replace('\\', '')
                if webview_screen in WEBVIEW_SCREEN_MAPPING:
                    new_record['query'] = prev_query
                    new_record['url'] = prev_url
                    new_record['confirmed'] = True
                    new_record.update(WEBVIEW_SCREEN_MAPPING[webview_screen])
                    if new_record['action_type'] == 'description':
                        current_item = sections.get('current_item') or sections.get('current_tv_show_item') or {}
                        if not current_item:
                            continue
                        new_record = update_webview_record(new_record, current_item)
                        if current_item.get("main_trailer_uuid"):
                            new_record["other"]["has_trailer"] = True
                        yield Record(**new_record)
                else:
                    new_record['action_type'] = sections.get('type', 'none')  # main/carousel
                    new_record['webview_type'] = 'home'

                items = sections.get('items') or []
                meta = get_meta(directives, intent)
                screen_hash = "_".join([str(get_provider_item_id(item)) for item in items if item.get('active')])
                if screen_hash in previously_seen_screens and not meta:  # no change in screen items + no item selection
                    continue
                else:
                    previously_seen_screens.add(screen_hash)

                for item in items:
                    if not item.get('active'):
                        continue
                    new_record = update_webview_record(new_record, item)

                    if meta:
                        new_record['confirmed'] = True
                        if isinstance(meta, list):
                            new_record["selected_item_id"] = meta[0]
                            if len(meta) == 2:
                                new_record["selected_show_id"] = meta[1]

                    previously_seen = update_previously_seen(previously_seen, new_record)

                    yield Record(**new_record)


def prepare_table(job, date, sessions_root, output_root):
    licenses = (job.table('//home/videolog/strm_meta/iron_branch/concat')
                        .project('license', 'UpdateTime', provider_item_id='UUID')
                        .groupby('provider_item_id')
                        .aggregate(license=na.last('license', by='UpdateTime')))

    types = (job.table('//home/videolog/strm_meta/iron_branch/concat')
                        .map(map_events)
                        .groupby('provider_item_id')
                        .aggregate(iron_type=na.last('iron_type', by='update_time')))

    ratings = (job.table('//home/dict/recommender/actions/latest')
                        .filter(
                            nf.custom(lambda x: x in (1, 2, 10), 'action_type'),  # AT_Rate, AT_KinopoiskRate, AT_NotInterested
                            nf.custom(lambda x: x.startswith("p/"), 'id'))
                        .project(puid=ne.custom(lambda x: x.strip("p/"), 'id'))
                        .groupby('puid')
                        .aggregate(puid_ratings_cnt=na.count()))

    (job.table(path.join(sessions_root, date))
                        .groupby('uuid').sort('server_time')
                        .reduce(reduce_events)
                        .project(ne.all(exclude=['license', 'iron_type', 'puid_ratings_cnt']))
                        .join(licenses, type='left', by='provider_item_id',  assume_unique_right=True)
                        .join(types, type='left', by='provider_item_id',  assume_unique_right=True)
                        .join(ratings, type='left', by='puid',  assume_unique_right=True)
                        .put(path.join(output_root, date), schema=SCHEMA))

    return job


def main(date, pool="voice",
         sessions_root='//home/voice/vins/logs/dialogs',
         output_root='//home/alice/dialog/videos'):

    if isinstance(date, (list, tuple)):
        start_date, end_date = date
        for date in date_range(start_date, end_date):
            prepare_table(date, pool, sessions_root, output_root)
        return {'out_first_path': path.join(output_root, start_date),
                'out_last_path': path.join(output_root, end_date)}

    templates = {"job_root": "//tmp/robot-voice-qa"}
    cluster = hahn_with_deps(
        pool=pool,
        templates=templates,
        neighbours_for=__file__,
        neighbour_names=['usage_fields.py'])
    job = cluster.job().env()

    job = prepare_table(job, date, sessions_root, output_root)
    job.run()

    return {'out_path': path.join(output_root, date)}


if __name__ == '__main__':
    call_as_operation(main)
    # main('2020-01-01') # local run
