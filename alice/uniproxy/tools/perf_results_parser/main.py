import argparse
import json
import sys
import voicetech.common.lib.utils as utils
from prettytable import PrettyTable
from collections import defaultdict

logger = utils.initialize_logging(__name__)

QUANTILES = [0.25, 0.50, 0.75, 0.9, 0.95, 0.99]


def get_quantiles(items, qs=QUANTILES):
    sorted_items = sorted(items)
    l = len(sorted_items)
    return [round(sorted_items[int(l * q)], 3) for q in qs]


def parse_metrics(entries):
    metrics = defaultdict(list)
    for entry in entries:
        directive = entry.get('directive', {})
        header = directive.get('header', {})
        namespace = header.get('namespace', '')
        name = header.get('name', '')
        if namespace == 'Vins' and name == 'UniproxyVinsTimings':
            payload = directive['payload'].copy()
            evts = ['vins_response_sec', 'end_of_utterance_sec', 'end_of_speech_sec',
                    'last_vins_full_request_duration_sec']
            if any(payload.get(e) is None for e in evts):
                continue
            eos_sec = payload.get('end_of_speech_sec')
            eou_sec = payload.get('end_of_utterance_sec')
            vins_response_sec = payload.get('vins_response_sec')
            first_tts_chunk_sec = payload.get('first_tts_chunk_sec')
            vins_response_or_tts_chunk = first_tts_chunk_sec if first_tts_chunk_sec is not None else vins_response_sec
            metrics['vins_response_minus_eos'].append(vins_response_or_tts_chunk - eos_sec)
            metrics['eou_minus_eos'].append(eou_sec - eos_sec)
            metrics['vins_response_minus_eou'].append(vins_response_sec - eou_sec)
            metrics['vins_requests_count'].append(payload['vins_request_count'])
    return metrics


def go():
    entries = json.load(sys.stdin)
    metrics = parse_metrics(entries)
    t = PrettyTable(field_names=['Metric'] + ['q' + str(int(q * 100)) for q in QUANTILES])
    t.add_row(['vins_response - eos'] + get_quantiles(metrics['vins_response_minus_eos'], QUANTILES))
    t.add_row(['vins_response - eou'] + get_quantiles(metrics['vins_response_minus_eou'], QUANTILES))
    t.add_row(['eou - eos'] + get_quantiles(metrics['eou_minus_eos'], QUANTILES))
    print(t)


def main():
    go()
