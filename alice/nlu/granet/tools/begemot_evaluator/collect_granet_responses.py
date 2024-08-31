# -*- coding: utf-8 -*-

import attr
import argparse
import requests
import yt.wrapper as yt

from tqdm import tqdm

from vins_core.common.sample import Sample
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor


_FST_RULES = [
    'FstAlbum', 'FstArtist', 'FstCurrency', 'FstFilms100_750', 'FstFilms50Filtered',
    'FstPoiCategoryRu', 'FstSite', 'FstSoft', 'FstSwear', 'FstTrack', 'FstCalc',
    'FstDate', 'FstDatetime', 'FstDatetimeRange', 'FstFio', 'FstFloat', 'FstGeo',
    'FstNum', 'FstTime', 'FstUnitsTime', 'FstWeekdays'
]

_NONSENSE_TAGGER_RULES = ['AliceNonsenseTagger', 'AliceEmbeddings']

_BEGEMOT_RULES = ['Granet', 'CustomEntities'] + _FST_RULES


@attr.s(frozen=True)
class GranetRequest(object):
    text = attr.ib()
    intent = attr.ib()
    count = attr.ib(converter=int)
    response = attr.ib(default=None)


def _prepare_data(table, row_count):
    normalizer = NormalizeSampleProcessor('normalizer_ru')

    yt.config['proxy']['url'] = 'hahn'
    data = []
    for i, row in enumerate(tqdm(yt.read_table(table), 'Preparing data', total=row_count)):
        text = normalizer._process(Sample.from_string(row['text'])).text.encode('utf-8')
        data.append(GranetRequest(text=text, intent=row['intent'], count=row['count']))
        if row_count is not None and i == row_count - 1:
            break
    return data


def _collect_results(data, begemot_url, use_nonsense_tagger):
    if use_nonsense_tagger:
        rules = ','.join(_BEGEMOT_RULES + _NONSENSE_TAGGER_RULES)
    else:
        rules = ','.join(_BEGEMOT_RULES)

    results = []
    with requests.Session() as session:
        url = 'http://{}/wizard'.format(begemot_url)
        session.params = {'format': 'json', 'wizclient': 'megamind', 'rwr': rules}
        for request in tqdm(data, 'Collecting results'):
            response = session.get(url, params={'text': request.text})
            if response.status_code != requests.codes.ok:
                print 'Error response for line \"{}\": {}'.format(request.text, response.status_code)
                continue
            results.append(attr.evolve(request, response=response.json()['rules']['Granet']))

    return results


def _dump_results(results, output_path):
    with open(output_path, 'w') as f:
        f.write('Text\tPredicted intents\tCorrect intent\tCount\n')
        for result in results:
            predicted_intents = ','.join(x['Name'] for x in result.response.get('Forms', [{'Name': ''}]))
            f.write('{text}\t{predicted_intents}\t{correct_intent}\t{count}\n'.format(
                text=result.text, predicted_intents=predicted_intents, correct_intent=result.intent, count=result.count
            ))


def main():
    argument_parser = argparse.ArgumentParser(add_help=True)
    argument_parser.add_argument('--begemot-url', required=True)
    argument_parser.add_argument('--output-path', required=True)
    argument_parser.add_argument('--row-count', type=int, default=None)
    argument_parser.add_argument('--use-nonsense-tagger', action='store_true', default=False)
    argument_parser.add_argument(
        '--data-path', default='//home/voice/dan-anastasev/quasar_toloka_requests',
        help='Path to yt table with the test data'
    )
    args = argument_parser.parse_args()

    data = _prepare_data(args.data_path, args.row_count)
    results = _collect_results(data, args.begemot_url, args.use_nonsense_tagger)
    _dump_results(results, args.output_path)
