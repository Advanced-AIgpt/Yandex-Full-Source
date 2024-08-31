#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import codecs
import argparse
import urllib
import urlparse
import os
import sys
import json
import click
import itertools
import random

from random import randint, choice
from uuid import uuid4, UUID

import yt.wrapper as yt

from vins_core.dm.request_events import make_asr_result
from vins_core.utils.datetime import utcnow, datetime_to_timestamp
from vins_core.utils.misc import gen_uuid_for_tests

from navi_app.lib.mockdata import FAVOURITES_MOCK


def post_ammo(body, params, url_parsed, tag=None, http_type='1.0', content_type='application/x-www-form-urlencoded',
              close_connection=True):
    url = urlparse.urlunparse((None, None, url_parsed.path, None, urllib.urlencode(params), None))
    connection_str = "Connection: Close\r\n" if close_connection else ""
    req = (
        "POST %s HTTP/%s\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: %s\r\n"
        "HOST: %s\r\n"
        "%s\r\n"
        "%s\r\n\r\n"
    ) % (url, http_type, len(body), content_type, url_parsed.netloc, connection_str, body)

    if tag is None:
        return "%d\n%s" % (len(req), req)
    return "%d %s\n%s" % (len(req), tag, req)


def post_ammo_final():
    return "0\n"


def get_ammo_header(host):
    return \
        "[Connection: close]\n" \
        "[Host: %s]\n" \
        "[Cookie: None]\n" % host


def get_ammo(url_parsed, params, tag=None):
    if tag is None:
        return url_parsed.path + urllib.urlencode(params) + '\n'
    return ''.join([url_parsed.path, urllib.urlencode(params), ' ', tag, '\n'])


YANDEX_LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}


def navi_ammo(instream, outstream, url_parsed, tag, limit):
    for i, line in enumerate(instream):
        if i >= limit:
            break

        favourites = None
        current_route = None
        map_view = {
            'tl_lat': 55.7307,
            'tl_lon': 37.6351,
            'tr_lat': 55.7158,
            'tr_lon': 37.6058,
            'br_lat': 55.7434,
            'br_lon': 37.5701,
            'bl_lat': 55.7536,
            'bl_lon': 37.5903
        }

        if isinstance(line, basestring):
            text = line.rstrip('\r\n')
            favourites = [{'name': f[0], 'lat': f[2], 'lon': f[1]} for f in FAVOURITES_MOCK]
        else:
            text = line['utterance_text']
            if 'favourites' in line:
                favourites = [
                    {'name': fav['name'], 'lat': fav['lat'], 'lon': fav['lat']} for fav in line['favourites']
                ]
            if 'points' in line:
                current_route = {
                    'points': [{'lat': point['lat'], 'lon': point['lon']} for point in line['points']],
                    'distance_to_destination': 7093.17,
                    'raw_time_to_destination': 723,
                    'arrival_timestamp': 1486736347,
                    'time_to_destination': 1200,
                    'time_in_traffic_jam': 600,
                    'distance_in_traffic_jam': 2000
                }
            if 'map_view' in line:
                map_view = {
                    'tl_lat': line['map_view']['tl_lat'],
                    'tl_lon': line['map_view']['tl_lon'],
                    'tr_lat': line['map_view']['tr_lat'],
                    'tr_lon': line['map_view']['tr_lon'],
                    'br_lat': line['map_view']['br_lat'],
                    'br_lon': line['map_view']['br_lon'],
                    'bl_lat': line['map_view']['bl_lat'],
                    'bl_lon': line['map_view']['bl_lon'],
                }

        if not text:
            continue

        uuid = str(uuid4())

        device_state = {
            'navigator': {
                'map_view': map_view,
                'user_favorites': favourites or [],
                'current_route': current_route or {},
            }
        }

        ammo = generate_sk_request(uuid=uuid, text=text, text_source='voice', device_state=json.dumps(device_state))

        outstream.write(post_ammo(
            ammo,
            {},
            url_parsed,
            tag=tag,
            http_type='1.1',
            content_type='application/json',
            close_connection=False,
        ))

    outstream.write(post_ammo_final())


