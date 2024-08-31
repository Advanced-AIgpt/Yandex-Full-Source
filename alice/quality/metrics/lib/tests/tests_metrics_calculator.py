import collections
import math
import numpy as np
import pytest
import random
import yatest

from sklearn.metrics import f1_score
from sklearn.preprocessing import MultiLabelBinarizer
from alice.quality.metrics.lib.binary.binary_metrics_accumulator import BinaryMetricsAccumulator
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder
from alice.quality.metrics.lib.multiclass.multiclass_metrics_accumulator import MulticlassMetricsAccumulator
from alice.quality.metrics.lib.multilabel.input_converter import MissingWeightPolicy
from alice.quality.metrics.lib.multilabel.multilabel_metrics_accumulator import MultilabelMetricsAccumulator


@pytest.fixture
def intent_label_encoder():
    return IntentLabelEncoder(yatest.common.test_source_path('label_encode.json'), target_column='intent')


def test_binary():
    acc = BinaryMetricsAccumulator(
        threshold=0.5
    )

    samples = []
    for _ in range(100):
        samples.append((
            random.randint(0, 1),
            random.randint(0, 1),
            random.randint(1, 100)
        ))
    for _ in range(100):
        num_samples = random.randint(1, 10)
        samples.append((
            np.random.randint(2, size=num_samples).tolist(),
            random.random(),
            np.random.randint(1, 100, size=num_samples).tolist()
        ))
    y_true = []
    y_pred = []
    weights = []
    for true_intent, pred_intent, weight in samples:
        acc.add(true_intent, pred_intent, weight)
        if not isinstance(true_intent, list):
            true_intent = [true_intent]
        if not isinstance(weight, list):
            weight = [weight]
        for target, sample_weight in zip(true_intent, weight):
            y_true.append(target)
            y_pred.append(1 if pred_intent >= 0.5 else 0)
            weights.append(sample_weight)
        true_f1 = f1_score(y_true, y_pred, sample_weight=weights, zero_division=0)
        test_f1 = acc.get_metric_by_name("F1").get_f1()
        assert math.isclose(test_f1, true_f1, rel_tol=1e-5)


def test_multiclass(intent_label_encoder):
    acc = MulticlassMetricsAccumulator(
        intent_label_encoder,
        threshold=0.5
    )
    intents = intent_label_encoder.intents
    samples = []
    for _ in range(100):
        samples.append((
            random.choice(intents),
            random.randint(0, len(intents) - 1),
            random.randint(1, 100)
        ))
    for _ in range(100):
        num = random.randint(1, 10)
        preds = np.random.rand(len(intents))
        preds = np.exp(preds)
        preds /= preds.sum()
        samples.append((
            [random.choice(intents) for _ in range(num)],
            dict(zip(intents, preds)),
            np.random.randint(1, 100, size=num).tolist()
        ))
    for _ in range(100):
        preds = np.random.rand(len(intents))
        preds = np.exp(preds)
        preds /= preds.sum()
        samples.append((
            random.choice(intents),
            list(preds),
            random.randint(1, 100)
        ))
    y_true = []
    y_pred = []
    weights = []
    for true_intent, pred_intent, weight in samples:
        acc.add(true_intent, pred_intent, weight)
        if not isinstance(true_intent, list):
            true_intent = [true_intent]
        if not isinstance(weight, list):
            weight = [weight]
        if isinstance(pred_intent, list):
            pred_intent = np.argmax(pred_intent)
        if isinstance(pred_intent, dict):
            pred_intent = intents.index(max(pred_intent, key=pred_intent.get))
        for target, sample_weight in zip(true_intent, weight):
            y_true.append(intents.index(target))
            y_pred.append(pred_intent)
            weights.append(sample_weight)
        true_f1 = f1_score(y_true, y_pred, sample_weight=weights, labels=range(len(intents)), average='macro', zero_division=0)
        true_micro_f1 = f1_score(y_true, y_pred, sample_weight=weights, labels=range(len(intents)), average='micro', zero_division=0)
        test_f1 = acc.get_metric_by_name("F1").get_f1()
        test_micro_f1 = acc.get_metric_by_name("F1").get_micro_f1()
        assert math.isclose(test_f1, true_f1, rel_tol=1e-5)
        assert math.isclose(test_micro_f1, true_micro_f1, rel_tol=1e-5)


