#!/usr/bin/env python
# encoding: utf-8
import re
import time
import json
from collections import defaultdict

from nirvana.job_context import context


def divide_by_types(assignments, validation_key_rx='^eval_'):
    match = re.compile(validation_key_rx).search
    honeypots = []
    validation = []
    actual = []
    for a in assignments:
        if a['knownSolutions'] is not None:
            honeypots.append(a)
        elif match(a['inputValues']['key']):
            validation.append(a)
        else:
            actual.append(a)
    return honeypots, validation, actual


def milliseconds_now():
    return int(time.time() * 1000)


def project_ds_fields(assignments):
    submit_ts = str(milliseconds_now())
    return [{"inputValues" : a['inputValues'],
             "outputValues": a['outputValues'],
             "submitTs": submit_ts,
             "algorithm": "DS",
             "probability": a['probability']['result'],
             }
            for a in assignments]


def filter_confident(ds_actual, confidence_level=0.9):
    return [assign for assign in ds_actual if assign['probability'] > confidence_level]


def prepare_raw_result(raw_actual, ds_confident):
    # код, в основном, стащен из одноимённого кубика
    stats = defaultdict(lambda: defaultdict(int))
    for task in raw_actual:
        stats[task['inputValues']['key']][task['outputValues']['result'].encode('utf8')] += 1

    submit_ts = str(milliseconds_now())
    all_votes = {}
    for task in raw_actual:
        inputval = {k.encode('utf8'): (v.encode('utf8') if v else "")
                    for k, v in task['inputValues'].iteritems()}

        all_votes[task['inputValues']['key']] = {
            "inputValues": inputval,
            'outputValues': {
                "result": stats[task['inputValues']['key']]
            },
            'algorithm': 'raw',
            'submitTs': submit_ts,
            'probability': 1.0
        }

    confident_keys = set(x['inputValues']['key'] for x in ds_confident)
    confident_votes = [val
                       for key, val in all_votes.iteritems()
                       if key in confident_keys]

    return all_votes.values(), confident_votes


def process(raw_assignments, ds_assignments, validation_key_rx, confidence_level):
    out = {}  # {"output_name": object_to_store}
    _raw_honeypots, _raw_validation, raw_actual = divide_by_types(raw_assignments, validation_key_rx)

    _ds_honeypots, ds_validation, ds_actual = divide_by_types(ds_assignments, validation_key_rx)
    out['ds_validation'] = project_ds_fields(ds_validation)
    out['ds_actual'] = project_ds_fields(ds_actual)

    out['ds_confident'] = filter_confident(out['ds_actual'], confidence_level)
    out['all_votes'], out['confident_votes'] = prepare_raw_result(raw_actual, out['ds_confident'])
    return out


def main():
    ctx = context()
    inputs = ctx.get_inputs()
    outputs = ctx.get_outputs()
    params = ctx.get_parameters()

    results = process(json.load(open(inputs.get('raw_assignments')), encoding='utf-8'),
                      json.load(open(inputs.get('ds_assignments')), encoding='utf-8'),
                      params['validation_key_rx'],
                      params['confidence_level'])

    for name, result in results.iteritems():
        with open(outputs.get(name), 'w') as out:
            json.dump(result, out, indent=2, encoding='utf-8')


if __name__ == '__main__':
    main()
