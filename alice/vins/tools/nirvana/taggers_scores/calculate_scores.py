# coding: utf-8

import logging
import click
import json

import yt.logger as yt_logger
import yt.wrapper as yt

from collections import defaultdict
from operator import itemgetter
from datetime import datetime

from vins_tools.nlu.inspection.taggers_recall import calc_recalls_at, calc_recall_by_slots


yt_logger.LOGGER.setLevel(logging.DEBUG)
logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logger = logging.getLogger(__name__)


COLUMNS = [
    ('text', 'string'),
    ('request_id', 'string'),
    ('app', 'any'),
    ('device_state', 'any'),
    ('experiments', 'any'),
    ('slots', 'any'),
]
SCHEMA = [{'name': name, 'type': type_} for (name, type_) in COLUMNS]


def _parse_vins_response(response):
    request_id = response['header']['request_id']
    for meta in response['response']['meta']:
        if meta['type'] == 'debug_info' and meta['data'].get('type') == 'tagger_predictions':
            for stage in meta['data']['data']['scores']:
                if 'post_processed' in stage:
                    return request_id, stage['post_processed']
    return request_id, []


def _convert_slots(slot_list):
    slots = defaultdict(list)
    for slot in slot_list:
        slots[slot['value']].append({
            'start': slot['start'],
            'end': slot['end'],
            'is_continuation': slot['is_continuation'],
        })
    return slots


def _calc_scores(requests, recalls_at):
    output = {}

    true_slots_list = map(itemgetter('true_slots'), requests.values())
    pred_slots_list = map(itemgetter('predicted_slots'), requests.values())

    output['recall'] = calc_recalls_at(recalls_at, true_slots_list, pred_slots_list)

    recall_by_slots = calc_recall_by_slots(recalls_at, true_slots_list, pred_slots_list)

    output.update(recall_by_slots.to_dict())

    logger.info('Weighted slot recall = {:.1%}\n'.format(recall_by_slots.weighted_mean_recall))
    logger.info('Weighted slot precision = {:.1%}\n'.format(recall_by_slots.weighted_mean_precision))
    logger.info('Weighted slot F1-score = {:.1%}\n'.format(recall_by_slots.weighted_mean_f1_score))

    weighted_means = {
        'weighted_mean_recall': recall_by_slots.weighted_mean_recall,
        'weighted_mean_precision': recall_by_slots.weighted_mean_precision,
        'weighted_mean_f1_score': recall_by_slots.weighted_mean_f1_score,
    }

    return output, weighted_means


def _filter_requests(requests):
    result = {}
    for request_id, values in requests.iteritems():
        if 'true_slots' not in values:
            logger.error('True slots not found for request {}'.format(request_id))
            continue
        if 'predicted_slots' not in values:
            logger.error('Predicted slots not found for request {}'.format(request_id))
            continue
        result[request_id] = values
    return result


@click.command()
@click.option('--input_requests', required=True, help='Input table with requests', envvar='INPUT_REQUESTS')
@click.option('--input_scores', required=True, help='Input table with scores', envvar='INPUT_SCORES')
@click.option('--weighted_means_file', required=True, help='Weighted means output file path',
              envvar='WEIGHTED_MEANS_FILE')
@click.option('--scores_file', required=True, help='Weighted means output file path', envvar='SCORES_FILE')
@click.option('--intent', required=True, help='Intent name', envvar='INTENT')
@click.option('--recalls-at', help='recall@K levels, e.g. "1,2,5,10"', default='1,2,5,10', envvar='RECALLS_AT')
@click.option('--date', help='Date, optional"', default=None, envvar='DATE')
@click.option('--yt-proxy', default='hahn', help='YT cluster proxy', envvar='YT_PROXY')
@click.option('--yt-token', required=True, envvar='YT_TOKEN', help='Yt token')
def main(input_requests, input_scores, weighted_means_file, scores_file, intent, recalls_at, date, yt_proxy, yt_token):
    yt.config['proxy']['url'] = yt_proxy
    yt.config['token'] = yt_token

    recalls_at = map(int, recalls_at.split(','))

    requests = defaultdict(dict)
    for row in yt.read_table(input_requests):
        requests[row['request_id']]['true_slots'] = row['slots']

    for row in yt.read_table(input_scores):
        if 'response' not in row:
            logger.error('Response not found in row:\n{}'.format(row))
            continue
        request_id, slots = _parse_vins_response(json.loads(row['response']))
        slots = [_convert_slots(result['slots']) for result in slots]
        requests[request_id]['predicted_slots'] = slots

    requests = _filter_requests(requests)
    scores, weighted_means = _calc_scores(requests, recalls_at=recalls_at)

    weighted_means['intent'] = intent
    weighted_means['fielddate'] = date or datetime.now().strftime("%Y-%m-%d")
    weighted_means = [weighted_means]

    with open(weighted_means_file, 'wt') as fd:
        json.dump(weighted_means, fd, sort_keys=True, indent=4)

    with open(scores_file, 'wt') as fd:
        json.dump(scores, fd, sort_keys=True, indent=4)


if __name__ == '__main__':
    main()
