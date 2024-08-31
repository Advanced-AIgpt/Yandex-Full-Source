#!/usr/bin/env python
# encoding: utf-8

import os
import sys
import json
import logging
import click

from alice.analytics.operations.dialog.pulsar.pulsar_urls import (
    get_pulsar_link_compare,
    get_pulsar_link_for_instance,
    hit_pulsar_cache,
)


def _setup_logger():
    logging.basicConfig(
        format="[%(levelname)s] %(asctime)s - %(message)s",
        stream=sys.stdout,
        level=logging.INFO
    )


def _need_screenshot(basket_name):
    if not basket_name:
        return False
    return 'general' in basket_name or 'fairytale' in basket_name


@click.command()
@click.option('--metrics', required=True)
@click.option('--pulsar-prod', required=True)
@click.option('--out', required=True)
@click.option('--pulsar-token', required=True)
@click.option('--pulsar-test', required=False)
def main(metrics, pulsar_prod, out, pulsar_token, pulsar_test=None):
    _setup_logger()
    logging.info('add_pulsar_links start')
    with open(metrics) as fi:
        metrics_data = json.load(fi)

    with open(pulsar_prod) as fi:
        pulsar_prod_data = json.load(fi)
        instance_id_prod = pulsar_prod_data[0]['instance_id']

    instance_id_test = None
    if pulsar_test and os.path.exists(pulsar_test):
        with open(pulsar_test) as fi:
            pulsar_test_data = json.load(fi)
            instance_id_test = pulsar_test_data[0]['instance_id']

    logging.info('data loaded, {} metrics'.format(len(metrics_data)))
    additional_slice_metrics = ['metric_integral_on_scenario_changed', 'metric_integral_on_asr_changed']
    metric_names = [x for x in pulsar_prod_data[0]['columns_list'] if x.startswith('metric_')] + additional_slice_metrics
    for idx, item in enumerate(metrics_data):
        metric_name = 'metric_{}'.format(item.get('metric_name', ''))
        if metric_name not in metric_names:
            logging.info('{}) skip pulsar links for: {}, {}'.format(idx, item.get('basket'), metric_name))
            continue

        logging.info('{}) before get pulsar link for: {}, {}'.format(idx, item.get('basket'), metric_name))
        need_screenshot = _need_screenshot(item.get('basket'))

        for mode in ['short', 'eosp', 'full'] if instance_id_test else ['full']:
            if instance_id_test:
                pulsar_link = get_pulsar_link_compare(
                    instance_id_prod,
                    instance_id_test,
                    item.get('basket'),
                    metric_name,
                    metric_names,
                    pulsar_token,
                    need_screenshot,
                    mode=mode,
                )
            else:
                pulsar_link = get_pulsar_link_for_instance(
                    instance_id_prod,
                    metric_names,
                )
            if pulsar_link:
                logging.info('  got link "{}", objectTableState: {}, prod: {}, test: {}'.format(
                    mode,
                    pulsar_link.split('&objectTableState=')[-1] if instance_id_test else None,
                    instance_id_prod,
                    instance_id_test
                ))
                item['pulsar_link{}'.format('' if mode == 'full' else '_{}'.format(mode))] = pulsar_link

    logging.info('pulsar links done')

    if instance_id_test:
        hit_pulsar_cache(instance_id_prod, instance_id_test, metric_names, pulsar_token)
        logging.info('hit pulsar cache for metrics: {}'.format(len(metric_names)))

    with open(out, 'w') as fo:
        json.dump(metrics_data, fo, indent=4)

    logging.info('add_pulsar_links finish')
