import struct
import numpy as np

from vins_core.utils.serialize import string_to_bytes, bytes_to_string, nested_list_items_to_bytes_with_to_bytes, \
    bytes_to_nested_list_items_with_from_bytes
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.nlu.features.base import Features, SampleFeatures
from vins_core.utils.strings import smart_unicode, smart_utf8
from vins_core.common.sample import Sample


def _sparse_feature_value_to_bytes(sparse_feature_value):
    value = smart_utf8(sparse_feature_value.value)
    value_len = len(value)
    return struct.pack('<i%dsf' % value_len, value_len, value, sparse_feature_value.weight)


def _sparse_feature_value_from_bytes(b):
    value_len = struct.unpack('<i', b[:4])
    value, weight = struct.unpack('<%dsf' % value_len, b[4:])
    return SparseFeatureValue(value=smart_unicode(value), weight=weight)


def _sparse_to_bytes(feature):
    return string_to_bytes(nested_list_items_to_bytes_with_to_bytes(feature, _sparse_feature_value_to_bytes))


_sparse_seq_to_bytes = _sparse_to_bytes


def _dense_to_bytes(feature):
    return string_to_bytes(feature.tobytes())


def _dense_seq_to_bytes(feature):
    b = feature.tobytes()
    nrows, ncols = feature.shape
    return struct.pack('<iii%ds' % len(b), len(b), nrows, ncols, b)


def _dense_seq_ids_to_bytes(feature):
    b = np.array(feature, dtype=np.int32).tobytes()
    return struct.pack('<i{}s'.format(len(b)), len(b), b)


def _sparse_from_bytes(b, offset):
    sparse_feature_bytes, offset = bytes_to_string(b, offset)
    return bytes_to_nested_list_items_with_from_bytes(sparse_feature_bytes, _sparse_feature_value_from_bytes), offset


_sparse_seq_from_bytes = _sparse_from_bytes


def _dense_from_bytes(b, offset):
    dense_feature_bytes, offset = bytes_to_string(b, offset)
    return np.fromstring(dense_feature_bytes, dtype=np.float32), offset


def _dense_seq_from_bytes(b, offset):
    size, nrows, ncols = struct.unpack('<iii', b[offset:(offset + 12)])
    offset += 12
    arr = np.fromstring(b[offset:(offset + size)], dtype=np.float32)
    offset += size

    return np.reshape(arr, newshape=(nrows, ncols)), offset


def _dense_seq_ids_from_bytes(b, offset):
    size, = struct.unpack('<i', b[offset:(offset + 4)])
    offset += 4
    arr = np.fromstring(b[offset:(offset + size)], dtype=np.int32)
    offset += size

    return arr, offset


_FEATURES_TO_BYTES = {
    Features.SPARSE: _sparse_to_bytes,
    Features.SPARSE_SEQ: _sparse_seq_to_bytes,
    Features.DENSE: _dense_to_bytes,
    Features.DENSE_SEQ: _dense_seq_to_bytes,
    Features.DENSE_SEQ_IDS: _dense_seq_ids_to_bytes
}


_FEATURES_FROM_BYTES = {
    Features.SPARSE: _sparse_from_bytes,
    Features.SPARSE_SEQ: _sparse_seq_from_bytes,
    Features.DENSE: _dense_from_bytes,
    Features.DENSE_SEQ: _dense_seq_from_bytes,
    Features.DENSE_SEQ_IDS: _dense_seq_ids_from_bytes
}


def _sample_from_bytes(b, offset):
    sample_bytes, offset = bytes_to_string(b, offset)
    len_token_text, len_tags_text = struct.unpack('<ii', sample_bytes[:8])
    token_text, tags_text, weight = struct.unpack('<%ds%dsf' % (len_token_text, len_tags_text), sample_bytes[8:])
    return Sample(
        tokens=smart_unicode(token_text).split(),
        tags=smart_unicode(tags_text).split(),
        weight=weight), offset


def _sample_to_bytes(sample):
    # TODO: add Utterance, Annotations
    token_text = smart_utf8(sample.text)
    tags_text = smart_utf8(' '.join(sample.tags))
    return string_to_bytes(struct.pack(
        '<ii%ds%dsf' % (len(token_text), len(tags_text)),
        len(token_text), len(tags_text), token_text, tags_text, sample.weight))


_FEATURE_TYPES_ORDER = [
    Features.SPARSE,
    Features.SPARSE_SEQ,
    Features.DENSE,
    Features.DENSE_SEQ,
    Features.DENSE_SEQ_IDS
]


def sample_features_to_bytes(sample_features, order):
    out = b''
    out += _sample_to_bytes(sample_features.sample)

    for feature_type in _FEATURE_TYPES_ORDER:
        to_bytes = _FEATURES_TO_BYTES[feature_type]
        features = sample_features.features[feature_type]

        for feature_id in order[feature_type]:
            if feature_id in features:  # checking if this feature was empty while adding
                out += struct.pack('?', False)
                out += to_bytes(features[feature_id])
            else:
                out += struct.pack('?', True)

    return out


def sample_features_from_bytes(b, order):
    offset = 0

    sample, offset = _sample_from_bytes(b, offset)

    features = {key: {} for key in list(Features)}

    for feature_type in _FEATURE_TYPES_ORDER:
        from_bytes = _FEATURES_FROM_BYTES[feature_type]
        curr_features = features[feature_type]

        for feature_id in order[feature_type]:
            empty, = struct.unpack('?', b[offset:offset + 1])
            offset += 1

            if not empty:
                curr_features[feature_id], offset = from_bytes(b, offset)

    return SampleFeatures.from_features(sample, features)
