import argparse
import asyncio
import logging
import os
from pathlib import Path
from urllib.parse import urlparse, urlencode, urlunparse, unquote

import alice.apphost.graph_generator.scenario as scenario_graph
import alice.hollywood.hw.template as template
import alice.library.python.utils as utils
import coloredlogs
from alice.library.python import server_runner
from alice.library.python.utils.arcadia import arcadia_path

from .service import AppHost, Megamind, Hollywood


def create(args):
    config_path = arcadia_path('alice/megamind/configs')
    for config_type in ['dev', 'hamster', 'production', 'rc']:
        filename, config = template.render_scenario_config(
            scenario_name=args.name,
            config_type=config_type,
            abc_services=args.abc,
        )
        path = config_path / config_type / 'scenarios' / filename
        if args.force or not path.exists():
            utils.write_file(path, config)
    scenario_graph.generate(
        config_path,
        'apphost/conf/verticals/ALICE',
        force_if=lambda _: _ == utils.to_snake_case(args.name),
    )

    paths = []
    hw_scenario_path = Path('alice/hollywood/library/scenarios')
    for filename, data in template.render_hollywood_scenario(args.name):
        utils.write_file(hw_scenario_path / filename, data)
        paths.append(filename)

    scenario_name = os.path.commonpath(paths)
    path = hw_scenario_path / 'ya.make'
    utils.write_file(path, template.yamake.render_for_test(path, scenario_name))

    path = 'alice/library/analytics/common/product_scenarios.h'
    utils.write_file(path, template.product_scenarios.render(path, scenario_name))

    scenario_path = hw_scenario_path / scenario_name
    for shard in args.shard:
        shard_path = Path('alice/hollywood/shards', shard)
        path = shard_path / 'scenarios' / 'ya.make'
        utils.write_file(path, template.yamake.render(path, scenario_path))
        for shard_type in ['dev', 'prod']:
            path = arcadia_path(shard_path, shard_type, 'hollywood.pb.txt')
            if path.exists():
                utils.write_file(path, template.shards.render(path, scenario_name))


@server_runner.async_loop
async def run_services(args):
    tasks = []

    apphost = AppHost(port=39999)
    mm = Megamind()
    hw = Hollywood(args.shard, args.scenario)

    url_parts = list(urlparse(apphost.url))
    srcrwrs = [('srcrwr', _.srcrwr) for _ in [mm, hw]]
    url_parts[4] = urlencode(srcrwrs)
    logging.info(f'MEGAMIND URL: {unquote(urlunparse(url_parts))}')

    tasks.append(asyncio.create_task(apphost.run()))
    tasks.append(asyncio.create_task(mm.run()))
    tasks.append(asyncio.create_task(hw.run()))

    return tasks


def run(args):
    asyncio.run(run_services(args))


def parse_args(args=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(required=True, title='subcommands')

    parser_create = subparsers.add_parser(
        'create',
        aliases=['cr'],
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='create new HW scenario',
    )
    parser_create.add_argument('-n', '--name', required=True, help='Scenario name')
    parser_create.add_argument('--abc', required=True, action='append', help='Abc services')
    parser_create.add_argument('--shard', default=['all', 'common'], action='append', help='Hollywood shards')
    parser_create.add_argument('-f', '--force', action='store_true', help='force to rebuild megamind configs')
    parser_create.set_defaults(func=create)

    parser_run = subparsers.add_parser(
        'run',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        help='run Apphost, MM, HW with local diff',
    )
    parser_run.add_argument('shard', default='all', nargs='?', help='Hollywood shard')
    parser_run.add_argument('-s', '--scenario', default=[], action='append', help='Hollywood shards')
    parser_run.set_defaults(func=run)

    parser.epilog = f"""subcommands usage:
    {parser_create.format_usage()}    {parser_run.format_usage()}"""

    return parser.parse_args(args)


def main(args):
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(logging.INFO)
    args.func(args)


if __name__ == '__main__':
    main(parse_args())
