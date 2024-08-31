#!/usr/bin/env python
# encoding: utf-8

import sys
import argparse
from scipy.stats import ttest_rel

from utils.nirvana.op_caller import call_as_operation

SCORES = {
    'not_music': 0,
    'not_video': 0,
    'not_found': 0,
    'bad': 0,
    'irrel': 0,
    'part': 0.5,
    'rel_minus': 0.5,
    'good': 1.,
    'rel_plus': 1.
}


def main(prod_data, test_data):
    set_keys = set(prod_data[0].keys()).union(set(test_data[0].keys()))
    prod_results = dict((x, prod_data[0].get(x, 'other')) for x in set_keys)
    test_results = dict((x, test_data[0].get(x, 'other')) for x in set_keys)

    ttest = ttest_rel([SCORES.get(prod_results[k], 0) for k in set_keys],
                      [SCORES.get(test_results[k], 0) for k in set_keys])

    result = {
        'prod_quality': round(sum([SCORES.get(prod_results[k], 0) for k in set_keys]) /
                              float(len(set_keys)), 4),
        'test_quality': round(sum([SCORES.get(test_results[k], 0) for k in set_keys]) /
                              float(len(set_keys)), 4),
        'pvalue': ttest.pvalue,
    }
    return result

if __name__ == '__main__':
    call_as_operation(main,
                      input_spec={'prod_data': {'link_name': 'prod',
                                                'required': True, 'parser': 'json'},
                                  'test_data': {'link_name': 'test',
                                                'required': True, 'parser': 'json'}})
