#!/usr/bin/env python3
# coding: utf-8

import click
import logging
import os
import re
import sys
import yt.wrapper as yt


logger = logging.getLogger(__name__)

MM_LOG_DIR = '//logs/megamind-log/stream/5min'


def _create_yt_client(token=None):
    token = token or os.environ.get('YT_TOKEN', '')
    return yt.YtClient(proxy='hahn', token=token)


def _find_mm_log_tables(yt_client, table_name_suffix=''):
    names = [name for name in yt_client.list(MM_LOG_DIR) if name.endswith(table_name_suffix)]
    names = sorted(names, reverse=True)
    logger.info(f'{len(names)} tables of Megaming logs were found in {MM_LOG_DIR}')
    return [yt.ypath_join(MM_LOG_DIR, name) for name in names]


def _safe_decode_utf8(raw):
    if raw is None:
        return None
    try:
        return raw.decode('utf-8')
    except UnicodeDecodeError:
        return None


def _read_requests_from_mm_logs(count, table_name_suffix='', yt_token=None):
    requests = []
    regex = re.compile(r"AppHost Http proxy item '(mm_begemot_native_request|mm_polyglot_begemot_native_request)' path '(?P<request>.*)'")
    yt_client = _create_yt_client(yt_token)
    for table in _find_mm_log_tables(yt_client, table_name_suffix):
        logger.info(f'Processing {table}')
        portion_size = 0
        for row in yt_client.read_table(table, format=yt.YsonFormat(encoding=None)):
            message = _safe_decode_utf8(row[b'Message'])
            if message is None:
                continue
            m = regex.fullmatch(message)
            if m is None:
                continue
            requests.append(m['request'])
            portion_size += 1
            if count > 0 and len(requests) >= count:
                break
        logger.info(f'{portion_size} requests were collected from {table}')
        if count > 0 and len(requests) >= count:
            return requests
    logger.warn(f'Only {len(requests)} requests were found')
    return requests


def _make_ammo_from_requests(requests):
    ammo = []
    bad = []
    for request in requests:
        if request.startswith('?'):
            ammo.append('/wizard' + request)
        else:
            bad.append(request)
    if bad:
        logger.error(f'Can not recognize {len(bad)} requests. One of invalid requests: "{bad[0]}"')
    return ammo


def collect_ammo_from_mm_logs(count, table_name_suffix='', yt_token=None):
    requests = _read_requests_from_mm_logs(count, table_name_suffix, yt_token)
    return _make_ammo_from_requests(requests)


def _write_lines(lines, path):
    if path:
        with open(path, 'w') as f:
            for line in lines:
                f.write(line + '\n')
    else:
        for line in lines:
            sys.stdout.write(line + '\n')


@click.command()
@click.option('-o', '--output', required=False, default='', help='Output file path to save the ammo.')
@click.option('-n', '--count', required=True, type=int, help='Max number of requests.')
@click.option('--yt-token', required=False, default='', help='YT Token.')
@click.option('--table-name-suffix', required=False, default='', help='Filter log tables by name suffix.')
def main(output, count, yt_token, table_name_suffix):
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(levelname)s] %(message)s')
    lines = collect_ammo_from_mm_logs(count, table_name_suffix, yt_token)
    _write_lines(lines, output)
    logger.info('%d requests were saved to %s' % (len(lines), output or 'output'))


if __name__ == '__main__':
    main()
