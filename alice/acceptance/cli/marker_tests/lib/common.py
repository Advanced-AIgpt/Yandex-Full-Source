# coding: utf-8

import json
import os
from uuid import uuid4

from library.python import resource


def gen_reqid_for_test(prefix='ffffffff'):
    base = str(uuid4())
    if prefix:
        uuid_str = '-'.join([prefix] + base.split('-')[1:])
    else:
        uuid_str = base
    return uuid_str


def compose_context_request(test_json_request):
    keys = ['context', 'request']
    requests = []
    for k in keys:
        if k in test_json_request:
            r = test_json_request[k]['request'].copy()
            r['expected'] = test_json_request[k]['expected'].copy()
            if not r.get('request_id'):
                r['request_id'] = gen_reqid_for_test()
            requests.append(r)
    req_ids = (r['request_id'] for r in requests)
    session_id = '__'.join(req_ids)
    for i, r in enumerate(requests):
        r['session_id'] = session_id
        r['session_sequence'] = i
        r['reversed_session_sequence'] = len(requests) - 1 - i
    return requests


def all_json_files(dir_):
    result = []
    for file_name in os.listdir(dir_):
        full_name = os.path.join(dir_, file_name)
        if os.path.isdir(full_name):
            result.extend(all_json_files(full_name))
        elif full_name.endswith('.json'):
            result.append(full_name)
    return result


def concat_json_files(files):
    result = []
    for fname in files:
        with open(fname) as f:
            data = json.load(f)
            result.extend(data)
    return result


def concat_json_files_from_resources(files):
    result = []
    for fname in files:
        data = json.loads(resource.find(fname))
        result.extend(data)
    return result


def prepare_experiments(experiments):
    experiments = experiments or '{}'
    try:
        return json.dumps(json.loads(experiments))
    except Exception:
        raise ValueError('Bad experiments json: %s' % experiments)
