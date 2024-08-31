#!/usr/bin/env python3
# coding: utf-8
#
import argparse
import datetime
import json
import logging
import os
import random
import sys
import time

logger = logging.getLogger(__name__)


def finish_time(r):
    return r['start_test'] + r['test_duration']


def _quantile(arr, q, sort=True):
    if sort:
        arr.sort()
    qidx = int(len(arr) * q + 1)
    if qidx >= len(arr):  # handle tail overrun
        return (arr[-1] if arr else 0)
    return arr[qidx]


def _q_round(arr, q, round_=3):
    return round(_quantile(arr, q, sort=False), round_)


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()

    parser.add_argument('--crop-tail', metavar='NUM', type=int, default=0,
                        help='ignore NUM latest sessions')

    parser.add_argument('--sess-result', metavar='FILENAME', default='stress_out.jsonl',
                        help='saved test sessions results (JSONL file) (sess per line) default=stress.out')
    parser.add_argument('--stat-result', metavar='FILENAME', default='stress_stat.out',
                        help='save here calculated statistics default=stress_stat.out')

    context = parser.parse_args()

    # load stress tests results
    results = []
    with open(context.sess_result) as f:
        for line in f:
            results.append(json.loads(line))

    stats = {
        'input_sessions': len(results),
    }

    # detect test type
    test_voice_input = False
    for r in results:
        if not r['errors'] and r.get('asr_result'):
            test_voice_input = True
            break

    # sort by session end time
    results = sorted(results, key=finish_time)
    if context.crop_tail:
        results = results[:-context.crop_tail]
    stats['used_sessions'] = len(results)

    # calculate statistics
    last_result = results[-1]
    start_time = min(r['start_test'] for r in results)
    sessions_duration = (finish_time(last_result) - start_time)
    stats['rps'] = round(len(results)/sessions_duration, 2)

    ok_results = [result for result in results if not result['errors']]
    stats['rps_ok'] = round(len(ok_results)/sessions_duration, 2)

    stats['ok_sessions'] = len(ok_results)
    stats['perc_ok_sessions'] = round(100. * len(ok_results) / len(results), 2)
    fail_results = [result for result in results if result['errors']]
    errs = {}
    for errors in (result['errors'] for result in fail_results):
        s_err = ' '.join(errors)
        s_err = s_err[:80]
        if s_err in errs:
            errs[s_err] += 1
        else:
            errs[s_err] = 1

    if test_voice_input:
        asr_results = [r['asr_result'] for r in ok_results]
        asr_results.sort()
        stats['asr_result_q99'] = _q_round(asr_results, 0.99)
        stats['asr_result_q95'] = _q_round(asr_results, 0.95)
        stats['asr_result_q90'] = _q_round(asr_results, 0.90)
        stats['asr_result_q80'] = _q_round(asr_results, 0.80)
        stats['asr_result_q50'] = _q_round(asr_results, 0.50)

    if "vins_response" in ok_results[-1]:
        vins_results = [r['vins_response'] for r in ok_results]
        vins_results.sort()
        stats['vins_response_q99'] = _q_round(vins_results, 0.99)
        stats['vins_response_q95'] = _q_round(vins_results, 0.95)
        stats['vins_response_q90'] = _q_round(vins_results, 0.90)
        stats['vins_response_q80'] = _q_round(vins_results, 0.80)
        stats['vins_response_q50'] = _q_round(vins_results, 0.50)

    test_duration = [r['test_duration'] for r in results]
    test_duration.sort()
    stats['test_duration_q99'] = _q_round(test_duration, 0.99)
    stats['test_duration_q95'] = _q_round(test_duration, 0.95)
    stats['test_duration_q90'] = _q_round(test_duration, 0.90)
    stats['test_duration_q80'] = _q_round(test_duration, 0.80)
    stats['test_duration_q50'] = _q_round(test_duration, 0.50)

    stats['errors'] = errs

    # save statistics
    with open(context.stat_result, 'w') as f:
        json.dump(stats, f, indent=4, sort_keys=True)

    return 0


if __name__ == '__main__':
    sys.exit(main())
