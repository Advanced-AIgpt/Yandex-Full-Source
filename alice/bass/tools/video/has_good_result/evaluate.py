#!/usr/bin/env python3

import argparse
import json
import math
import numpy as np
import pandas as pd

from common import *

from catboost import CatBoostClassifier, Pool
from sklearn.metrics import roc_auc_score, roc_curve
from sklearn.model_selection import KFold


DEFAULT_DATA = 'data.csv'
DEFAULT_SEED = 42


def go(args):
    Xs, ys, ws, qs = load_data(args.input_csv, args.weight_samples)
    features = Xs.columns.values

    kf = KFold(n_splits=5, shuffle=True, random_state=args.seed)

    roc_auc_scores = []
    false_positive_rates = []
    thresholds = []

    for train_index, test_index in kf.split(Xs):
        X_train, y_train, w_train = Xs.iloc[train_index], ys[train_index], ws[train_index]
        X_test, y_test, w_test = Xs.iloc[test_index], ys[test_index], ws[test_index]

        train_pool = Pool(X_train, y_train, weight=w_train)
        test_pool = Pool(X_test, y_test, weight=w_test)

        clf = make_classifier(verbose=False, random_state=args.seed)
        clf.fit(train_pool)

        y_prob = clf.predict_proba(test_pool)[:, 1]

        roc_auc_scores.append(roc_auc_score(y_test, y_prob, sample_weight=w_test))

        fprs, tprs, thrs = roc_curve(y_test, y_prob, sample_weight=w_test)

        for i in range(len(thrs)):
            if tprs[i] >= args.min_tpr:
                false_positive_rates.append(fprs[i])
                thresholds.append(thrs[i])
                break

        if args.show_errors:
            y_pred = clf.predict(test_pool)

            mismatch = X_test[y_pred != y_test].index
            queries = qs[mismatch]
            targets = y_test[mismatch]

            feature_importances = clf.get_feature_importance(data=train_pool)
            for f, i in sorted(zip(features, feature_importances), key=lambda v: v[1], reverse=True):
                print('{}: {}'.format(f, i))
            for (q, t) in zip(queries, targets):
                print(q, t)

    print('Mean ROC-AUC: {:.2f}'.format(np.mean(roc_auc_scores)))
    print('Mean FPR is: {:.2f}'.format(np.mean(false_positive_rates)))
    print('Mean threshold is: {:.6f}'.format(np.mean(thresholds)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--input-csv', type=str, default=DEFAULT_DATA, help='path to test/train data')
    parser.add_argument('--seed', type=int, default=DEFAULT_SEED, help='seed')
    parser.add_argument('--show-errors', action='store_true', default=False, help='show errors')
    parser.add_argument('--weight-samples',
                        action='store_true',
                        default=DEFAULT_WEIGHT_SAMPLES,
                        help='use sample weights in learning')
    parser.add_argument('--min-tpr', type=float, default=DEFAULT_TPR,
                        help='minimum true-positive-rate (recall), used to select decision boundary')

    args = parser.parse_args()
    go(args)
