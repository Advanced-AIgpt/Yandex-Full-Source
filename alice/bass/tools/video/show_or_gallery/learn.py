#!/usr/bin/env python3
import argparse

from catboost import Pool
from common import *


def go(args):
    X, y, w, _ = load_data(args.input_csv, args.weight_samples)

    pool = Pool(X, y, weight=w)
    clf = make_classifier(verbose=True, random_state=args.seed)
    clf.fit(pool)
    clf.save_model(args.output_cbm)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--input-csv', type=str, default=DEFAULT_DATA_PATH, help='path to train data')
    parser.add_argument('--output-cbm', type=str, default=DEFAULT_MODEL_PATH, help='path for trained model')
    parser.add_argument('--seed', type=int, default=DEFAULT_SEED, help='seed')
    parser.add_argument('--weight-samples',
                        action='store_true',
                        default=DEFAULT_WEIGHT_SAMPLES,
                        help='use sample weights in learning')
    args = parser.parse_args()
    go(args)
