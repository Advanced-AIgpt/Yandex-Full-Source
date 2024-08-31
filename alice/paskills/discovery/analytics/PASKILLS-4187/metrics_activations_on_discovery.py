# -*- coding: utf-8 -*-

import argparse

from recommender_client import Recommender


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--positives', required=True)
    parser.add_argument('--negatives', required=True)
    args = parser.parse_args()

    file_positives = args.positivesm
    file_negatives = args.negatives
    recommender = Recommender()

    positive_count = 0
    negative_count = 0

    TP = 0
    FP = 0
    TN = 0
    FN = 0

    with open(file_positives) as file:
        for _, line in enumerate(file):
            positive_count += 1
            response_json = recommender.search(line)

            if 'items' in response_json and (len(response_json['items'])) > 0:
                TP += 1
            else:
                FN += 1

    with open(file_negatives) as file:
        for _, line in enumerate(file):
            positive_count += 1
            response_json = recommender.search(line)

            if 'items' in response_json and (len(response_json['items'])) > 0:
                FP += 1
            else:
                TN += 1

    all_count = positive_count + negative_count
    print('positive: ', positive_count)
    print('negative: ', positive_count)

    print('TP: ', TP)
    print('FN: ', FP)
    print('TP: ', TN)
    print('FN: ', FN)

    print('all: ', all_count)

    print('accuracy: ', 100.0 * (TP + TN) / (TP + FP + TN + FN), '%')
    print('precision: ', 100.0 * TP / (TP + FP), '%')
    print('recall: ', 100.0 * TP / (TP + FN), '%')


main()
