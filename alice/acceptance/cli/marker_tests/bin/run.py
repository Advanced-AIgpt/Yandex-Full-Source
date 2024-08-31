# coding: utf-8

import os
import subprocess
import time

import click
import vh

from alice.acceptance.cli.marker_tests.bin import config as bin_config
from alice.acceptance.cli.marker_tests.lib import deep_graph


CONFIGS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__name__))), 'configs')
DEFAULT_CONFIG_PATH = os.path.join(CONFIGS_DIR, 'config_production.yaml')
DEFAULT_DATA_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__name__))), 'data')


@click.command()
@click.option('--data-dir', required=False,
              default=DEFAULT_DATA_DIR, help='Directory with json data for tests', show_default=True)
@click.option('--config-path', required=False,
              default=DEFAULT_CONFIG_PATH, help='Config file path', show_default=True)
@click.option('--guid', required=False, help='Nirvana workflow guid')
@click.option('--no-exec', default=False, is_flag=True, help='Do not execute graph after creation')
@click.option('--prod-run', default=False, is_flag=True, help='Rewrite production workflow')
@click.option('--wait', default=False, is_flag=True, help='Sync script execution with graph execution')
@click.option('--scraper-mode', type=click.Choice(['test', 'prod', 'soy_test']), default='prod', show_default=True)
@click.option('--use-linked-resources', default=False, is_flag=True, help='Use data linked to binary, overrides data-dir')
def main(data_dir, config_path, guid, no_exec, prod_run, wait, scraper_mode, use_linked_resources):
    config = bin_config.init_config(config_path)
    global_options = bin_config.global_options
    deep_graph.build_graph(data_dir, global_options, config, use_linked_resources, scraper_mode)

    start = not no_exec
    if prod_run:
        start = False
        wait = False
        guid = config.nirvana.hitman_guid

    timestamp = int(time.time())
    run_kwargs = dict(
        label='Marker tests run',
        workflow_guid=guid,
        workflow_ttl=1,
        quota=config.nirvana.quota,
        start=start,
        wait=wait,
        oauth_token=os.environ.get('NIRVANA_TOKEN') or None,
        yt_token_secret=config.yt.token,
        global_options={
            'yt-token': config.yt.token,
            'yt-proxy': config.yt.proxy,
            'yt-pool': config.yt.pool if config.yt.pool else None,
            'mr-account': config.yt.mr_account,
            'mr-output-ttl': config.yt.mr_output_ttl,
            'mr-output-path': config.yt.mr_output_path,

            'yql-token': config.yql.token,
            'arcanum-token': config.arcanum.token,

            'uniproxy-url': config.alice.uniproxy_url,
            'vins-url': config.alice.vins_url,
            'ya-plus-token': config.alice.oauth,

            'cache_sync': timestamp,
        },
    )
    if scraper_mode == 'no':
        run_kwargs['arcadia_root'] = str(subprocess.check_output('ya dump root'.split()), 'ascii')
    vh.run(**run_kwargs)


if __name__ == '__main__':
    main()
