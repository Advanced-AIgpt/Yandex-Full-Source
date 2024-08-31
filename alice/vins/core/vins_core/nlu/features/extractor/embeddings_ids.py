import struct
import numpy as np

from marisa_trie import RecordTrie

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, DenseSeqIdFeatures
from vins_core.nlu.features.extractor.embeddings import EmbeddingsMapBase
from vins_core.utils.misc import read_exactly


class EmbeddingsMapIds(EmbeddingsMapBase):
    def __init__(self, trie, embeddings_matrix, load_meta=None):
        super(EmbeddingsMapIds, self).__init__(trie, load_meta)

        self._embeddings_matrix = embeddings_matrix

    @property
    def embeddings_matrix(self):
        return self._embeddings_matrix

    def get_value(self, word, default=0):
        return super(EmbeddingsMapIds, self).get_value(word, default)

    def get_value_unsafe(self, word):
        return self._trie[word][0][0]

    @staticmethod
    def dump_binary(output_file, key_value):
        trie_data = []
        embeddings_matrix = []

        for i, (key, value) in enumerate(key_value):
            trie_data.append((key, (i + 1,)))
            embeddings_matrix.append(value)

        if not embeddings_matrix:
            raise ValueError('There is no embeddings to dump')
        else:
            embeddings_matrix.insert(0, np.zeros_like(embeddings_matrix[0], dtype=np.float32))

        trie = RecordTrie('I', trie_data)
        embeddings_matrix = np.array(embeddings_matrix, dtype=np.float32)

        EmbeddingsMapIds(trie, embeddings_matrix).save_to_bin_file(output_file)

    @classmethod
    def _load_binary(cls, input_file, meta):
        with open(input_file, 'rb') as infile:
            (trie_size,) = struct.unpack('i', read_exactly(infile, 4))
            trie = RecordTrie('I').frombytes(read_exactly(infile, trie_size))

        matrix = np.memmap(input_file, mode='r', offset=4 + trie_size, dtype=np.float32).reshape((len(trie) + 1, -1))

        return cls(trie, matrix, meta)

    def save_to_bin_file(self, filename):
        trie_bytes = self._trie.tobytes()
        trie_size = len(trie_bytes)
        entry = struct.pack('i', trie_size)

        with open(filename, 'wb') as outfile:
            outfile.write(entry)
            outfile.write(trie_bytes)

        fp = np.memmap(filename, mode='r+', offset=4 + trie_size,
                       dtype=np.float32, shape=self._embeddings_matrix.shape)
        fp[:] = self._embeddings_matrix


class EmbeddingsIdsFeaturesExtractor(BaseFeatureExtractor):
    def __init__(self, embeddings_map, embeddings_map_key, **kwargs):
        super(EmbeddingsIdsFeaturesExtractor, self).__init__()

        self._embeddings_map = embeddings_map
        self._embeddings_map_key = embeddings_map_key

    def _call(self, sample, **kwargs):
        ids = [self._embeddings_map.get_value(token) for token in sample.tokens]

        return [DenseSeqIdFeatures(ids)]

    def get_classifiers_info(self):
        return {
            self._embeddings_map_key: self._embeddings_map
        }

    @property
    def _features_cls(self):
        return DenseSeqIdFeatures
