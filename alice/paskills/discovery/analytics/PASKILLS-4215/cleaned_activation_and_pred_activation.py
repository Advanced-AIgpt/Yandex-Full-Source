# -*- coding: utf-8 -*-

import argparse

from metrics_with_slug import print_metrics
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


def is_positive(serp, query, slug):
    search_results = serp.search(query)

    for search_result in search_results:
        url = search_result['doc']['url']

        if url == "https://dialogs.yandex.ru/store/skills/" + slug:
            return True

        if url.startswith("https://dialogs.yandex.ru/store/categories"):
            return True

        if url.startswith("https://dialogs.yandex.ru/store/compilations"):
            return True

    return False


main()