def generate_sk_request(uuid, text=None, text_source=None, callback_name=None, callback_args=None, experiments=None,
                        app_id=None, device_state=None, oauth_token=None, request_id_rd=None):
    if text is not None and text_source is not None:
        if text_source == 'text':
            event = {
                'type': 'text_input',
                'text': text
            }
        elif text_source == 'voice':
            event = {
                'type': 'voice_input',
                'asr_result': make_asr_result(text),
            }
        elif text_source == 'suggested':
            event = {
                'type': 'suggested_input',
                'text': text,
            }
        else:
            raise ValueError('Unexpected utterance source %s for text %s' % (text_source, text))

    elif callback_name is not None and callback_args is not None:
        event = {
            'type': 'server_action',
            'name': callback_name,
            'payload': callback_args,
        }
    elif text_source is not None and callback_args is not None:
        if text_source == 'image':
            event = {
                'type': 'image_input',
                'payload': callback_args,
            }
        elif text_source == 'music':
            event = {
                'type': 'music_input',
                'payload': callback_args,
            }
        else:
            raise ValueError('Unexpected utterance source %s' % text_source)
    else:
        raise ValueError('Unknown event, please check input data')

    curr_time = utcnow()

    if request_id_rd:
        request_id = str(UUID(int=request_id_rd.getrandbits(128)))
    else:
        request_id = str(uuid4())

    request = {
        'header': {
            'request_id': request_id,
        },
        'application': {
            'uuid': str(uuid),
            'client_time': curr_time.strftime('%Y%m%dT%H%M%S'),
            'app_id': app_id or 'com.yandex.vins.shooting',
            'app_version': '1.2.3',
            'os_version': '5.0',
            'platform': 'android',
            'lang': 'ru-RU',
            'timezone': 'Europe/Moscow',
            'timestamp': str(datetime_to_timestamp(curr_time)),
        },
        'request': {
            'event': event,
            'location': YANDEX_LOCATION,
            'device_state': json.loads(device_state),
            'additional_options': {
                'bass_options': {
                    'client_ip': '95.108.175.247'
                }
            }
        }
    }

    if experiments:
        request['request']['experiments'] = experiments

    if oauth_token is not None:
        request['request']['additional_options']['oauth_token'] = oauth_token

    return json.dumps(request)


