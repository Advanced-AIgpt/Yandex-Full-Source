# coding: utf-8
"""
Measures classification quality w/o and w/ bass change form
"""


import attr
import click
import json
import pandas as pd
import numpy as np
import requests
import logging

from operator import attrgetter
from sklearn.metrics import precision_recall_fscore_support, accuracy_score
from requests.adapters import HTTPAdapter

from vins_core.utils.misc import parallel
from vins_core.ext.base import CertificationAuthority
from vins_core.utils.intent_renamer import IntentRenamer
from personal_assistant.api.personal_assistant import PersonalAssistantAPI

logger = logging.getLogger(__name__)


@attr.s
class ProcessItemResult(object):
    text = attr.ib()
    intent = attr.ib()
    form_chooser_intent = attr.ib()
    final_intent = attr.ib()


def _get_test_user():
    pa_api = PersonalAssistantAPI()
    return pa_api.get_test_user(['oauth', 'ya_music'])


def _make_vins_request(text, device_state, prev_intent, test_user, vins_url, experiments):
    # TODO: set prev_intent

    vins_request = {
        'header': {
            'request_id': '6ea4b1c4-5f62-49ea-bffe-0bee417e511d',
        },
        'application': {
            'uuid': test_user.uuid,
            'platform': 'android',
            'app_id': 'ru.yandex.quasar.vins_test',
            'os_version': '6.0.1',
            'lang': 'ru-RU',
            'timestamp': '1542374299',
            'timezone': 'Europe/Moscow',
            'client_time': '20181116T161819',
            'app_version': '1.2.3',
            'device_id': '828bba7cdef4f4a747e02f89632e9dce'
        },
        'request': {
            'reset_session': True,
            'device_state': json.loads(device_state),
            'additional_options': {
                'oauth_token': test_user.token,
            },
            'experiments': experiments,
            'event': {
                'type': 'text_input',
                'text': text
            }
        }
    }

    session = requests.Session()
    if CertificationAuthority.cert_exist():
        session.verify = CertificationAuthority.get_cert()
    session.mount('http://', HTTPAdapter(max_retries=3))
    session.mount('https://', HTTPAdapter(max_retries=3))

    response = session.request('POST', url=vins_url, json=vins_request, timeout=30.)
    return response.json()


def _parse_vins_response(response):
    form_chooser_intent = ''
    bass_update_intent = ''
    for meta in response['response']['meta']:
        if meta['type'] == 'debug_info' and meta['data'].get('type') == 'intent_predictions':
            for stage in meta['data']['data']:
                if 'form_chooser' in stage:
                    form_chooser_intent = stage['form_chooser'][0]['name']
                elif 'final_intent' in stage:
                    bass_update_intent = stage['final_intent'][0]['name']

    final_intent = form_chooser_intent
    if bass_update_intent:
        final_intent = bass_update_intent

    return form_chooser_intent, bass_update_intent, final_intent


def _process_item(item, test_user, vins_url, experiments, intent_renamer, **kwargs):
    index, item = item

    if index != 0 and index % 100 == 0:
        logger.info('Processing %s item', index)

    device_state = item['device_state']
    intent = item['intent']
    prev_intent = item['prev_intent']
    text = item['text']

    response = _make_vins_request(text, device_state, prev_intent, test_user, vins_url, experiments)
    form_chooser_intent, bass_update_intent, final_intent = _parse_vins_response(response)
    intent = intent_renamer(intent, IntentRenamer.By.TRUE_INTENT)
    form_chooser_intent = intent_renamer(form_chooser_intent, IntentRenamer.By.PRED_INTENT)
    final_intent = intent_renamer(final_intent, IntentRenamer.By.PRED_INTENT)

    return ProcessItemResult(
        text=text, intent=intent, form_chooser_intent=form_chooser_intent, final_intent=final_intent
    )


def _collect_labels(results):
    return sorted(
        set(result.intent for result in results)
        | set(result.form_chooser_intent for result in results)
        | set(result.final_intent for result in results)
    )


