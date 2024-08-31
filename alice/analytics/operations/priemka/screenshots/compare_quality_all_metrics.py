#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from compare_quality import compare_quality_related

from collections import defaultdict


def fill_raw_data(raw_data, data, data_name):
    for item in data:
        for key, value in item.items():
            if key.startswith('metric_'):
                raw_data[key][data_name][item.get('req_id') or item.get('request_id')] = value
                if 'integral' in key:
                    raw_data[key + "_withnone"][data_name][item.get('req_id') or item.get('request_id')] = value
    return raw_data


def main(prod_data, test_data, null_constant=0.5):
    raw_data = defaultdict(lambda: {'prod_data': {}, 'test_data': {}})
    raw_data = fill_raw_data(raw_data, prod_data, 'prod_data')
    raw_data = fill_raw_data(raw_data, test_data, 'test_data')

    # compute all metrics (with filtered None values)
    quality = {}
    for metric_name, data in raw_data.items():
        if 'withnone' in metric_name:
            prod_marks = [mark or 0.0 for mark in data['prod_data'].values()]
            test_marks = [mark or 0.0 for mark in data['test_data'].values()]
        else:
            prod_marks = filter(lambda mark: mark is not None, data['prod_data'].values())
            test_marks = filter(lambda mark: mark is not None, data['test_data'].values())
        quality[metric_name] = {
            'prod_quality': round(sum(prod_marks) / float(len(prod_marks) or 1.), 4),
            'test_quality': round(sum(test_marks) / float(len(test_marks) or 1.), 4)
        }

    # filter requests where at least one of marks (prod_mark and test_mark) is not None
    # replace left None marks with a constant value "null_constant"
    to_test = defaultdict(lambda: {'prod_data': [], 'test_data': []})
    for metric_name, data in raw_data.items():
        for request_id in set(data['prod_data'].keys()).union(data['test_data'].keys()):
            if request_id in data['prod_data']:
                prod_mark = data['prod_data'][request_id]
            else:
                prod_mark = None
            if request_id in data['test_data']:
                test_mark = data['test_data'][request_id]
            else:
                test_mark = None

            if prod_mark is None and test_mark is None:
                continue
            if 'withnone' in metric_name:
                null_replacement = 0.0
            else:
                null_replacement = null_constant
            prod_mark = prod_mark if prod_mark is not None else null_replacement
            test_mark = test_mark if test_mark is not None else null_replacement
            to_test[metric_name]['prod_data'].append({'request_id': request_id, 'mark': prod_mark})
            to_test[metric_name]['test_data'].append({'request_id': request_id, 'mark': test_mark})

    # get pvalues
    results = []
    for metric_name, data in to_test.items():
        result = compare_quality_related(data['prod_data'], data['test_data'])
        results.append(dict(quality[metric_name], pvalue=result['pvalue'], metric_name=metric_name.split('_', 1)[1]))

    return results


if __name__ == '__main__':
    call_as_operation(main,
                      input_spec={'prod_data': {'link_name': 'prod',
                                                'required': True, 'parser': 'json'},
                                  'test_data': {'link_name': 'test',
                                                'required': True, 'parser': 'json'}})
