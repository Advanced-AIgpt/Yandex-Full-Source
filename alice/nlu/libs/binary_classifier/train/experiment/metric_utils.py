# coding: utf-8

import os
import numpy as np

from sklearn.metrics import log_loss, precision_recall_curve, roc_auc_score, average_precision_score


def _calculate_f1_score(precision, recall):
    return 2 * (precision * recall) / (precision + recall) if precision + recall != 0. else 0.


def compute_metrics(valid_predictions, valid_labels, output_dir, output_prefix):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    valid_labels, valid_predictions = valid_labels.astype(np.float64), valid_predictions.astype(np.float64)

    precisions, recalls, thresholds = precision_recall_curve(valid_labels, valid_predictions)

    fig, ax = plt.subplots()
    ax.step(recalls, precisions, color='b', alpha=0.2, where='post')
    ax.fill_between(recalls, precisions, alpha=0.2, color='b', step='post')

    ax.set_xlabel('Recall')
    ax.set_ylabel('Precision')
    ax.set_ylim([0.0, 1.05])
    ax.set_xlim([0.0, 1.0])
    fig.savefig(os.path.join(output_dir, output_prefix + '_precision_recall_curve.png'))
    plt.close(fig)

    f1_scores = [
        (_calculate_f1_score(precision, recall), threshold)
        for precision, recall, threshold in zip(precisions, recalls, thresholds)
    ]
    best_f1_score, best_threshold = max(f1_scores, key=lambda pair: pair[0])

    logloss = log_loss(valid_labels, valid_predictions)
    auc_score = average_precision_score(valid_labels, valid_predictions)

    with open(os.path.join(output_dir, output_prefix + '_metrics.txt'), 'w') as f:
        f.write('Log loss = {:.4f}, AUC = {:.2%}, F1-score = {:.2%}, threshold = {:.3f}'.format(
            logloss, auc_score, best_f1_score, best_threshold
        ))
