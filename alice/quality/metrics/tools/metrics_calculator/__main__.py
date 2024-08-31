# -*- coding: utf-8 -*-

import json
import click
import yt.wrapper as yt
from alice.quality.metrics.lib.multiclass.multiclass_metrics_accumulator import MulticlassMetricsAccumulator
from alice.quality.metrics.lib.multilabel.input_converter import MissingWeightPolicy
from alice.quality.metrics.lib.multilabel.multilabel_metrics_accumulator import MultilabelMetricsAccumulator
from alice.quality.metrics.lib.binary.binary_metrics_accumulator import BinaryMetricsAccumulator
from alice.quality.metrics.lib.core.metric_type import MetricType
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder


def flatten_report(d):
    ret = {}

    def update_report(key, value):
        if key in ret:
            update_report(f'_{key}', value)
        else:
            ret[key] = value

    for k, v in d.items():
        if not isinstance(v, dict):
            update_report(k, v)
            continue

        v_flat = flatten_report(v)
        for k_inner, v_inner in v_flat.items():
            update_report(f'{k}.{k_inner}', v_inner)

    return ret


def replace_dots(metrics):
    # Pulsar has problems with dots in metric names https://st.yandex-team.ru/PULSAR-582
    return {metric.replace('.', '_'): value for metric, value in metrics.items()}


@click.command()
@click.option('--input-table', required=True)
@click.option('--target-column', required=True)
@click.option('--predict-column', required=True)
@click.option('--output-path', required=True)
@click.option('--cluster', default='hahn')
@click.option('--metrics-type', required=True, type=click.Choice(MetricType))
@click.option('--weight-column', required=False)
@click.option('--label-encode-path', default='')
@click.option('--threshold', default=0.5, type=float)
@click.option('--calculate-auc', default=False, is_flag=True)
@click.option('--missing-weight-policy', default=MissingWeightPolicy.EXISTING_SUM, type=click.Choice(MissingWeightPolicy))
def main(input_table, target_column, predict_column, output_path, cluster, metrics_type, weight_column, label_encode_path, threshold, calculate_auc, missing_weight_policy):
    yt.config.set_proxy(cluster)

    input_samples = yt.read_table(input_table)
    intent_label_encoder = IntentLabelEncoder(label_encode_path, target_column)

    if metrics_type == MetricType.MULTICLASS:
        metric_constructor = lambda: MulticlassMetricsAccumulator(intent_label_encoder, threshold)
    elif metrics_type == MetricType.BINARY:
        metric_constructor = lambda: BinaryMetricsAccumulator(threshold, calculate_auc)
    elif metrics_type == MetricType.MULTILABEL:
        metric_constructor = lambda: MultilabelMetricsAccumulator(intent_label_encoder, weight_policy=missing_weight_policy, threshold=threshold)

    metric = metric_constructor()
    unweighted_metric = metric_constructor()

    for row in input_samples:
        weight = row[weight_column] if weight_column is not None else None
        metric.add(row[target_column], row[predict_column], weight)
        unweighted_metric.add(row[target_column], row[predict_column], weight=None)

    with open(output_path, 'w') as f:
        clf_report = metric.get_classification_report()
        clf_report['Unweighted'] = unweighted_metric.get_classification_report()
        json.dump(replace_dots(flatten_report(clf_report)), f, indent=4, sort_keys=True)
