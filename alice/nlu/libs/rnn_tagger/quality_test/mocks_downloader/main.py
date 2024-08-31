import argparse
import base64
import json
import logging
import re
import requests

from multiprocessing import Queue, Process

logger = logging.getLogger(__name__)


_NUM_PROCS = 32

_DEFAULT_RULES = [
    "AliceActionRecognizer",
    "AliceAnaphoraMatcher",
    "AliceAnaphoraSubstitutor",
    "AliceArFst",
    "AliceCustomEntities",
    "AliceEmbeddings",
    "AliceEmbeddingsExport",
    "AliceEntitiesCollector",
    "AliceEntityRecognizer",
    "AliceExternalSampleFeatures",
    "AliceFixlist",
    "AliceFrameFiller",
    "AliceGcDssmClassifier",
    "AliceIot",
    "AliceIotConfigParser",
    "AliceIotHelper",
    "AliceItemSelector",
    "AliceMicrointents",
    "AliceMultiIntentClassifier",
    "AliceNonsenseTagger",
    "AliceNormalizer",
    "AliceRequest",
    "AliceSampleFeatures",
    "AliceScenariosWordLstm",
    "AliceSession",
    "AliceTagger",
    "AliceThesaurus",
    "AliceTranslit",
    "AliceTokenEmbedder",
    "AliceTolokaWordLstm",
    "AliceTrivialTagger",
    "AliceTypeParserTime",
    "AliceUserEntities",
    "AliceVersions",
    "AliceWizDetection",
    "CustomEntities",
    "Date",
    "DirtyLang",
    "EntityFinder",
    "ExternalMarkup",
    "Fio",
    "FstAlbum",
    "FstArtist",
    "FstCalc",
    "FstCurrency",
    "FstDate",
    "FstDatetime",
    "FstDatetimeRange",
    "FstFilms100_750",
    "FstFilms50Filtered",
    "FstFio",
    "FstFloat",
    "FstGeo",
    "FstNum",
    "FstPoiCategoryRu",
    "FstSite",
    "FstSoft",
    "FstSwear",
    "FstTime",
    "FstTrack",
    "FstUnitsTime",
    "FstWeekdays",
    "GeoAddr",
    "Granet",
    "GranetCompiler",
    "GranetConfig",
    "IsNav",
    "PornQuery",
    "Wares"
]

_NORMALIZE_RULES = ['AliceNormalizer', 'ExternalMarkup']

_BEGEMOT_URL = 'http://hamzard.yandex.net:8891/wizard'

_TAGGED_TEXT_PATTERN = re.compile(r"'([^)]+)'\((\+?[a-z_]+)\)")


class BegemotRequestSession:
    def __init__(self, begemot_url=None, begemot_rules=None):
        self._session = None
        self._begemot_url = begemot_url or _BEGEMOT_URL
        self._begemot_rules = ','.join(begemot_rules or _DEFAULT_RULES)

    def __enter__(self):
        self._session = requests.Session()
        self._session.params = {
            'format': 'json',
            'wizclient': 'megamind',
            'rwr': self._begemot_rules,
        }
        return self

    def __exit__(self, type, value, traceback):
        self._session.close()

    def get(self, request_text, nlu_extra='', additional_params=None):
        params = {'text': request_text}
        params.update(additional_params or {})
        params.update({
            'wizextra': 'alice_preprocessing=true;bg_nlu_extra={}'.format(base64.b64encode(nlu_extra.encode('utf8')).decode()),
        })

        response = self._session.get(self._begemot_url, params=params)
        try:
            if response.status_code == requests.codes.ok:
                return response.json()
        except Exception:
            logger.warning('Something went wrong with the request: "%s"', request_text)
            return


def _parse_markup(markup):
    prev_pos = 0
    for match in _TAGGED_TEXT_PATTERN.finditer(markup):
        match_start = match.start()
        yield markup[prev_pos: match_start], 'O'
        yield match.group(1), match.group(2)
        prev_pos = match.end()

    yield markup[prev_pos:], 'O'


