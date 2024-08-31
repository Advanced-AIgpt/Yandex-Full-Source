class DenseFeaturesGuard(object):
    def __init__(self):
        self._signature = None

    def _get_signature(self, batch_features):
        assert len(batch_features) > 0
        # TODO: assert all embedding sizes are equal

        probe_sample_features = batch_features[0]

        signature = {feature_type: {key: value.shape[-1] for key, value in features.iteritems()}
                     for feature_type, features in (('dense', probe_sample_features.dense),
                                                    ('dense_seq', probe_sample_features.dense_seq))}

        return signature

    def fit(self, batch_features):
        self._signature = self._get_signature(batch_features)

        return self

    def check(self, batch_features):
        signature = self._get_signature(batch_features)

        if signature != self._signature:
            raise ValueError(
                'Dense features dimension are not matched: guard contains dense_dims=%r, dense_seq_dims=%r, '
                'trying to create dense_dims=%r, dense_seq_dims=%r' % (
                    self._signature['dense'], self._signature['dense_seq'],
                    signature['dense'], signature['dense_seq']))

    def get_size(self):
        return {
            key: sum(value.itervalues()) for key, value in self._signature.iteritems()
        }

    def load(self, data):
        self._signature = data

    def save(self):
        return self._signature