def _test_weight_policy(acc: MultilabelMetricsAccumulator, intents, weight_policy_impl):
    samples = []
    for _ in range(100):
        target = random.sample(intents, random.randint(0, len(intents)))
        weights = np.random.randint(1, 100, size=len(target)).tolist()
        preds = np.random.rand(len(intents)).tolist()
        samples.append((
            target,
            preds,
            weights
        ))

    intent2targets = collections.defaultdict(list)
    intent2predicts = collections.defaultdict(list)
    intent2weights = collections.defaultdict(list)

    for true_intent, pred_intent, weights in samples:
        acc.add(true_intent, pred_intent, weights)

        for intent, pred_value in zip(intents, pred_intent):
            intent2targets[intent].append(1 if intent in true_intent else 0)
            intent2predicts[intent].append(1 if pred_value >= 0.5 else 0)

            if intent in true_intent:
                weight = weights[true_intent.index(intent)]
            else:
                weight = weight_policy_impl(weights)

            intent2weights[intent].append(weight)

        true_macro_f1 = 0.0
        for idx, intent in enumerate(intents):
            intent_f1_score = f1_score(
                intent2targets[intent],
                intent2predicts[intent],
                sample_weight=intent2weights[intent],
                zero_division=0,
            )
            assert math.isclose(intent_f1_score, acc.get_metric_by_name("F1").get_f1(idx), rel_tol=1e-5)
            true_macro_f1 += intent_f1_score

        true_macro_f1 /= len(intents)

        overall_predictions, overall_targets, overall_weights = [], [], []
        for intent in intents:
            overall_predictions += intent2predicts[intent]
            overall_targets += intent2targets[intent]
            overall_weights += intent2weights[intent]

        true_micro_f1 = f1_score(
            overall_targets,
            overall_predictions,
            sample_weight=overall_weights,
            zero_division=0,
        )

        test_macro_f1 = acc.get_metric_by_name("F1").get_f1()
        test_micro_f1 = acc.get_metric_by_name("F1").get_micro_f1()

        assert math.isclose(true_macro_f1, test_macro_f1, rel_tol=1e-5)
        assert math.isclose(true_micro_f1, test_micro_f1, rel_tol=1e-5)


def test_multilabel_existing_sum_missing_weights_policy(intent_label_encoder):
    acc = MultilabelMetricsAccumulator(
        intent_label_encoder,
        weight_policy=MissingWeightPolicy.EXISTING_SUM,
        threshold=0.5,
    )

    _test_weight_policy(acc, intent_label_encoder.intents, sum)


def test_multilabel_set_to_1_missing_weights_policy(intent_label_encoder):
    acc = MultilabelMetricsAccumulator(
        intent_label_encoder,
        weight_policy=MissingWeightPolicy.SET_TO_1,
        threshold=0.5,
    )

    _test_weight_policy(acc, intent_label_encoder.intents, lambda weights: 1)


@pytest.mark.parametrize("weight_policy", list(MissingWeightPolicy))
def test_multilabel_whole_sample_weights(intent_label_encoder, weight_policy):
    acc = MultilabelMetricsAccumulator(
        intent_label_encoder,
        weight_policy=weight_policy,
        threshold=0.5,
    )
    intents = intent_label_encoder.intents
    samples = []
    for _ in range(100):
        target = random.sample(intents, random.randint(0, len(intents)))
        preds = np.random.rand(len(intents)).tolist()
        weight = random.randint(1, 100)
        samples.append((
            target,
            preds,
            weight
        ))

    targets = []
    predicts = []
    weights = []

    mlb = MultiLabelBinarizer()
    mlb.fit([intents])

    for true_intent, pred_intent, sample_weight in samples:
        acc.add(true_intent, pred_intent, sample_weight)

        pred_intent_thresholded = []
        for intent, pred_value in zip(intents, pred_intent):
            if pred_value >= 0.5:
                pred_intent_thresholded.append(intent)

        targets.append(true_intent)
        predicts.append(pred_intent_thresholded)
        weights.append(sample_weight)

        ohe_targets = mlb.transform(targets)
        ohe_predicts = mlb.transform(predicts)

        true_macro_f1 = f1_score(ohe_targets, ohe_predicts, sample_weight=weights, zero_division=0, average='macro')
        true_micro_f1 = f1_score(ohe_targets, ohe_predicts, sample_weight=weights, zero_division=0, average='micro')

        test_macro_f1 = acc.get_metric_by_name("F1").get_f1()
        test_micro_f1 = acc.get_metric_by_name("F1").get_micro_f1()

        assert math.isclose(true_macro_f1, test_macro_f1, rel_tol=1e-5)
        assert math.isclose(true_micro_f1, test_micro_f1, rel_tol=1e-5)
