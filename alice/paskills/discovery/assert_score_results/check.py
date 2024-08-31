#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import argparse
import sys
import json


def main():
    parser = argparse.ArgumentParser(prog='Score checker')
    parser.add_argument('--score-results')
    parser.add_argument('--expected-acc1-min', type=float)
    parser.add_argument('--expected-acc5-min', type=float)
    parser.add_argument('--expected-acc10-min', type=float)
    parser.add_argument('--expected-found-percent-min', type=float)
    parser.add_argument('--expected-found-percent-max', type=float)
    parser.add_argument('--expected-not-found-max', type=int)
    parser.add_argument('--expected-not-found1-max', type=int)
    parser.add_argument('--expected-not-found5-max', type=int)
    parser.add_argument('--expected-not-found10-max', type=int)
    args = parser.parse_args()

    if args.score_results:
        with open(args.score_results) as score_results_file:
            score_results = json.load(score_results_file)
    else:
        score_results = json.load(sys.stdin)

    acc1 = float(score_results.get('acc1', 0))
    acc5 = float(score_results.get('acc5', 0))
    acc10 = float(score_results.get('acc10', 0))
    found_percent = float(score_results.get('found_percent', 0))
    not_found = int(score_results.get('not_found', 0))
    not_found1 = int(score_results.get('not_found1', 0))
    not_found5 = int(score_results.get('not_found5', 0))
    not_found10 = int(score_results.get('not_found10', 0))

    if args.expected_acc1_min is not None:
        assert (args.expected_acc1_min <= acc1)
    if args.expected_acc5_min is not None:
        assert (args.expected_acc5_min <= acc5)
    if args.expected_acc10_min is not None:
        assert (args.expected_acc10_min <= acc10)
    if args.expected_found_percent_min is not None:
        assert (args.expected_found_percent_min <= found_percent)
    if args.expected_found_percent_max is not None:
        assert (args.expected_found_percent_max >= found_percent)
    if args.expected_not_found_max is not None:
        assert (args.expected_not_found_max >= not_found)
    if args.expected_not_found1_max is not None:
        assert (args.expected_not_found1_max >= not_found1)
    if args.expected_not_found5_max is not None:
        assert (args.expected_not_found5_max >= not_found5)
    if args.expected_not_found10_max is not None:
        assert (args.expected_not_found10_max >= not_found10)


if __name__ == "__main__":
    main()
