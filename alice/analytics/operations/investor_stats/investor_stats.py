# -*-coding: utf8 -*-
import requests, datetime, sys
from os import path
from functools import partial
from collections import OrderedDict, defaultdict
from nile.api.v1 import (
    clusters,
    Record,
    with_hints,
)
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from utils.statface.auth import make_auth_headers
from qb2.api.v1 import typing


APP_MAPPING = {
    'Mobile': {'device_type': '\tR\tmobile\t', 'browser': '_total_'},
    'Desktop': {'device_type': '\tR\tdesktop\t', 'browser': '_total_'},
    'Search App': {'device_type': '\tR\tmobile\t', 'browser': 'YandexSearch'}
}

SCHEMA = OrderedDict([
    ("month", typing.Optional[typing.String]),
    ("app", typing.Optional[typing.String]),
    ("voice_searches", typing.Optional[typing.Float]),
    ("searches", typing.Optional[typing.Float]),
    ("voice_searches_share", typing.Optional[typing.Float])
])


def get_path(data, path, default=None):
    try:
        for item in path:
            data = data[item]
        return data
    except (KeyError, TypeError, IndexError):
        return default


def get_maps_data(headers, start_date, end_date):
    url = 'https://upload.stat.yandex-team.ru/_api/statreport/json/Mobile_Soft_Maps/Adhoc/Dashboard/MobileGeosearchBySearchTypeBySourceByGeography'
    params = {
        'scale': 'd',
        'app_platform': '_total_',
        'adv_type': '_total_',
        'geo_path': '\t10000\t',
        'search_type': '\tR\t',
        'max_distance': 3,
        'fields': 'searches',
        '_allow_default_dimensions': 0,
        'search_type__mode': 'treelevel_with_defined_level',
        'source__mode': 'treelevel_with_defined_level',
        'geo_path__mode': 'treelevel_with_defined_level',
        'search_type__lvl': 0,
        'source__lvl': 1,
        'geo_path__lvl': 0,
        'date_min': start_date.strftime('%d.%m.%Y'),
        'date_max': end_date.strftime('%d.%m.%Y')
    }

    r = requests.get(url, params=params, headers=headers)

    data = {}
    for i in r.json().get('values'):
        month = datetime.datetime.strptime(i.get('fielddate'), '%Y-%m-%d %H:%M:%S').date().strftime('%Y-%m')
        if month not in data:
            data[month] = defaultdict(float)
        if i.get('source') == '\tR\t_voice_\t':
            data[month]['voice_searches'] += i.get('searches', 0)
        elif i.get('source') == '\tR\t_text_\t':
            data[month]['text_searches'] += i.get('searches', 0)
        if data[month].get('text_searches') and data[month].get('voice_searches'):
            data[month]['searches'] += data[month]['text_searches'] + data[month]['voice_searches']

    return data


def get_searches_data(headers, start_date, end_date, app):
    url = 'https://upload.stat.yandex-team.ru/_api/statreport/json/Statface/KeyReports/Searches'
    params = {
        'scale': 'm',
        'browser_version': '\tR\t',
        'region': 10000,
        'os': '_total_',
        'entry_point': '_total_',
        'fields': 'hits',
        '_allow_default_dimensions': 0,
        'device_type__mode': 'treelevel_with_defined_level',
        'device_type__lvl': 1,
        'browser_version__mode': 'treelevel_with_defined_level',
        'browser_version__lvl': 0,
        'date_min': start_date.strftime('%d.%m.%Y'),
        'date_max': end_date.strftime('%d.%m.%Y')
    }
    app_params = APP_MAPPING[app]
    params.update(app_params)
    r = requests.get(url, params=params, headers=headers)

    data = {}
    for i in r.json().get('values'):
        month = datetime.datetime.strptime(i.get('fielddate'), '%Y-%m-%d %H:%M:%S').date().strftime('%Y-%m')
        if month not in data:
            data[month] = defaultdict(float)
        data[month]['searches'] += i.get('hits', 0)

    return data


def mapper(records, add_data):
    for record in records:
        record = record.to_dict()
        app = record.get('app')
        month = record.get('month')
        if not record.get('voice_searches_share') and get_path(add_data, [app, month]):
            record['voice_searches'] = record.get('voice_searches', 0.0) + get_path(add_data, [app, month, 'voice_searches'], 0.0)
            record['searches'] = record.get('searches', 0.0) + get_path(add_data, [app, month, 'searches'], 0.0)
            record['voice_searches_share'] = 100.0*record['voice_searches']/record['searches']
        yield Record(**record)


def main(start_date, end_date, pool = "voice",
        in_table = '//home/voice/datasets/investor_stats/total4',
        out_table = '//home/voice/datasets/investor_stats/total4'):
    start_date = datetime.datetime.strptime(start_date, '%Y-%m-%d').date()
    end_date = datetime.datetime.strptime(end_date, '%Y-%m-%d').date()
    headers = make_auth_headers(None, None, None)
    add_data = {}
    add_data["Maps on Mobile"] = get_maps_data(headers, start_date, end_date)
    for app in APP_MAPPING:
        add_data[app] = get_searches_data(headers, start_date, end_date, app)

    templates = {"job_root": "//tmp/robot-voice-qa"}
    cluster = hahn_with_deps(pool=pool, templates=templates)
    job = cluster.job().env()
    partial_mapper = partial(mapper, add_data=add_data)

    map_table = (job.table(in_table)
                .map(partial_mapper)
                .put(out_table, schema=SCHEMA))
    job.run()

    return {'out_path': out_table}


if __name__ == '__main__':
    call_as_operation(main)
    # main('2020-01-01') # local run
