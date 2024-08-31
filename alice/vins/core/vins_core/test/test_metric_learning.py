# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest
import numpy as np
import tensorflow as tf
import attr

from vins_core.nlu.neural.metric_learning.sample_features_preprocessor import SampleMasker
from vins_core.nlu.neural.metric_learning.losses import MarginLoss
from vins_core.utils.misc import is_close
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.common.sample import Sample


@attr.s
class DummyIntentInfo(object):
    positive_sampling = attr.ib(default=True)
    negative_sampling_from = attr.ib(default=None)
    negative_sampling_for = attr.ib(default=None)


@pytest.mark.parametrize(
    'labels, transition_model, intent_infos, out_precomputed_anchors_mask, out_precomputed_negatives_mask', [
        (
            # Only transition model with 2 elliptical intents
            ['intent_x', 'intent_y', 'intent_x__ellipsis', 'intent_y__ellipsis'],
            {
                ('intent_x', 'intent_x'): 1,
                ('intent_x', 'intent_y'): 1,
                ('intent_y', 'intent_x'): 1,
                ('intent_y', 'intent_y'): 1,
                ('intent_x', 'intent_x__ellipsis'): 1,
                ('intent_y', 'intent_y__ellipsis'): 1,
                ('intent_x__ellipsis', 'intent_x'): 1,
                ('intent_x__ellipsis', 'intent_y'): 1,
                ('intent_y__ellipsis', 'intent_x'): 1,
                ('intent_y__ellipsis', 'intent_y'): 1,
                ('intent_x__ellipsis', 'intent_x__ellipsis'): 1,
                ('intent_y__ellipsis', 'intent_y__ellipsis'): 1,
                ('intent_x', 'intent_y__ellipsis'): 0,
                ('intent_y', 'intent_x__ellipsis'): 0,
                ('intent_x__ellipsis', 'intent_y__ellipsis'): 0,
                ('intent_y__ellipsis', 'intent_x__ellipsis'): 0
            },
            None,
            np.array([1, 1, 1, 1], dtype=np.bool),
            np.array([
                [0, 1, 1, 1],
                [1, 0, 1, 1],
                [1, 1, 0, 0],
                [1, 1, 0, 0]
            ], dtype=np.bool)
        ),
        #  Only negatives_samples_from_intents
        (
            ['intent_w', 'intent_x', 'intent_y', 'intent_z'],
            None,
            {
                'intent_w': DummyIntentInfo(
                    negative_sampling_from='intent_x|intent_y'
                ),
                'intent_x': DummyIntentInfo(
                    negative_sampling_from='intent_z'
                ),
                'intent_y': DummyIntentInfo(),
                'intent_z': DummyIntentInfo(
                    positive_sampling=False
                )
            },
            np.array([1, 1, 1, 0], dtype=np.bool),
            np.array([
                [0, 1, 1, 0],
                [0, 0, 0, 1],
                [1, 1, 0, 1],
                [1, 1, 1, 0]
            ], dtype=np.bool)
        ),
        # negative_sampling_for test
        (
            ['intent_x', 'intent_y', 'intent_z'],
            None,
            {
                'intent_x': DummyIntentInfo(
                    negative_sampling_from='intent_x|intent_y'
                ),
                'intent_y': DummyIntentInfo(
                    negative_sampling_for='intent_y|intent_z'
                ),
                'intent_z': DummyIntentInfo(
                    negative_sampling_for='.*'
                )
            },
            np.array([1, 1, 1], dtype=np.bool),
            np.array([
                [0, 0, 0],
                [1, 0, 1],
                [1, 1, 0]
            ], dtype=np.bool)
        )
    ]
)
def test_sample_masker(labels, transition_model, intent_infos,
                       out_precomputed_anchors_mask, out_precomputed_negatives_mask):
    masker = SampleMasker(labels, transition_model, intent_infos)
    precomputed_anchors_mask = masker.get_precomputed_anchors_mask()
    precomputed_negatives_mask = masker.get_precomputed_negatives_mask()
    assert np.array_equal(precomputed_anchors_mask, out_precomputed_anchors_mask)
    assert np.array_equal(precomputed_negatives_mask, out_precomputed_negatives_mask)


