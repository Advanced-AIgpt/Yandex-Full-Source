import json
import os
from alice.nlu.tools.binary_classifier.dataset import Dataset
from alice.nlu.tools.binary_classifier.embeddings import sentence_embedding_from_base64, find_embedding_in_dataset_row
from alice.nlu.tools.binary_classifier.stub_fetcher import StubFetcherCollection


# ==== utils ====

def _update_feature_max(features, name, value):
    features[name] = max(features.get(name, 0), value)


# ==== DatasetWrap ====

class DatasetWrap(object):
    def __init__(self, dataset, stub_fetcher=None):
        self.dataset = dataset
        self.stub_fetcher = stub_fetcher
        self.caches = {}

    def row_count(self):
        return self.dataset.row_count()

    def get_text(self, index):
        return self.dataset.get_text(index)

    def get_features(self, index):
        cache = self._get_cache('features')
        if cache[index] is None:
            cache[index] = self._create_features(index)
        return cache[index]

    def _create_features(self, index):
        features = {}
        mock = self._create_mock(index)
        token_count = max(1, len(mock['TokenBegin']))
        entities = mock['Entities']
        for name, begin, end in zip(entities.get('Type', []), entities.get('Begin', []), entities.get('End', [])):
            _update_feature_max(features, name + '.relative_length', (end - begin) / token_count)
        return features

    def get_entities(self, index):
        return self.get_mock(index).get('Entities', {})

    def get_mock(self, index):
        cache = self._get_cache('mock')
        if cache[index] is None:
            cache[index] = self._create_mock(index)
        return cache[index]

    def _create_mock(self, index):
        if self.dataset.has_mock():
            return self.dataset.get_mock(index)
        return json.loads(self._fetch_stub(index, 'mock'))

    def get_embedding(self, index, embedding_name):
        cache = self._get_cache(embedding_name)
        if cache[index] is None:
            cache[index] = self._create_embedding(index, embedding_name)
        return cache[index]

    def _create_embedding(self, index, embedding_name):
        embedding = find_embedding_in_dataset_row(self.dataset, index, embedding_name)
        if embedding is None:
            embedding = self._fetch_stub(index, embedding_name)
        return sentence_embedding_from_base64(embedding)

    def _get_cache(self, name):
        if name not in self.caches:
            self.caches[name] = [None] * self.dataset.row_count()
        return self.caches[name]

    def _fetch_stub(self, index, stub_name):
        if self.stub_fetcher is None:
            raise Exception('No stub_storage')
        text = self.dataset.get_text(index)
        return self.stub_fetcher.fetch(text, stub_name)


# ==== DatasetLoader ====

OUTPUT_DATASET_FORMATS = ['tsv', 'txt', 'yt']
INPUT_DATASET_FORMATS = ['tsv', 'txt', 'yt', 'value']


class DatasetLoader(object):
    def __init__(self):
        self.cache = {}
        self.stub_fetchers = StubFetcherCollection()

    def load_from_config(self, config, target, stub_storage_path):
        datasets = []
        for format in INPUT_DATASET_FORMATS:
            values = config.get(target + '_' + format, [])
            if isinstance(values, str):
                values = [values]
            if not values:
                continue
            if format == 'value':
                dataset = Dataset(header=['text'], rows=[[value] for value in values])
                datasets.append(self.create_dataset(dataset, stub_storage_path))
                continue
            for path in values:
                datasets.append(self.load(path, format, stub_storage_path))
        return datasets

    def load(self, path, format, stub_storage_path):
        record = self.cache.setdefault((path, format), {})
        if format == 'yt':
            # todo(samoylovboris)
            timestamp = ''
        else:
            timestamp = os.path.getmtime(path)
        has_cache = record.get('timestamp') == timestamp
        print('    Load "%s"%s.' % (path, (' from cache' if has_cache else '')))
        if not has_cache:
            record['timestamp'] = timestamp
            record['dataset'] = self.create_dataset(Dataset.load(path, format), stub_storage_path)
        return record['dataset']

    def create_dataset(self, dataset, stub_storage_path):
        stub_fetcher = self.stub_fetchers.get_or_create(stub_storage_path)
        return DatasetWrap(dataset, stub_fetcher)
