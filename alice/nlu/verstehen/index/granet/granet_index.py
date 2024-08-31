import logging
from time import sleep

from ..index import Index
from granet import Sample, Grammar, TsvSampleDataset, SourceTextCollection, parse_samples, parse_all_samples, parse_to_almost_matched_parts
from verstehen.granet_errors import WrongGrammar
from verstehen.index.index_registry import registered_index
from verstehen.util import get_weak_grammar, markup_into_slots, fetch_synonyms, get_grammar_synonyms, update_nonterminal


logger = logging.getLogger(__name__)


@registered_index
class GranetIndex(Index):

    DEFAULT_CONFIG = {
        'index_type': 'granet'
    }

    def __init__(self, dataset, scores, grammars_dir, samples=None, synonyms_server=None, is_paskills=False):
        self.scores = scores
        self.sources = SourceTextCollection()
        self.synonyms_server = synonyms_server
        self.is_paskills = is_paskills
        if grammars_dir is not None:
            self.import_standard_grammars(grammars_dir)

        if dataset is not None:
            self.dataset = dataset
            self.samples = list(dataset)
        else:
            self.samples = samples

    def import_standard_grammars(self, path):
        self.sources.add_all_from_path(path.encode('utf8'))

    def preprocessing(self, query):
        opts = {
            'almostMatched': query.get(u'almostMatched', False),
            'grammarAsExperiment': query.get(u'grammarAsExperiment', False),
            'ids': query.get(u'appIds', None),
            'target': query.get(u'target', None),
            'needValues': query.get(u'printSlotsValues', False),
            'needTypes': query.get(u'printSlotsTypes', False),
            'userSamples': query.get(u'userSamples', []),
            'synonyms': query.get(u'synonyms', None),
            'updateGrammar': query.get(u'updateGrammar', None)
        }

        grammar = query[u'positive']
        grammar = grammar.replace('\t', '    ').encode('utf8')
        if opts['almostMatched']:
            grammar = get_weak_grammar(grammar)

        self.sources.update_main_text(grammar)
        try:
            grammar = Grammar(self.sources, is_paskills=self.is_paskills)
            return grammar, opts
        except RuntimeError as e:
            logger.error(e)
            raise WrongGrammar(bytes(e))

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        grammar, opts = preprocessed_query

        if len(opts['userSamples']) > 0:
            logger.debug('Parsing user samples')
            texts = opts['userSamples']
            ids = opts['ids']
            target = opts['target']
            if target is None:
                target = ['0'] * len(opts['userSamples'])

            # divide samples with ids and not
            samples_with_mocks = {'samples': [], 'mocks': [], 'target': []}
            samples_without_mocks = {'samples': [], 'mocks': [], 'target': []}
            for idx, text, t in zip(ids, texts, target):
                if idx is None:
                    sample = Sample.from_text(text.encode('utf8'))
                    mock = sample.get_mock()
                    samples_without_mocks['samples'].append(sample)
                    samples_without_mocks['mocks'].append(mock)
                    samples_without_mocks['target'].append(t)
                else:
                    mock = self.samples[idx].get_mock()
                    samples_with_mocks['samples'].append(self.samples[idx])
                    samples_with_mocks['mocks'].append(mock)
                    samples_with_mocks['target'].append(t)

            responses = []
            # parse samples from texts
            for i, s in enumerate(samples_without_mocks['samples']):
                result = grammar.parse(s)
                response = {
                    'text': s.get_text(),
                    'is_positive': result.is_positive(0),
                    'debug_info': result.dump_to_str(0),
                    'mock': samples_without_mocks['mocks'][i],
                    'markup': result.get_best_variant_as_str(0),
                    'target':  samples_without_mocks['target'][i]
                }
                responses.append((None, None, response))
            # parse samples with ids
            parse_results = parse_all_samples(
                samples_with_mocks['samples'], grammar)
            for i, res in enumerate(parse_results):
                response = {
                    'text': samples_with_mocks['samples'][i].get_text(),
                    'is_positive': res[0],
                    'debug_info': res[1],
                    'mock': samples_with_mocks['mocks'][i],
                    'markup': res[2],
                    'target': samples_with_mocks['target'][i]
                }
                responses.append((None, None, response))

            return responses  # here we do not truncate responses by n_samples as it is validation of user samples

        if opts['grammarAsExperiment']:
            result = grammar.to_base64()
            response = {'debug_info': result}
            return [(None, None, response)]

        if opts['almostMatched']:
            logger.debug(
                'Check {} samples for almost matching'.format(len(opts['ids'])))
            ids_to_match = opts['ids']
            samples_to_match = [self.samples[i] for i in ids_to_match]
            all_markups = parse_to_almost_matched_parts(
                samples_to_match, grammar)
            results = zip(ids_to_match, [self.scores[idx]
                                         for idx in ids_to_match], all_markups)
            return results

        if opts['synonyms'] is not None:
            if self.synonyms_server is None:
                logger.error('No synonyms server')
                return []
            text = opts['synonyms']['text'].encode('utf8')
            logger.debug('Synonyms request for {}'.format(text))
            # TODO skip parsing if interval specified
            interval = opts['synonyms'].get('interval', None)

            syn_opts = opts['synonyms'].get('opts', dict())

            sample = Sample.from_text(text)
            result = grammar.parse(sample)
            if not result.is_positive(0):
                return [(None, None, {'text': None, 'query': [], 'results': ['Not matched']})]

            slots = result.get_best_variant_slots(0)
            grammar_synonyms = []
            if syn_opts.get('grammarSynonyms', False):
                grammar_synonyms = get_grammar_synonyms(
                    grammar, slots, interval)

            parsed = markup_into_slots(text, slots)

            if interval is not None:
                start, end = interval
                synonyms = fetch_synonyms(text, start, end, host=self.synonyms_server,
                                          grammar_synonyms=grammar_synonyms, **syn_opts)
            else:
                nonterminal = next(slots.iter_nonterminals())
                start, end = nonterminal.get_interval()
                logger.debug('{} {} {}'.format(text, start, end))
                synonyms = fetch_synonyms(text, start, end, host=self.synonyms_server,
                                          grammar_synonyms=grammar_synonyms, **syn_opts)

            return [(None, None, {'text': text, 'query': parsed, 'results': synonyms, 'interval': [start, end]})]

        if opts['updateGrammar'] is not None:
            logger.debug('Update grammar')
            sample = Sample.from_text(
                opts['updateGrammar']['sample'].encode('utf8'))
            interval = opts['updateGrammar']['interval']
            value = opts['updateGrammar']['value'].encode('utf8')
            grammar_text = update_nonterminal(sample, interval, grammar, value)
            logger.debug(grammar_text)
            return [(None, None, {'grammar': grammar_text.encode('utf8')})]

        if self.dataset is not None:
            logger.debug('Parse samples from the dataset')
            parse_results = self.dataset.parse(
                grammar, opts['needValues'], opts['needTypes'])
        else:
            logger.debug('Parse samples from samples list')
            parse_results = parse_samples(
                self.samples, grammar, opts['needValues'], opts['needTypes'])

        results = []
        for parsed, idx in parse_results:
            results.append((idx, self.scores[idx], parsed))
        results = sorted(results, key=lambda x: -x[1])[:n_samples]
        return results

    def estimate_preprocessed(self, preprocessed_query):
        return self.scores

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        grammars_dir = index_config.get('grammars_dir', None)
        if grammars_dir is None:
            logger.warning('No grammars dir specified, imports won\'t work')

        synonyms_server = index_config.get('synonyms_server', None)
        if synonyms_server is None:
            logger.warning('No synonyms server specifies, synonyms won\'t work')

        logger.debug('Fetching scores for granet index')
        if (payload is not None) and ('occurrence_rate' in payload):
            scores = [float(freq) for freq in payload['occurrence_rate']]
        else:
            scores = [1] * len(texts)

        is_paskills = index_config.get('is_paskills', False)

        logger.debug('Reading dataset')
        mocks_path = index_config.get('mocks_path', None)
        if mocks_path is None:
            return GranetIndex.from_texts(texts, scores, grammars_dir, synonyms_server, is_paskills=is_paskills)
        dataset = TsvSampleDataset()
        try:
            dataset.load(mocks_path.encode('utf8'))
        except RuntimeError:
            logger.warning('Empty mocks, start parsing samples online')
            dataset.load(index_config['mocks_path'].encode(
                'utf8'), from_mocks=False)
        return GranetIndex(dataset, scores, grammars_dir, synonyms_server=synonyms_server, is_paskills=is_paskills)

    @staticmethod
    def from_texts(texts, scores, grammars_dir, synonyms_server, is_paskills=False):
        logger.warning(
            'Creating samples from texts, every sample will be sent to Begemot and may fail due to the large number of requests.')
        logger.warning('Please specify mocks_path in config to avoid this.')
        samples = []
        for i, text in enumerate(texts):
            samples.append(Sample.from_text(text.encode('utf8')))
            # without waiting begemot requests fail on large batches
            if i % 10 == 0:
                sleep(30)
            if i % 200 == 0:
                logger.debug('Progress: {:.2%}'.format(i / len(texts)))
        return GranetIndex(None, scores, grammars_dir, samples, synonyms_server=synonyms_server, is_paskills=is_paskills)

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        grammars_dir = index_config.get('grammars_dir', None)
        if grammars_dir is None:
            logger.warning('No grammars dir specified, imports won\'t work')

        synonyms_server = index_config.get('synonyms_server', None)
        if synonyms_server is None:
            logger.warning('No synonyms server specifies, synonyms won\'t work')

        is_paskills = index_config.get('is_paskills', False)

        logger.debug('Fetching scores for granet index')
        scores = dict()
        for skill_id, idxs in skill_to_idxs_map.items():
            if (payload is not None) and ('occurrence_rate' in payload[skill_id]):
                scores[skill_id] = [float(freq) for freq in payload[skill_id]['occurrence_rate']]
            else:
                scores[skill_id] = [1] * len(idxs)

        logger.debug('Reading dataset')
        mocks_path = index_config.get('mocks_path', None)
        if mocks_path is None:
            skill_to_index_map = dict()
            for skill_id, idxs in skill_to_idxs_map.items():
                skill_to_index_map[skill_id] = GranetIndex(
                    texts[skill_id],
                    scores[skill_id],
                    grammars_dir,
                    synonyms_server=synonyms_server,
                    is_paskills=is_paskills
                )
            return skill_to_index_map

        dataset = TsvSampleDataset()
        try:
            dataset.load(mocks_path.encode('utf8'))
        except RuntimeError:
            logger.warning('Empty mocks, start parsing samples online')
            dataset.load(index_config['mocks_path'].encode(
                'utf8'), from_mocks=False)
        is_paskills = index_config.get('is_paskills', False)

        skill_to_index_map = dict()
        for skill_id, idxs in skill_to_idxs_map.items():
            skillDataset = TsvSampleDataset()
            skillDataset.load_part(dataset, idxs)
            skill_to_index_map[skill_id] = GranetIndex(
                skillDataset,
                scores[skill_id],
                grammars_dir,
                synonyms_server=synonyms_server,
                is_paskills=is_paskills
            )
        return skill_to_index_map

    @staticmethod
    def from_texts_by_skill_id(texts, scores, grammars_dir, synonyms_server, skill_to_idxs_map):
        logger.warning(
            'Creating samples for skills from texts, every sample will be sent to Begemot and may fail due to the large number of requests.')
        logger.warning('Please specify mocks_path in config to avoid this.')

        counter = 0
        all_texts_number = 0
        for idxs in skill_to_idxs_map.values():
            all_texts_number += len(idxs)

        skill_to_index_map = dict()
        for skill_id, idxs in skill_to_idxs_map.iteritems():
            samples = []
            for text in texts[skill_id]:
                counter += 1
                samples.append(Sample.from_text(text.encode('utf8')))
                # without waiting begemot requests fail on large batches
                if counter % 10 == 0:
                    sleep(30)
                if counter % 200 == 0:
                    logger.debug('Progress: {:.2%}'.format(counter / all_texts_number))
            skill_to_index_map[skill_id] = GranetIndex(None, scores, grammars_dir, samples, synonyms_server=synonyms_server)

        return skill_to_index_map