def speechkit_ammo(instream, outstream, url_parsed, tag, limit, experiments=None, app_id=None,
                   oauth_token=None, request_id_seed=None):
    # Assume 10 messages per user
    random_uuids_pool = [gen_uuid_for_tests(str(i)) for i in range(int(min(limit, 100000)) // 10)]

    request_id_rd = None
    if request_id_seed is not None:
        request_id_rd = random.Random()
        request_id_rd.seed(request_id_seed)

    with click.progressbar(instream, bar_template='[%(bar)s] %(info)s', show_pos=True, width=50) as src:
        for i, line in enumerate(itertools.islice(src, limit)):
            try:
                if isinstance(line, basestring):
                    uuid = choice(random_uuids_pool)
                    ammo = generate_sk_request(
                        uuid,
                        text=line.rstrip('\r\n'),
                        text_source='voice',
                        experiments=experiments,
                        request_id_rd=request_id_rd)
                else:
                    ammo = generate_sk_request(
                        line['uuid'],
                        line['utterance_text'],
                        line['utterance_source'],
                        line['callback_name'],
                        line['callback_args'],
                        experiments=experiments,
                        app_id=app_id,
                        device_state=line.get('device_state'),
                        oauth_token=oauth_token,
                        request_id_rd=request_id_rd
                    )
                outstream.write(post_ammo(
                    ammo,
                    {},
                    url_parsed,
                    tag=tag,
                    http_type='1.1',
                    content_type='application/json',
                    close_connection=False,
                ))
            except Exception as e:
                print("Error processing line: {}\n{}".format(e, line))
    outstream.write(post_ammo_final())


def geosearch_ammo(instream, outstream, url_parsed, tag, limit):
    outstream.write(get_ammo_header(url_parsed.netloc))
    for i, line in enumerate(instream):
        if i >= limit:
            break

        text = line.rstrip('\r\n').encode('utf8')
        outstream.write(get_ammo(
            url_parsed,
            {
                'origin': 'mobile-yari-search-text',
                'lang': 'ru',
                'autoscale': '1',
                'key': '1',
                'format': 'json',
                'text': text,
                'll': '%f,%f' % (YANDEX_LOCATION['lon'], YANDEX_LOCATION['lat']),
                'results': '1',
                'spn': '0.00400,0.00400',
                'type': 'geo,biz'
            },
            tag=tag
        ))


def gen_from_dialog_logs(path, limit, start_row=None):
    yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')

    rows = yt.get_attribute(path, 'row_count')
    limit = min(limit, rows)
    start_row = start_row or randint(0, rows - limit)

    table = yt.read_table(
        path,
        response_parameters={
            'start_row_index': start_row,
            'approximate_row_count': limit,
        }
    )

    for row in table:
        yield {
            'uuid': gen_uuid_for_tests(row['uuid']),
            'utterance_text': row['utterance_text'],
            'callback_name': row['callback_name'],
            'callback_args': row['callback_args'],
            'utterance_source': row['utterance_source'],
            'location_lat': row.get('location_lat'),
            'location_lon': row.get('location_lon'),
            'device_state': row.get('device_state'),
        }


def get_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--ammo-type', '-t',
        choices=['navi', 'speechkit', 'geosearch'],
        help="Target ammo type.",
        default='speechkit',
    )
    parser.add_argument(
        '--tag', type=str, default=None, help='Requests could be grouped and marked by some tag'
    )
    parser.add_argument(
        '--url', help=('Service url to apply ammo to. E.g. '
                       '"https://vins-int.dev.voicetech.yandex.net/speechkit/app/pa/".'),
        default='https://vins-int.dev.voicetech.yandex.net/speechkit/app/pa/',
    )
    parser.add_argument(
        '--out', '-o', help='Filename with ammo results. Set to STDOUT by default.'
    )
    parser.add_argument(
        '--src', help=(
            'Filename with text queries on each line. Set to STDIN by default.'
            ' Could be yt.hahn path'
        ),
    )
    parser.add_argument(
        '--limit', help='Ammo count. Default unlimited', default=float('inf'), type=float,
    )
    parser.add_argument(
        '--start_row', help='Fixed start row for yt table', default=None, type=int,
    )
    parser.add_argument(
        '--experiments', metavar='EXP', nargs='+', required=False, help='experiment flags'
    )
    parser.add_argument(
        '--experiments_dict', type=str, default=None, help='experiment flags in json string'
    )
    parser.add_argument(
        '--app_id', type=str, default='com.yandex.vins.shooting', help='app id'
    )
    parser.add_argument(
        '--oauth_token', type=str, default=None, help='oauth token'
    )
    parser.add_argument(
        '--request_id_seed', type=int, default=None, help='Generate request_ids with this seed'
    )

    return parser


if __name__ == "__main__":
    parser = get_parser()
    args = parser.parse_args()

    outstream = codecs.open(args.out, 'w', encoding='utf8') if args.out else sys.stdout
    if not args.src:
        instream = sys.stdin
    elif args.src.startswith('//'):
        instream = gen_from_dialog_logs(args.src, args.limit, args.start_row)
    else:
        instream = codecs.open(args.src, encoding='utf8')

    url_parsed = urlparse.urlparse(args.url)
    if args.ammo_type == 'navi':
        navi_ammo(instream, outstream, url_parsed, args.tag, args.limit)
    elif args.ammo_type == 'geosearch':
        geosearch_ammo(instream, outstream, url_parsed, args.tag, args.limit)
    elif args.ammo_type == 'speechkit':
        if args.experiments_dict is not None:
            experiments = json.loads(args.experiments_dict)
        else:
            experiments = args.experiments
        speechkit_ammo(instream, outstream, url_parsed, args.tag, args.limit, experiments,
                       args.app_id, args.oauth_token, args.request_id_seed)
    else:
        raise NotImplementedError
