from collections import defaultdict
import logging
import pickle

from verstehen.index.index_registry import IndexRegistry

logger = logging.getLogger(__name__)


class UtteranceSearchApp:
    """
    Class to decorate text index with the ability to attach any parallel payload
    to be returned along with the results of the unerlying index.
    """

    def __init__(self, texts, indexes_configs, payload=None, grouped_by_skill_id=False):
        """
        Arguments:
            texts: (list) texts to use for index creation
            indexes_configs: (list) list of objects to use to build indexes (configs)
            payload: (dict of lists) optional dict of lists of size of `texts` that are associated with texts and will
                be returned as a part of asearch
            grouped_by_skill_id: (bool) flag that enables grouping logs by skill_id from payload
        """
        if payload is not None:
            if not isinstance(payload, dict):
                raise ValueError('If specified, payload must me a dict')

            for key, value in payload.items():
                if len(value) != len(texts):
                    raise ValueError(
                        'Payload\'s key `{}` length does not match the length of texts. Were {} expected {}'.format(
                            key, len(value), len(texts)
                        )
                    )

        logger.debug('Creating search app with the following indexes {}...'.format(
            ['{} ({})'.format(conf['name'], conf['index_type'])
             for conf in indexes_configs]
        ))

        self.grouped_by_skill_id = grouped_by_skill_id

        self.text_indexes_map = dict()

        if grouped_by_skill_id:
            skill_to_idxs_map = defaultdict(list)
            for i, skill_id in enumerate(payload['skill_id']):
                skill_to_idxs_map[skill_id].append(i)

            self.payload = dict()
            self.texts = dict()
            for skill_id, idxs in skill_to_idxs_map.items():
                self.payload[skill_id] = {key: [payload[key][idx] for idx in idxs] for key in payload}
                self.texts[skill_id] = [texts[i] for i in idxs]

            for index_config in indexes_configs:
                index_name = index_config['name']
                self.text_indexes_map[index_name] = self._create_indexes_by_skill_id(
                    index_config, self.texts, skill_to_idxs_map)
        else:
            self.texts = texts
            self.payload = payload
            for index_config in indexes_configs:
                index_name = index_config['name']
                self.text_indexes_map[index_name] = self._create_index(
                    index_config, texts)

    def _get_necessary_indexes(self, necessary_indexes):
        initialized_indexes = self.text_indexes_map.keys()

        all_there = all(
            [index in initialized_indexes for index in necessary_indexes])
        if not all_there:
            raise ValueError(
                'There is a need to have necessary indexes {} but there are only {}'.format(
                    necessary_indexes, initialized_indexes)
            )

        return [self.text_indexes_map[index] for index in necessary_indexes]

    def search(self, query, n_samples=None, index_name=None, filters=None, skill_id=None):
        logger.debug('Incoming query length={}, n_samples={}, index_name={}, filters={}'.format(
            len(query), n_samples, index_name, filters)
        )
        if index_name is None:
            if len(self.text_indexes_map) > 1:
                raise ValueError(
                    'The index app has more than one text indexes. You need to specify index_name for search call.'
                )
            index_name = self.text_indexes_map.keys()[0]

        if self.grouped_by_skill_id:
            if skill_id is None:
                raise ValueError(
                    'App indexes are grouped by skill id but skill id parameter not specified in search request'
                )
            if skill_id not in self.text_indexes_map[index_name]:
                logger.error('Invalid skill_id in search')
                return []
            search_results = self.text_indexes_map[index_name][skill_id].search(
                query, n_samples=n_samples)
        else:
            search_results = self.text_indexes_map[index_name].search(
                query, n_samples=n_samples)
        results = []

        for i, res in enumerate(search_results):
            idx = res[0]
            score = res[1]
            index_payload = res[2] if len(res) > 2 else None
            if self.grouped_by_skill_id:
                payload = self._prepare_payload_response_by_skill_id(skill_id, idx)
                text = self.texts[skill_id][idx] if idx is not None else None
            else:
                payload = self._prepare_payload_response(idx)
                text = self.texts[idx] if idx is not None else None
            results.append({
                'score': score,
                'payload': payload,
                'index_payload': index_payload,
                'text': text,
                'idx': idx
            })

        if filters is not None:
            results = UtteranceSearchApp._filter_results(results, filters)
        return results

    def estimate(self, query, index_name=None, filters=None, skill_id=None):
        logger.debug('Incoming query length={}, index_name={}'.format(
            len(query), index_name)
        )
        if index_name is None:
            if len(self.text_indexes_map) > 1:
                raise ValueError(
                    'The index app has more than one text indexes. You need to specify index_name for search call.'
                )
            index_name = self.text_indexes_map.keys()[0]

        if self.grouped_by_skill_id:
            if skill_id is None:
                raise TypeError(
                    'App indexes are grouped by skill id but skill id is not specified in search request'
                )
            search_results = self.text_indexes_map[index_name][skill_id].estimate(query)
        else:
            search_results = self.text_indexes_map[index_name].estimate(query)
        results = []
        for i, res in enumerate(search_results):
            score = res
            results.append({
                'score': score,
                'text': self.texts[skill_id][i]
            })
        logger.debug('Estimate results {}, out of {}'.format(len(results), len(self.texts)))
        if filters is not None:
            results = UtteranceSearchApp._filter_results(
                results, filters, filter_includes=True)
        return results

    def _prepare_payload_response(self, id):
        if self.payload is None or id is None:
            return None

        return {key: self.payload[key][id] for key in self.payload}

    def _prepare_payload_response_by_skill_id(self, skill_id, id):
        if self.payload is None or id is None:
            return None

        return {key: self.payload[skill_id][key][id] for key in self.payload[skill_id]}

    def pickle_to_file(self, path):
        with open(path, 'w') as f:
            pickle.dump(self, f)

    def _create_index(self, index_config, texts):
        return IndexRegistry.create_index(index_config, texts,
                                          payload=self.payload,
                                          indexes_map=self.text_indexes_map)

    def _create_indexes_by_skill_id(self, index_config, texts, skill_to_idxs_map):
        return IndexRegistry.create_indexes_by_skill_id(index_config, texts,
                                                        skill_to_idxs_map,
                                                        payload=self.payload,
                                                        indexes_map=self.text_indexes_map)

    @staticmethod
    def _filter_results(results, filters, filter_includes=False):
        filters = set(filters)
        if filter_includes:
            filtered_results = list(
                filter(lambda res: res['text'] in filters, results))
        else:
            filtered_results = list(
                filter(lambda res: res['text'] not in filters, results))
        return filtered_results

    @staticmethod
    def from_intent_dict(intents_to_texts, indexes_configs):
        """
        Creating search app from the map str -> list of strings.
        """
        # extracting texts and associated intents to them
        all_texts, all_intents = [], []
        for intent, texts in intents_to_texts.items():
            all_texts.extend(texts)
            all_intents.extend([intent] * len(texts))

        return UtteranceSearchApp(
            all_texts, indexes_configs=indexes_configs, payload=all_intents
        )

    @staticmethod
    def from_texts(texts, indexes_configs, payload=None, grouped_by_skill_id=False):
        """
        Creating search app from the list of strings.
        """
        return UtteranceSearchApp(texts, indexes_configs=indexes_configs,
                                  payload=payload, grouped_by_skill_id=grouped_by_skill_id)

    @staticmethod
    def from_config(app_config):
        logger.debug('Creating app `{}` from config'.format(
            app_config['name']))
        logger.debug('Reading texts from {}...'.format(
            app_config['texts_path']))
        with open(app_config['texts_path']) as f:
            texts = pickle.load(f)

        if 'payload_path' in app_config:
            logger.debug('Reading payload from {}...'.format(
                app_config['payload_path']))
            with open(app_config['payload_path']) as f:
                payload = pickle.load(f)

            if 'payload_keys' in app_config:
                keys = app_config['payload_keys']
                logger.debug(
                    'Reducing payload to the following keys only: `{}`'.format(
                        keys
                    )
                )
                payload = {key: payload[key] for key in keys}
        else:
            payload = None

        return UtteranceSearchApp.from_texts(texts, app_config['indexes'], payload, app_config.get('grouped_by_skill_id', False))
