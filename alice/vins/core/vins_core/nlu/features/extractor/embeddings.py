# coding: utf-8
import cPickle as pickle

import os
import numpy as np
from marisa_trie import BytesTrie

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, DenseSeqFeatures
from vins_core.utils.data import get_resource_full_path


class EmbeddingsMapBase(object):
    def __init__(self, trie, load_meta=None):
        self._trie = trie
        self._load_meta = load_meta

    def save_to_bin_file(self, filename):
        raise NotImplementedError

    @property
    def meta(self):
        return self._load_meta

    @classmethod
    def load_from_bin_file(cls, filename):
        meta = {'load_type': 'bin_file', 'args': (filename,)}
        if os.path.isdir(filename):
            filename = os.path.join(filename, os.listdir(filename)[0])
        return cls._load_binary(filename, meta)

    @classmethod
    def load_from_bin_resource(cls, resource_id):
        meta = {'load_type': 'bin_resource', 'args': (resource_id,)}
        filename = get_resource_full_path(resource_id)
        if os.path.isdir(filename):
            filename = os.path.join(filename, os.listdir(filename)[0])
        return cls._load_binary(filename, meta)

    @classmethod
    def load_from_text_file(cls, filename):
        raise NotImplementedError

    @classmethod
    def _load_binary(cls, input_file, meta):
        raise NotImplementedError

    def get_value(self, word, default=None):
        if word in self:
            return self.get_value_unsafe(word)
        else:
            return default

    def get_value_unsafe(self, word):
        raise NotImplementedError

    def __contains__(self, word):
        return word in self._trie

    def __getstate__(self):
        return self._load_meta

    def __setstate__(self, state):
        if 'load_type' not in state:
            raise pickle.UnpicklingError(
                'Could not find load metadata '
                'for embedings in serialized data'
            )

        type_ = state['load_type']

        if type_ == 'bin_file':
            method = self.load_from_bin_file
        elif type_ == 'bin_resource':
            method = self.load_from_bin_resource
        elif type_ == 'text_file':
            method = self.load_from_text_file
        else:
            raise ValueError('Unknown load type "{}"'.format(type_))

        obj = method(*state['args'])
        self.__dict__.update(obj.__dict__)


class EmbeddingsMap(EmbeddingsMapBase):
    def __init__(self, trie, load_meta=None):
        super(EmbeddingsMap, self).__init__(trie, load_meta)

        self._feature_count = None

    def get_value_unsafe(self, word):
        return np.fromstring(self._trie[word][0], dtype='float32')

    @classmethod
    def load_from_text_file(cls, filename):
        meta = {'load_type': 'text_file', 'args': (filename,)}
        with open(filename, 'r') as input_file:
            trie = BytesTrie(cls._iter_text(input_file))
            return cls(trie, meta)

    @staticmethod
    def _iter_text(stream):
        feature_count = None
        words = set()

        for line in stream:
            try:
                line = line.decode('utf-8')
            except UnicodeDecodeError:
                # Some embedding files we have contain malformed unicode strings
                continue

            parts = line.rstrip('\n').split('\t')

            word = parts[0]
            assert word not in words, 'Word %s is present more than once in the embedding file' % word
            words.add(word)

            vec = np.array([float(p) for p in parts[1:]], dtype='float32')
            assert feature_count is None or len(vec) == feature_count,\
                '%d features expected, but %d found' % (feature_count, len(vec))

            feature_count = len(vec)
            yield word, vec.tobytes()

    @property
    def feature_count(self):
        if self._feature_count is None:
            key = next(self._trie.iterkeys(), None)
            if key is None:
                return 0
            self._feature_count = len(self.get_value(key))
        return self._feature_count

    @classmethod
    def _load_binary(cls, input_file, meta):
        trie = BytesTrie().mmap(input_file)
        return cls(trie, meta)

    def save_to_bin_file(self, filename):
        self._trie.save(filename)


class EmbeddingsFeaturesExtractor(BaseFeatureExtractor):

    def __init__(self, embeddings_map, **kwargs):
        super(EmbeddingsFeaturesExtractor, self).__init__()

        assert isinstance(embeddings_map, EmbeddingsMap)
        self._embeddings = embeddings_map
        self._zero_embedding = np.zeros(self._embeddings.feature_count, dtype=np.float32)

    def _call(self, sample, **kwargs):
        emb = np.zeros((len(sample), self._embeddings.feature_count), dtype=np.float32)
        for i, token in enumerate(sample.tokens):
            if token in self._embeddings:
                emb[i, :] = self._embeddings.get_value_unsafe(token)
        return [DenseSeqFeatures(emb)]

    @property
    def _features_cls(self):
        return DenseSeqFeatures
