#!/usr/bin/env python
# encoding: utf-8

from nile.api.v1 import clusters
from compare_quality_all_metrics import main as compare_quality_all_metrics
from utils.nirvana.op_caller import call_as_operation

from collections import defaultdict

def mr_read_json(table):
    cluster = clusters.yt.Hahn()
    job = cluster.job()

    return (record.to_dict() for record in job.table(table).read())

def main(prod_data, test_data):
    to_compare = defaultdict(dict)
    for note in prod_data:
        to_compare[note['dataset_name']]['prod'] = note['table']

    for note in test_data:
        to_compare[note['dataset_name']]['test'] = note['table']

    result = []
    for basket, tables in to_compare.items():
        pvalues = compare_quality_all_metrics(mr_read_json(tables['prod']), mr_read_json(tables['test']))
        for pvalue in pvalues:
            pvalue['basket'] = basket
            result.append(pvalue)

    return result

if __name__ == '__main__':
    call_as_operation(main,
                      input_spec={'prod_data': {'link_name': 'prod',
                                                'required': True, 'parser': 'json'},
                                  'test_data': {'link_name': 'test',
                                                'required': True, 'parser': 'json'}
                                  })
