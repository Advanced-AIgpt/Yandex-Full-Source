# coding: utf-8

import argparse
import logging
import requests
import yt.wrapper as yt

from multiprocessing import Queue, Process

logger = logging.getLogger(__name__)


_NUM_PROCS = 64

_BEGEMOT_URL = 'http://hamzard.yandex.net:8891/wizard'


class BegemotRequestSession(object):
    def __init__(self, begemot_url=None):
        self._session = None
        self._begemot_url = begemot_url or _BEGEMOT_URL
        self._begemot_rules = ['AliceBinaryIntentClassifier', 'AliceNormalizer']
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


def _setup_yt():
    yt.config['proxy']['url'] = 'hahn'
    yt.config['read_parallel']['enable'] = True
    yt.config['read_parallel']['max_thread_count'] = 32

    yt.config['write_parallel']['enable'] = True
    yt.config['write_parallel']['max_thread_count'] = 32


def _get_intent_probability(begemot_response, intent):
    probabilities = begemot_response.get('AliceBinaryIntentClassifier', {}).get('Probabilities', [])
    for probability in probabilities:
        if probability['key'] == intent:
            return probability['value']

    return None


def _process_sample(sample, begemot_session, intent, text_column, prob_column):
    begemot_response = begemot_session.get(sample[text_column])
    if not begemot_response:
        return

    intent_probability = _get_intent_probability(begemot_response, intent)
    if intent_probability:
        sample[prob_column] = intent_probability
        return sample


def _process_sample_worker(process_id, in_queue, out_queue, begemot_url, intent, text_column, prob_column):
    counter = 0
    with BegemotRequestSession(begemot_url=begemot_url) as begemot_session:
        while True:
            sample = in_queue.get()
            if sample is None:
                return

            out_queue.put(_process_sample(sample, begemot_session, intent, text_column, prob_column))

            counter += 1
            if process_id == 0 and counter > 0 and counter % 100 == 0:
                logger.info('[Process %s] Processed: %s samples', process_id, _NUM_PROCS * counter)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-table', required=True)
    parser.add_argument('-o', '--output-table', required=True)
    parser.add_argument('--text-column', required=True)
    parser.add_argument('--prob-column', required=True)
    parser.add_argument('--intent', required=True)
    parser.add_argument('--begemot-url', default=None)

    args = parser.parse_args()

    _setup_yt()

    input_samples = list(yt.read_table(args.input_table))

    in_queue, out_queue = Queue(), Queue()

    processes = [
        Process(target=_process_sample_worker,
                args=(process_id, in_queue, out_queue, args.begemot_url,
                      args.intent, args.text_column, args.prob_column))
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
    results = sorted(results, key=lambda x: x[args.prob_column], reverse=True)

    yt.write_table(args.output_table, results)


if __name__ == "__main__":
    main()
