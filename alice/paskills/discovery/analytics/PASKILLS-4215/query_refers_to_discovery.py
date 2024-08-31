# -*- coding: utf-8 -*-

import argparse

from metrics import print_metrics
from serp_client import SERP
from functools import partial


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--key', required=True)
    parser.add_argument('--positives', required=True)
    parser.add_argument('--negatives', required=True)

    args = parser.parse_args()

    file_positives = args.positives
    file_negatives = args.negatives

    serp = SERP(args.key)

    # search_results = serp.search("навык заказа пиццы")

    print_metrics(file_positives, file_negatives, partial(is_positive, serp))


def is_positive(serp, query):
    search_results = serp.search(query)

    for search_result in search_results:
        url = search_result['doc']['url']
        domain = search_result['doc']['domain']

        if domain == "dialogs.yandex.ru":
            return True

    return False


main()
