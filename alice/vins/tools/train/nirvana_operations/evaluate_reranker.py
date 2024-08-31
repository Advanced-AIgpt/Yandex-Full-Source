# -*- coding: utf-8 -*-

import argparse
import numpy as np
import pandas as pd

from itertools import izip
from collections import Counter
from sklearn.metrics import precision_recall_fscore_support, accuracy_score


def _iterate_group_indices(group_ids):
    cur_group_id, cur_group_begin = 0, 0
    for ind, group_id in enumerate(group_ids):
        if group_id != cur_group_id:
            yield cur_group_begin, ind
            cur_group_id, cur_group_begin = group_id, ind
    yield cur_group_begin, len(group_ids)


def _collect_answers(scores, group_ids, variants, y_true):
    y_pred, left_items, correct_intent_scores, predicted_intent_scores, hypotheses = [], [], [], [], []

    for sample_ind, (group, true_intent) in enumerate(izip(_iterate_group_indices(group_ids), y_true)):
        group_begin, group_end = group
        group_variants = variants[group_begin: group_end]
        group_scores = scores[group_begin: group_end]

        if len(group_variants) == 0:
            continue

        correct_intent_indices = (group_variants == true_intent).nonzero()[0]
        if len(correct_intent_indices) > 0:
            correct_intent_scores.append(group_scores[correct_intent_indices[0]])
        else:
            correct_intent_scores.append(0.)

        best_variants = group_variants[group_scores == group_scores.max()]
        predicted_intent = best_variants[0]
        if len(best_variants) > 1:
            for variant_intent in best_variants:
                if variant_intent != true_intent:
                    predicted_intent = variant_intent
                    break
        y_pred.append(predicted_intent)
        predicted_intent_scores.append(group_scores.max())

        left_items.append(sample_ind)
        hypotheses.append(','.join(group_variants))

    assert len(y_pred) == len(left_items) == len(correct_intent_scores) == len(predicted_intent_scores)
    return y_pred, left_items, np.array(correct_intent_scores), np.array(predicted_intent_scores), hypotheses


def _eval_predictions(y_pred, y_true, output_file):
    errors_stats = Counter()
    for true_intent, predicted_intent in izip(y_true, y_pred):
        if true_intent != predicted_intent:
            errors_stats[(true_intent, predicted_intent)] += 1

    output_file.write('Most common errors:\n')
    for ((true_intent, predicted_intent), count) in errors_stats.most_common(10):
        output_file.write('{} -> {} - {} times\n'.format(true_intent, predicted_intent, count))
    output_file.write('\n')

    labels = sorted(set(y_true) - {'personal_assistant.scenarios.other'})

    precisions, recalls, f1_scores, supports = precision_recall_fscore_support(
        y_true, y_pred, labels=labels, average=None
    )
    total_precision, total_recall, total_f1_score, _ = precision_recall_fscore_support(
        y_true, y_pred, labels=labels, average='weighted'
    )

    report = pd.DataFrame(
        data={
            'precision': list(precisions) + [total_precision],
            'recall': list(recalls) + [total_recall],
            'f1-score': list(f1_scores) + [total_f1_score],
            'support': list(supports) + [supports.sum()]
        },
        index=list(labels) + ['total'],
        columns=['precision', 'recall', 'f1-score', 'support']
    )
    return report, accuracy_score(y_true, y_pred)


def _evaluate(data_path, preds_path, report_path, full_results_path):
    data = np.genfromtxt(data_path, dtype=None, delimiter='\t', usecols=range(6))

    group_ids = np.array([row[1] for row in data], dtype=np.int)
    sample_labels = np.array([row[3] for row in data])
    _, indices = np.unique(group_ids, return_index=True)
    sample_labels = sample_labels[indices]
    ranking_variants = np.array([row[4] for row in data])

    with open(report_path, 'w') as output_file:
        output_file.write('Baseline:\n')

        baseline_scores = np.array([row[5] for row in data])
        (baseline_labels, left_items, baseline_correct_intent_scores,
         baseline_predicted_intent_scores, _) = _collect_answers(
            baseline_scores, group_ids, ranking_variants, sample_labels
        )
        baseline_report, baseline_accuracy = _eval_predictions(baseline_labels, sample_labels[left_items], output_file)

        output_file.write('Reranker:\n')
        reranker_scores = pd.read_csv(preds_path, sep='\t')['RawFormulaVal'].values
        (reranker_labels, left_items, reranker_correct_intent_scores,
         reranker_predicted_intent_scores, hypotheses) = _collect_answers(
            reranker_scores, group_ids, ranking_variants, sample_labels
        )
        reranker_report, reranker_accuracy = _eval_predictions(
            reranker_labels, sample_labels[left_items], output_file
        )

        report = pd.concat([baseline_report[['precision', 'recall', 'f1-score']], reranker_report], axis=1)
        report['delta'] = reranker_report['f1-score'].values - baseline_report['f1-score'].values

        output_file.write('Report:\n{}\n\n'.format(report))
        output_file.write('Accuracy: {:.2%} -> {:.2%}\n'.format(baseline_accuracy, reranker_accuracy))

    texts = np.array([row[2] for row in data])[indices]
    assert len(left_items) == len(baseline_labels) == len(reranker_labels)
    full_results = pd.DataFrame(
        {
            'text': texts[left_items],
            'correct_intent': sample_labels[left_items],
            'scenarios_predicted_intent': baseline_labels,
            'reranker_predicted_intent': reranker_labels,
            'scenarios_correct_intent_score': baseline_correct_intent_scores,
            'scenarios_predicted_intent_score': baseline_predicted_intent_scores,
            'reranker_correct_intent_score': reranker_correct_intent_scores,
            'reranker_predicted_intent_score': reranker_predicted_intent_scores,
            'hypotheses': hypotheses
        },
        columns=['text', 'correct_intent', 'scenarios_predicted_intent', 'reranker_predicted_intent',
                 'scenarios_correct_intent_score', 'scenarios_predicted_intent_score',
                 'reranker_correct_intent_score', 'reranker_predicted_intent_score', 'hypotheses']
    )
    full_results.to_csv(full_results_path, sep='\t', encoding='utf8', index=False)


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--data-path', type=str, required=True)
    parser.add_argument('--preds-path', type=str, required=True)
    parser.add_argument('--report-path', type=str, required=False)
    parser.add_argument('--full-results-path', type=str, required=False)

    args = parser.parse_args()

    pd.set_option('display.width', 1000)
    pd.set_option('display.max_rows', 1000)
    pd.set_option('display.max_colwidth', 100)
    pd.set_option('precision', 4)

    _evaluate(args.data_path, args.preds_path, args.report_path, args.full_results_path)


if __name__ == '__main__':
    main()
