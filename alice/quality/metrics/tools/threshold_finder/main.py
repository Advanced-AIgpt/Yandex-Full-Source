from collections import defaultdict
import json
from typing import Dict, Iterable, List, Optional

import click

import yt.wrapper

from alice.quality.metrics.lib.binary.input_converter import BinaryClassificationInput, BinaryClassificationResult
from alice.quality.metrics.lib.core.metric_type import MetricType
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder
from alice.quality.metrics.lib.multilabel.input_converter import MissingWeightPolicy, MultilabelClassificationInput
from alice.quality.metrics.lib.thresholds.plots import combine_plots_in_html, plot_curves_with_optimums
from alice.quality.metrics.lib.thresholds.threshold_finder import build_curves_and_find_thresholds


_BINARY_CLASSIFICATION_CLASS_NAME = 'binary classification'


def read_binary_classification_samples(
    input_rows: Iterable[dict],
    predict_column: str,
    target_column: str,
    weight_column: Optional[str],
) -> Dict[str, List[BinaryClassificationResult]]:

    binary_samples = []

    for row in input_rows:
        target = row[target_column]
        weight = row[weight_column] if weight_column is not None else None
        prediction = row[predict_column]

        clf_input = BinaryClassificationInput(target, weight, prediction)
        binary_samples.extend(clf_input.convert_to_result())

    if not binary_samples:
        raise ValueError('empty dataset provided')

    return {_BINARY_CLASSIFICATION_CLASS_NAME: binary_samples}


def read_multilabel_classification_samples(
    input_rows: Iterable[dict],
    predict_column: str,
    target_column: str,
    weight_column: Optional[str],
    label_encoder: IntentLabelEncoder,
    missing_weight_policy: MissingWeightPolicy,
) -> Dict[str, List[BinaryClassificationResult]]:

    class_label2binary_samples = defaultdict(list)

    for row in input_rows:
        target = row[target_column]
        weight = row[weight_column] if weight_column is not None else None
        prediction = row[predict_column]

        clf_input = MultilabelClassificationInput(target, weight, prediction)
        for multilabel_result in clf_input.convert_to_result(label_encoder):
            for class_label, binary_result in enumerate(multilabel_result.convert_to_binary(missing_weight_policy)):
                class_label2binary_samples[class_label].append(binary_result)

    if not class_label2binary_samples:
        raise ValueError('empty dataset provided')

    class2binary_samples = {}
    for class_ in label_encoder.intents:
        class_label = label_encoder.encode(class_)
        class2binary_samples[class_] = class_label2binary_samples[class_label]

    return class2binary_samples


@click.command()
@click.option('--cluster', default='hahn')
@click.option('--input-table', required=True)
@click.option('--target-column', required=True)
@click.option('--predict-column', required=True)
@click.option('--thresholds-output-path', required=True, type=click.Path(exists=False))
@click.option('--html-plots-output-path', required=True, type=click.Path(exists=False))
@click.option('--classification-type', required=True, type=click.Choice([MetricType.BINARY, MetricType.MULTILABEL]))
@click.option('--weight-column', required=False)
@click.option('--label-encode-path', required=False, type=click.Path(exists=True, dir_okay=False))
@click.option('--missing-weight-policy', default=MissingWeightPolicy.EXISTING_SUM, type=click.Choice(MissingWeightPolicy))
def main(
    cluster, input_table,
    target_column, predict_column,
    thresholds_output_path, html_plots_output_path, classification_type,
    weight_column, label_encode_path, missing_weight_policy,
):
    client = yt.wrapper.YtClient(proxy=cluster)
    rows = yt.wrapper.read_table(input_table, client=client)

    if classification_type == MetricType.BINARY:
        class2samples = read_binary_classification_samples(
            rows, predict_column, target_column, weight_column,
        )
    elif classification_type == MetricType.MULTILABEL:
        if label_encode_path is None:
            raise ValueError(f'label encoder is necessary to process {classification_type}')

        label_encoder = IntentLabelEncoder(label_encode_path, target_column)

        class2samples = read_multilabel_classification_samples(
            rows, predict_column, target_column, weight_column, label_encoder, missing_weight_policy,
        )
    else:
        raise ValueError(f'{classification_type} is not supported')

    optimum2thresholds = defaultdict(dict)
    class2plots = {}

    for class_, samples in class2samples.items():
        roc_curve, precision_recall_curve, optimums = build_curves_and_find_thresholds(samples)

        for optimum_type, optimum in optimums.items():
            optimum2thresholds[optimum_type][class_] = optimum.optimal_threshold

        class2plots[class_] = plot_curves_with_optimums(roc_curve, precision_recall_curve, optimums)

    if classification_type == MetricType.BINARY:
        # unwrap single binary classification threshold
        optimum2thresholds = {
            optimum_type: class_thresholds[_BINARY_CLASSIFICATION_CLASS_NAME]
            for optimum_type, class_thresholds in optimum2thresholds.items()
        }

    with open(thresholds_output_path, 'w', encoding='utf-8') as f:
        json.dump(optimum2thresholds, f, sort_keys=True, ensure_ascii=False, indent=4)

    with open(html_plots_output_path, 'w', encoding='utf-8') as f:
        f.write(combine_plots_in_html(class2plots))
