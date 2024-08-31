# -*- coding: utf-8 -*-

import argparse

from recommender_client import Recommender


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--file', required=True)
    args = parser.parse_args()

    file_input = args.file
    recommender = Recommender()
    all_count = 0
    positive_count = 0

    with open(file_input) as file:
        for _, line in enumerate(file):
            all_count += 1
            response_json = recommender.search(line)

            if 'items' in response_json:
                if (len(response_json['items'])) > 0:
                    positive_count += 1

        print('all: ', all_count)
        print('positive: ', positive_count)

        print('accuracy: ', 100.0 * positive_count / all_count, '%')


main()