def _preprocess_markup(markup, begemot_url):
    tokens, tags = [], []

    normalized_markup = ''
    with BegemotRequestSession(begemot_url=begemot_url, begemot_rules=_NORMALIZE_RULES) as begemot_session:
        for substring, slot in _parse_markup(markup):
            substring = substring.strip()
            if not substring:
                continue

            begemot_normalize_response = begemot_session.get(substring)
            substring_tokens = [token['Text'] for token in begemot_normalize_response['markup']['Tokens']]
            if slot == 'O':
                normalized_markup += ' '.join(substring_tokens)
            else:
                normalized_markup += "'{}'({})".format(' '.join(substring_tokens), slot)
            normalized_markup += ' '
            tokens.extend(substring_tokens)

            if slot == 'O':
                tags.extend(['O'] * len(substring_tokens))
            elif slot.startswith('+'):
                tags.extend(['I-' + slot[1:]] * len(substring_tokens))
            else:
                substring_tags = ['I-' + slot] * len(substring_tokens)
                substring_tags[0] = 'B' + substring_tags[0][1:]
                tags.extend(substring_tags)

    assert len(tokens) == len(tags)

    return {
        'tokens': tokens,
        'tags': tags,
        'markup': normalized_markup.strip(),
    }


def _process_sample(sample, nlu_extra, begemot_session):
    begemot_output = begemot_session.get(' '.join(sample['tokens']), nlu_extra=nlu_extra)
    if not begemot_output:
        logger.info('No begemot response for %s', sample)
        return

    features = begemot_output['rules']['AliceExternalSampleFeatures']['sample_features']

    if features['sample']['tokens'] != sample['tokens']:
        logger.info('Wrong token alignment: %s != %s',
                    ' '.join(features['sample']['tokens']),
                    ' '.join(sample['tokens']))
        return

    sparse_seq_features = {}
    for feature in features['sparse_seq_features']:
        sparse_seq_features[feature['key']] = [element.get('data', []) for element in feature['value']['data']]

    features['sparse_seq_features'] = sparse_seq_features

    del features['dense_seq_features']
    del features['sample']['utterance']

    features['sample']['tags'] = sample['tags']
    features['sample']['markup'] = sample['markup']

    return features


def _process_sample_worker(begemot_url, process_id, in_queue, out_queue):
    counter = 0
    with BegemotRequestSession(begemot_url=begemot_url) as begemot_session:
        while True:
            sample = in_queue.get()
            if sample is None:
                return

            markup, nlu_extra, intent = sample

            sample = _preprocess_markup(markup, begemot_url)
            mock = _process_sample(sample, nlu_extra, begemot_session)

            if mock:
                out_queue.put((markup, intent, mock))
            else:
                out_queue.put(None)

            counter += 1
            if process_id == 0 and counter > 0 and counter % 100 == 0:
                logger.info('Processed: ~%s samples', _NUM_PROCS * counter)


def _read_data(intent, input_path):
    input_samples = []
    with open(input_path) as f:
        fields = f.readline().strip().split('\t')
        assert fields == ['markup', 'nlu_extra', 'intent'] or fields == ['markup', 'nlu_extra']

        has_intent_column = 'intent' in fields
        for line in f:
            line = line.rstrip()
            if has_intent_column:
                markup, nlu_extra, intent = line.split('\t')
            else:
                markup, nlu_extra = line.split('\t')

            input_samples.append((markup, nlu_extra, intent))

    return input_samples


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-path', required=True)
    parser.add_argument('-o', '--output-path', required=True)
    parser.add_argument('--intent', default=None)
    parser.add_argument('--begemot-url', default='http://hamzard.yandex.net:8891/wizard')

    args = parser.parse_args()

    in_queue, out_queue = Queue(), Queue()

    processes = [
        Process(target=_process_sample_worker, args=(args.begemot_url, process_id, in_queue, out_queue))
        for process_id in range(_NUM_PROCS)
    ]
    for process in processes:
        process.daemon = True
        process.start()

    input_samples = _read_data(args.intent, args.input_path)

    for sample in input_samples:
        in_queue.put(sample)
    for _ in range(_NUM_PROCS):
        in_queue.put(None)

    results = [out_queue.get() for _ in range(len(input_samples))]
    results = [sample for sample in results if sample]

    for process in processes:
        process.join()

    has_intent = all(intent for _, intent, _ in results)
    with open(args.output_path, 'w') as f:
        if has_intent:
            f.write('markup\tintent\tmock\n')
        else:
            f.write('markup\tmock\n')

        for markup, intent, mock in results:
            f.write(markup + '\t')
            if has_intent:
                f.write(intent + '\t')
            f.write(json.dumps(mock, ensure_ascii=False,) + '\n')


if __name__ == "__main__":
    main()
