import os
import time

import click

import vh

from alice.acceptance.runners.check_analytics_info import run as check_analytics_info
from alice.acceptance.runners.config import init_config


ACC_MODULES = {
    'check_analytics_info': check_analytics_info,
}


def run_in_config_order(config):
    prev_after_arr = []
    for group in config.acceptance_order:
        after_arr = []
        for module_key in group:
            if module_key and module_key in ACC_MODULES:
                after_arr.append(
                    ACC_MODULES[module_key].build_graph(config, prev_after_arr)
                )
        prev_after_arr = after_arr


@click.command()
@click.option(
    '--user-config', default='~/.alice_acceptance/config.yaml',
    help='User config filepath',
)
@click.option('--no-exec', is_flag=True, default=False)
def main(user_config, no_exec):
    config = init_config(user_config)
    run_in_config_order(config)

    ts = int(time.time())
    vh.run(
        start=not no_exec,
        global_options={
            'yql_token': config.yql.token,
            'user_oauth': config.alice_common.ya_plus_robot_token,
            'cache_sync': ts,
            'soy_token': config.soy.token,
        },
        yt_proxy=config.yt.proxy,
        yt_token_secret=config.yt.token,
        mr_account=config.yt.mr_account,
        quota=config.nirvana.quota,
        project=config.nirvana.project,
        label=config.nirvana.label,
        oauth_token=os.environ.get('NIRVANA_TOKEN') or None,
    )


if __name__ == '__main__':
    main()
