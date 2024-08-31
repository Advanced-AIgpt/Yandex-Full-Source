# -*- coding: utf-8 -*-
import unittest
import pickle
import json
import operator
import numpy as np
from numpy.testing import assert_array_equal
from alice.paskills.penguinarium.ml.index import (
    SklearnIndex, ModelFactory, SerializedModel, NotBuildedError,
    BaseIndex
)


class TestSklearnIndex(unittest.TestCase):
    def test_custom_metric(self):
        si = SklearnIndex(
            thresh=10.,
            dist_thresh_rel=operator.lt,
            metric=lambda x, y: 9.,
            n_neighbors=1,
            p=2
        )

        si.build(embeddings=np.full((5, 10), 0.), intents=np.ones(5))
        embed = np.full(10, 1000.)
        assert si.search(embed) is not None

        si._thresh = 8.
        assert len(si.search(embed).preds) == 0

    def test_sklearn_pickle_and_search(self):
        model_params = {
            'thresh': 1.,
            'dist_thresh_rel': operator.lt,
            'metric': 'minkowski',
            'n_neighbors': 1,
            'p': 2
        }
        si_factory = ModelFactory(
            SklearnIndex,
            **model_params
        )
        si = si_factory.produce()

        with self.assertRaises(RuntimeError):
            si.serialize()

        embs = np.concatenate((
            np.full((1, 10), 5.,),
            np.full((5, 10), 10.),
        )).astype(np.float32)
        intents = np.concatenate((
            np.zeros(1),
            np.ones(5),
        ))
        si.build(embeddings=embs, intents=intents)
        ser = si.serialize()

        assert type(ser) is SerializedModel
        assert type(ser.meta) is str
        assert type(ser.binary) is bytes

        json.loads(ser.meta)  # check is valid json
        assert len(pickle.dumps(si)) > len(ser.meta.encode('utf-8')) + len(ser.binary)  # Test serialization overhead

        si_loaded = si_factory.load(ser)

        for param, val in model_params.items():
            assert getattr(si_loaded, '_' + param) == val

        assert_array_equal(si_loaded._embeddings, embs)
        assert_array_equal(si_loaded.embeddings, embs)
        assert_array_equal(si_loaded._intents, intents)
        assert_array_equal(si_loaded.intents, intents)

        assert si_loaded.search(np.full(10, 5.)).preds == np.array([0])
        assert not si_loaded.search(np.full(10, 0.)).preds

    def test_not_builded_raises(self):
        si = SklearnIndex(
            thresh=1.,
            dist_thresh_rel=operator.lt,
            metric='minkowski',
            n_neighbors=1,
            p=2
        )
        with self.assertRaises(RuntimeError):
            si.search(np.repeat(0., 100))

    def test_top(self):
        si = SklearnIndex(
            thresh=5.,
            dist_thresh_rel=operator.lt,
            metric='minkowski',
            n_neighbors=3,
            p=2
        )

        embs = np.concatenate((
            np.full((1, 10), 6.),
            np.full((10, 10), 5.),
            np.full((5, 10), 10.),
        ))
        intents = np.concatenate((
            np.full(1, 0),
            np.full(10, 1),
            np.full(5, 2),
        ))
        si.build(embeddings=embs, intents=intents)

        search_res = si.search(np.full(10, 5.))
        assert_array_equal(search_res.preds, [1, 0])

    def test_not_builded(self):
        si = SklearnIndex(
            thresh=5.,
            dist_thresh_rel=operator.lt,
            metric='minkowski',
            n_neighbors=3,
            p=2
        )

        with self.assertRaises(NotBuildedError):
            si.embeddings
        with self.assertRaises(NotBuildedError):
            si.intents


class TestBaseIndex(unittest.TestCase):
    def test_abstract(self):
        assert BaseIndex._build(None, None, None) is None
        assert BaseIndex._search(None, None) is None
        assert BaseIndex._serialize(None) is None
        assert BaseIndex.load(None) is None