def _evaluate(results, labels, predicted_label_attr):
    true_labels = [result.intent for result in results]
    predicted_labels = [predicted_label_attr(result) for result in results]

    precisions, recalls, f1_scores, supports = precision_recall_fscore_support(
        true_labels, predicted_labels, labels=labels, average=None
    )
    total_precision, total_recall, total_f1_score, _ = precision_recall_fscore_support(
        true_labels, predicted_labels, labels=labels, average='weighted'
    )

    intent_wise_results = [precisions, recalls, f1_scores]
    total_results = [total_precision, total_recall, total_f1_score]

    accuracy = accuracy_score(true_labels, predicted_labels)
    return intent_wise_results, total_results, supports, accuracy


def _build_evaluation_report(results):
    labels = _collect_labels(results)

    intent_wise_results, total_results, supports, vins_accuracy = _evaluate(
        results, labels, predicted_label_attr=attrgetter('form_chooser_intent')
    )

    final_intent_wise_results, final_total_results, _, final_accuracy = _evaluate(
        results, labels, predicted_label_attr=attrgetter('final_intent')
    )

    intent_wise_results.extend(final_intent_wise_results + [supports])
    intent_wise_results = [np.expand_dims(result, 1) for result in intent_wise_results]
    intent_wise_results = np.concatenate(intent_wise_results, -1)

    total_results.extend(final_total_results + [supports.sum()])
    total_results = np.array([total_results])

    report_data = np.concatenate((intent_wise_results, total_results), 0)

    report = pd.DataFrame(
        data=report_data,
        index=list(labels) + ['total'],
        columns=['precision', 'recall', 'f1-score', 'precision', 'recall', 'f1-score', 'support']
    )

    return report, vins_accuracy, final_accuracy


_DEFAULT_RENAMING_DIR = 'apps/personal_assistant/personal_assistant/tests/validation_sets/'
_DEFAULT_RENAMING_PATHS = ','.join([
    _DEFAULT_RENAMING_DIR + 'toloka_intent_renames_quasar.json',
    _DEFAULT_RENAMING_DIR + 'toloka_intent_renames.json'
])

_REQUIRED_EXPERIMENTS = [
    'video_play',
    'how_much',
    'avia',
    'tv',
    'tv_stream',
    'quasar_tv',
    'translate',
    'radio_play_in_quasar'
]


@click.command()
@click.option('--vins-url', help='Vins api url')
@click.option('--input-path', type=click.Path(exists=True), help='Path to the test file in tsv format')
@click.option('--experiments', default='catboost_post_classifier,force_intents,bass_setup_features',
              help='Comma separated list of experiments')
@click.option('--renaming-paths', default=_DEFAULT_RENAMING_PATHS,
              help='Comma separated list to renaming configuration paths')
def main(vins_url, input_path, experiments, renaming_paths):
    logging.basicConfig(format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s', level=logging.INFO)

    data = pd.read_csv(input_path, delimiter='\t')

    test_user = _get_test_user()

    experiments = experiments.split(',')
    experiments.append('debug_classification_scores')
    experiments.extend(_REQUIRED_EXPERIMENTS)

    intent_renamer = IntentRenamer(renaming_paths.split(','))

    items = list(data.iterrows())

    results = parallel(
        _process_item,
        items,
        function_kwargs={
            'test_user': test_user,
            'vins_url': vins_url,
            'experiments': experiments,
            'intent_renamer': intent_renamer
        },
        raise_on_error=False,
        filter_errors=True
    )

    report, vins_accuracy, final_accuracy = _build_evaluation_report(results)

    pd.set_option('display.width', 1000)
    pd.set_option('display.max_rows', 1000)
    pd.set_option('display.max_colwidth', 100)
    pd.set_option('precision', 4)

    print 'Report:\n{}\n'.format(report)
    print 'Accuracy: {:.2%} -> {:.2%}'.format(vins_accuracy, final_accuracy)


if __name__ == '__main__':
    main()
