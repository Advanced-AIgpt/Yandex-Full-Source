import json
import os

import click

from alice.acceptance.modules.check_analytics_info.lib.checkers import (
    MegamindAnalyticsInfoChecker,
    MetaChecker,
    TunnellerChecker
)
from alice.acceptance.modules.check_analytics_info.lib.utils import collect_dump_data, generate_file_name

CHECKERS = [MegamindAnalyticsInfoChecker, MetaChecker, TunnellerChecker]


@click.command()
@click.option('--data-dir', type=click.Path(exists=True))
def main(data_dir):
    for test_dir in os.listdir(data_dir):
        cur_dir = os.path.join(data_dir, test_dir)
        with open(os.path.join(cur_dir, 'input_table.json'), 'r') as table_file:
            input_table = json.load(table_file)
        for checker_cls in CHECKERS:
            checker = checker_cls()
            canonized_data = collect_dump_data(checker, input_table)
            with open(os.path.join(cur_dir, '{}.json'.format(generate_file_name(checker))),
                      'w') as canonized_data_file:
                json.dump(canonized_data, canonized_data_file, indent=4, sort_keys=True)


if __name__ == '__main__':
    main()
