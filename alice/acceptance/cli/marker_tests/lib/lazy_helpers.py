# coding: utf-8

import json
import logging
import math
import tempfile
import vh


def _get_payload_item(mm_response, name, def_value=None):
    # NOTE(a-square): in case someone might search 'directive', try the longer path first
    item = mm_response.get('directive', {}).get('payload', {}).get(name) or mm_response.get(name)
    return def_value if item is None else item


def _get_new_analytics_info(mm_response):
    store = _get_payload_item(mm_response, 'megamind_analytics_info').get('analytics_info')
    if not store:
        return None

    items = list(store.items())
    assert len(items) == 1
    scenario_name, analytics_info = items[0]
    return scenario_name, analytics_info


def parse_comparable_data(mm_response):
    if not mm_response:
        return (None, None)
    mm_response = json.loads(mm_response)
    response = _get_payload_item(mm_response, 'response', {})

    scenario_name, analytics_info = _get_new_analytics_info(mm_response)
    intent = None
    if analytics_info:
        intent = (
            analytics_info.get('scenario_analytics_info', {}).get('intent') or
            analytics_info.get('semantic_frame', {}).get('name')
        )
    intent = intent or scenario_name
    if not intent:
        logging.error('No analytics_info or no intent in response: %r')

    directives = response.get('directives', [])
    directives_result = ', '.join('{}:{}'.format(d['type'], d['name']) for d in directives)
    return ({
        'intent': intent,
        'directives': directives_result,
    }, mm_response)


def create_diff(reference_data, downloaded_data):
    missed = {}
    diff = {}
    for key in reference_data.keys():
        # If statements order is important
        if reference_data[key].get('xfail'):
            continue
        elif key not in downloaded_data:
            missed[key] = {
                'expected': reference_data[key],
                'downloaded': None,
            }
        elif reference_data[key] == downloaded_data[key]['downloaded']:
            continue
        else:
            diff[key] = {'expected': reference_data[key]}
            diff[key].update(downloaded_data[key])
    return diff, missed


@vh.lazy((vh.File, vh.File, vh.File, bool), reference_requests=vh.File, downloaded_requests=vh.File)
def compare_results(reference_requests, downloaded_requests):
    with open(reference_requests) as f:
        reference_requests = json.load(f)
    with open(downloaded_requests) as f:
        downloaded_requests = json.load(f)

    full_results = {r['request_id']: {'expected': r} for r in reference_requests}
    reference_data = {r['request_id']: r['expected'] for r in reference_requests}

    downloaded_data = {}
    for dr in downloaded_requests:
        req_id = dr['RequestId']
        response = dr['VinsResponse']
        extra = {
            'setrace_url': dr['SetraceUrl'],
            'session_id': dr['SessionId'],
            'session_sequence': dr['SessionSequence'],
        }

        compare_data, response = parse_comparable_data(response)
        if compare_data is None:
            extra['message'] = 'Nothing has been recognised, Vins response missed'
        full_results[req_id]['downloaded'] = response
        downloaded_data[req_id] = {
            'downloaded': compare_data,
            'extra': extra,
        }

    with tempfile.NamedTemporaryFile(delete=False, mode='wt') as full_results_output:
        json.dump(full_results, full_results_output, sort_keys=True, ensure_ascii=False)

    diff, missed = create_diff(reference_data, downloaded_data)
    with tempfile.NamedTemporaryFile(delete=False, mode='wt') as diff_output:
        json.dump(diff, diff_output, sort_keys=True, ensure_ascii=False)
    with tempfile.NamedTemporaryFile(delete=False, mode='wt') as missed_output:
        json.dump(missed, missed_output, sort_keys=True, ensure_ascii=False)

    fails_quota = math.ceil(len(reference_requests) * 0.01)
    fail_flag = False
    if diff or len(missed) > fails_quota:
        fail_flag = True
    return (diff_output.name, missed_output.name, full_results_output.name, fail_flag)


@vh.lazy(bool, fail_flag=bool)
def assert_test_result(fail_flag):
    assert not fail_flag, 'Test results failed, see diff in "compare_results" cube :)'
    return fail_flag