@pytest.mark.parametrize(
    'input, output', [
        (
            dict(
                loss_class=MarginLoss,
                samples=np.array([
                    [1, 0.1],
                    [-0.1, 1],
                    [1, 1],
                    [-1, -1]
                ], dtype=np.float32),
                precomputed_anchors_mask=np.array([1, 1, 0], dtype=np.bool),
                precomputed_negatives_mask=np.array([
                    [0, 1, 1],
                    [1, 0, 1],
                    [0, 0, 0]
                ], dtype=np.bool),
                positive_mining='hard',
                negative_mining='semihard',
                num_positives=1,
                num_negatives=1,
                labels=np.array([0, 0, 1, 2], dtype=np.int32),
                threshold=1.0
            ),
            dict(
                sims=np.array([
                    [1.01, 0, 1.1, -1.1],
                    [0, 1.01, 0.9, -0.9],
                    [1.1, 0.9, 2., -2.]
                ], dtype=np.float32),
                pos_sims=np.array([[0], [0], [1.1]], dtype=np.float32),
                neg_sims=np.array([[-1.1], [-0.9], [0.9]], dtype=np.float32),
                pos_sims_recall=np.array([[0], [0], [1.1]], dtype=np.float32),
                neg_sims_recall=np.array([[1.1], [0.9], [1.1]], dtype=np.float32),
                loss=(2.2 + 0.0) / 3.0
            )
        ),
        (
            dict(
                loss_class=MarginLoss,
                samples=np.array([
                    [1, 0.1],
                    [-0.1, 1],
                    [1, 1],
                    [-1, -1]
                ], dtype=np.float32),
                precomputed_anchors_mask=np.array([1, 1, 0], dtype=np.bool),
                precomputed_negatives_mask=np.array([
                    [0, 1, 1],
                    [1, 0, 1],
                    [0, 0, 0]
                ], dtype=np.bool),
                positive_mining='hard',
                negative_mining='hard',
                num_positives=1,
                num_negatives=1,
                labels=np.array([0, 0, 1, 2], dtype=np.int32),
                threshold=1.0
            ),
            dict(
                sims=np.array([
                    [1.01, 0, 1.1, -1.1],
                    [0, 1.01, 0.9, -0.9],
                    [1.1, 0.9, 2., -2.]
                ], dtype=np.float32),
                pos_sims=np.array([[0], [0], [1.1]], dtype=np.float32),
                neg_sims=np.array([[1.1], [0.9], [1.1]], dtype=np.float32),
                pos_sims_recall=np.array([[0], [0], [1.1]], dtype=np.float32),
                neg_sims_recall=np.array([[1.1], [0.9], [1.1]], dtype=np.float32),
                loss=(2.2 + 0.4) / 3.0
            )
        ),
        # the test below checks that the diagonal of sims matrix is masked properly
        # for positive/negative mining in presence of non-trivial anchors mask
        (
            dict(
                loss_class=MarginLoss,
                samples=np.array([
                    [1, 0.1],
                    [-0.1, 1],
                    [1, 1]
                ], dtype=np.float32),
                precomputed_anchors_mask=np.array([0, 1], dtype=np.bool),
                precomputed_negatives_mask=np.array([
                    [0, 1],
                    [1, 0]
                ], dtype=np.bool),
                positive_mining='hard',
                negative_mining='hard',
                num_positives=1,
                num_negatives=1,
                labels=np.array([0, 1, 1], dtype=np.int32),
                threshold=1.0
            ),
            dict(
                sims=np.array([
                    [0, 1.01, 0.9],
                    [1.1, 0.9, 2.]
                ], dtype=np.float32),
                pos_sims=np.array([[.9], [.9]], dtype=np.float32),
                neg_sims=np.array([[0], [1.1]], dtype=np.float32),
                pos_sims_recall=np.array([[.9], [.9]], dtype=np.float32),
                neg_sims_recall=np.array([[0], [1.1]], dtype=np.float32),
                loss=(0.4 + 0.2) / 2.0
            )
        )
    ])
def test_losses(input, output):
    with tf.Graph().as_default():
        loss_obj = input['loss_class'](
            samples=tf.convert_to_tensor(input['samples']),
            precomputed_anchors_mask=input['precomputed_anchors_mask'],
            precomputed_negatives_mask=input['precomputed_negatives_mask'],
            positive_mining=input['positive_mining'],
            negative_mining=input['negative_mining'],
            labels=tf.convert_to_tensor(input['labels']),
            num_positives=input['num_positives'],
            num_negatives=input['num_negatives'],
            threshold=input['threshold']
        )
        sims_t = loss_obj._sims
        pos_sims_t, neg_sims_t = loss_obj._get_sims_for_loss()
        pos_sims_recall_t, neg_sims_recall_t = loss_obj._get_sims_for_recall()
        loss_t = loss_obj.output_feed['loss']
        with tf.Session() as sess:
            sess.run(tf.global_variables_initializer())
            sess.run(tf.local_variables_initializer())
            [sims, pos_sims, neg_sims, pos_sims_recall, neg_sims_recall, loss] = sess.run([
                sims_t, pos_sims_t, neg_sims_t, pos_sims_recall_t, neg_sims_recall_t, loss_t
            ])
    assert is_close(sims, output['sims'])
    assert is_close(pos_sims, output['pos_sims'])
    assert is_close(neg_sims, output['neg_sims'])
    assert is_close(pos_sims_recall, output['pos_sims_recall'])
    assert is_close(neg_sims_recall, output['neg_sims_recall'])
    assert is_close(loss, output['loss'])


def _gen_dummy_sample_feature(token, feature):
    num = 2
    return SampleFeatures(
        sample=Sample.from_string(' '.join([token] * num)),
        sparse_seq={
            'word': [[SparseFeatureValue(token)]] * num,
            'feature': [[SparseFeatureValue('token'), SparseFeatureValue(feature)]] * num
        }
    )
