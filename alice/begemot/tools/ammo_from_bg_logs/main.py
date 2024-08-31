#!/usr/bin/env python3
# coding: utf-8

import base64
import click
import logging
import os
import sys
import yt.wrapper as yt


logger = logging.getLogger(__name__)

BG_LOG_DIR = '//home/begemot/eventlogdata/begemot_megamind_yp_production_sas-cgi-requests-log'


def _create_yt_client(token=None):
    token = token or os.environ.get('YT_TOKEN', '')
    return yt.YtClient(proxy='hahn', token=token)


def _find_bg_log_tables(yt_client):
    names = sorted(yt_client.list(BG_LOG_DIR), reverse=True)
    logger.info(f'{len(names)} tables of Megaming logs were found in {BG_LOG_DIR}')
    return [yt.ypath_join(BG_LOG_DIR, name) for name in names]


def collect_ammo_from_bg_logs(count, yt_token=None):
    ammo = []
    yt_client = _create_yt_client(yt_token)
    for table in _find_bg_log_tables(yt_client):
        logger.info(f'Processing {table}')
        req_counter = 0
        for row in yt_client.read_table(yt.TablePath(table)):
            if row['event_type'] != 'TRequestReceived':
                continue
            ammo.append('/wizard?' + base64.b64decode(row['event_data']).decode('utf-8'))
            req_counter += 1
            if count > 0 and len(ammo) >= count:
                break
        logger.info(f'{req_counter} requests were collected from {table}')
        if count > 0 and len(ammo) >= count:
            return ammo
    logger.warn(f'Only {len(ammo)} requests were found')
    return ammo


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
def main(output, count, yt_token):
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(levelname)s] %(message)s')
    lines = collect_ammo_from_bg_logs(count, yt_token)
    _write_lines(lines, output)
    logger.info('%d requests were saved to %s' % (len(lines), output or 'output'))


if __name__ == '__main__':
    main()
