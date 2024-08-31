# coding: utf-8

import argparse
import logging
import requests

from multiprocessing import Queue, Process

logger = logging.getLogger(__name__)


_NUM_PROCS = 64

_FEATURE_RULES = [
    'FstAlbum', 'FstArtist', 'FstCurrency', 'FstFilms100_750', 'FstFilms50Filtered',
    'FstPoiCategoryRu', 'FstSite', 'FstSoft', 'FstSwear', 'FstTrack', 'FstCalc',
    'FstDate', 'FstDatetime', 'FstDatetimeRange', 'FstFio', 'FstFloat', 'FstGeo',
    'FstNum', 'FstTime', 'FstUnitsTime', 'FstWeekdays',
    'CustomEntities', 'AliceTokenEmbedder', 'AliceEmbeddings', 'AliceTypeParserTime',
    'AliceSampleFeatures', 'AliceRequest', 'AliceSession', 'AliceNormalizer'
]

_DEFAULT_RULES = [
    'AliceTagger', 'AliceBinaryIntentClassifier'
] + _FEATURE_RULES

_BEGEMOT_URL = 'http://hamzard.yandex.net:8891/wizard'


class BegemotRequestSession(object):
    def __init__(self, begemot_url=None, begemot_rules=None):
        self._session = None
        self._begemot_url = begemot_url or _BEGEMOT_URL
        self._begemot_rules = begemot_rules or _DEFAULT_RULES
        self._begemot_rules = ','.join(self._begemot_rules)

    def __enter__(self):
        self._session = requests.Session()
        self._session.params = {
            'format': 'json', 'wizclient': 'megamind',
            'rwr': self._begemot_rules, 'wizextra': 'alice_preprocessing=true'
        }
        return self

    def __exit__(self, type, value, traceback):
        self._session.close()

    def get(self, request_text, additional_params=None):
        if isinstance(request_text, unicode):
            request_text = request_text.encode('utf8')

        params = {'text': request_text}
        params.update(additional_params or {})

        response = self._session.get(self._begemot_url, params=params)
        try:
            if response.status_code == requests.codes.ok:
                return response.json()['rules']
        except:
            logger.warning('Something went wrong with request: "%s"', request_text)
            return


def _get_intent_probability(begemot_response, intent):
    probabilities = begemot_response.get('AliceBinaryIntentClassifier', {}).get('Probabilities', [])
    for probability in probabilities:
        if probability['key'] == intent:
            return probability['value']

    return None


def _generate_markup(tokens):
    def _format_tokens(tokens, tag):
        if tag != 'O':
            return "'{}'({})".format(' '.join(tokens), tag)
        return ' '.join(tokens)

    prev_tag, prev_tokens = None, []
    for token in tokens:
        tag = token['Tag'][2:] if token['Tag'] != 'O' else 'O'
        if prev_tag != tag:
            if prev_tokens:
                yield _format_tokens(prev_tokens, prev_tag)
            prev_tag, prev_tokens = tag, []

        prev_tokens.append(token['Text'].encode('utf8'))

    if prev_tokens:
        yield _format_tokens(prev_tokens, prev_tag)


def _get_tagger_markup(begemot_response, intent):
    tagger_predictions = begemot_response.get('AliceTagger', {}).get('Predictions', [])
    for prediction in tagger_predictions:
        if prediction['key'] != intent:
            continue

        tokens = prediction['value']['Prediction'][0]['Token']
        return ' '.join(_generate_markup(tokens))

    return None


def _process_sample(sample, begemot_session, intent):
    count, text = sample

    begemot_response = begemot_session.get(text)
    if not begemot_response:
        return None

    intent_probability = _get_intent_probability(begemot_response, intent)

    markup = _get_tagger_markup(begemot_response, intent)

    if markup and intent_probability:
        return count, markup, intent_probability

    return None


def _process_sample_worker(process_id, in_queue, out_queue, begemot_url, intent):
    counter = 0
    with BegemotRequestSession(begemot_url=begemot_url) as begemot_session:
        while True:
            sample = in_queue.get()
            if sample is None:
                return

            out_queue.put(_process_sample(sample, begemot_session, intent))

            counter += 1
            if process_id == 0 and counter > 0 and counter % 500 == 0:
                logger.info('[Process %s] Processed: %s samples', process_id, _NUM_PROCS * counter)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-path', required=True)
    parser.add_argument('--intent', required=True)
    parser.add_argument('--begemot-url', required=True)
    parser.add_argument('-o', '--output-path', required=True)

    args = parser.parse_args()

    input_samples = []
    with open(args.input_path) as f:
        f.readline()
        for line in f:
            input_samples.append(line.split('\t')[:2])

    in_queue, out_queue = Queue(), Queue()

    processes = [
        Process(target=_process_sample_worker, args=(process_id, in_queue, out_queue, args.begemot_url, args.intent))
        for process_id in xrange(_NUM_PROCS)
    ]

    for process in processes:
        process.daemon = True
        process.start()

    for sample in input_samples:
        in_queue.put(sample)

    for _ in xrange(_NUM_PROCS):
        in_queue.put(None)

    results = [out_queue.get() for _ in xrange(len(input_samples))]

    for process in processes:
        process.join()

    results = [sample for sample in results if sample]
    results = sorted(results, key=lambda x: x[2], reverse=True)

    with open(args.output_path, 'w') as f:
        for count, markup, probability in results:
            f.write('{}\t{}\t{}\n'.format(count, markup, probability))


if __name__ == "__main__":
    main()
