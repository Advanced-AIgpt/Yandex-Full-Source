import os
import sys
import json
import click
import logging

from alice.analytics.utils.json_utils import json_dumps
from alice.analytics.operations.priemka.metrics_calculator.calc_metrics_local import calc_metrics


def _setup_logger():
    logging.basicConfig(
        format="[%(levelname)s] %(asctime)s - %(message)s",
        stream=sys.stdout,
        level=logging.DEBUG
    )


@click.command()
@click.option('--prod_input_data', required=True)
@click.option('--test_input_data')
@click.option('--out', required=True)
@click.option('--metrics_groups')
@click.option('--metrics_params_json')
def main(prod_input_data, test_input_data, out, metrics_groups=None, metrics_params_json=None):
    _setup_logger()
    logging.info('start alice_metrics_calculator')

    metrics_params = {}
    if metrics_params_json is not None:
        with open(metrics_params_json) as f:
            metrics_params = json.load(f)
        logging.info('metrics_params: {}'.format(metrics_params))

    with open(prod_input_data) as f:
        prod_raw_items_list = json.load(f)

    test_raw_items_list = []
    if test_input_data and os.path.exists(test_input_data):
        with open(test_input_data) as f:
            test_raw_items_list = json.load(f)

    metrics_groups_list = None
    if metrics_groups and isinstance(metrics_groups, str) and len(metrics_groups):
        metrics_groups_list = [x.strip() for x in metrics_groups.split(',')]
    logging.info('read data finish')

    logging.info('start calculate metrics')
    result = calc_metrics(prod_raw_items_list, test_raw_items_list, metrics_groups_list, metrics_params)
    logging.info('finish calculate metrics')

    with open(out, 'w') as f:
        f.write(json_dumps(result))

    logging.info('data saved')
