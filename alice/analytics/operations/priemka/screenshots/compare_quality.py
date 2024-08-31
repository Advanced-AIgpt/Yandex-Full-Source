#!/usr/bin/env python
# encoding: utf-8

from scipy.stats import ttest_rel, ttest_ind
import numpy as np

from utils.nirvana.op_caller import call_as_operation


def get_points(sample):
    return {item['request_id']: item['mark'] for item in sample if item['mark'] is not None}


def compare_quality_related(prod_data, test_data, type='union'):
    prod_results = get_points(prod_data)
    test_results = get_points(test_data)

    if type == 'union':
        set_keys = set(prod_results).union(set(test_results))
    elif type == 'intersection':
        set_keys = set(prod_results).intersection(set(test_results))
    prod_values = [prod_results.get(k, 0) for k in set_keys]
    test_values = [test_results.get(k, 0) for k in set_keys]
    ttest = ttest_rel(prod_values, test_values)
    pvalue = ttest.pvalue
    if np.isnan(pvalue):
        if prod_values == test_values:
            pvalue = 1.0
        else:
            pvalue = None
    result = {
        'prod_quality': round(sum(prod_values) /
                              float(len(set_keys)), 4),
        'test_quality': round(sum(test_values) /
                              float(len(set_keys)), 4),
        'pvalue': pvalue
    }
    return result


def compare_quality_independent(prod_data, test_data):
    prod_results = get_points(prod_data).values()
    test_results = get_points(test_data).values()

    ttest = ttest_ind(prod_results, test_results, equal_var=False)
    result = {
        'prod_quality': round(sum(prod_results) / float(len(prod_results)), 4),
        'test_quality': round(sum(test_results) / float(len(test_results)), 4),
        'pvalue': None if np.isnan(ttest.pvalue) else ttest.pvalue
    }
    return result


if __name__ == '__main__':
    call_as_operation(compare_quality_related,
                      input_spec={'prod_data': {'link_name': 'prod',
                                                'required': True, 'parser': 'json'},
                                  'test_data': {'link_name': 'test',
                                                'required': True, 'parser': 'json'}})
