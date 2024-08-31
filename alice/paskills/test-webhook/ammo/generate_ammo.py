#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
from pathlib import Path
from datetime import datetime

import json
now = str(int(datetime.timestamp(datetime.now())))

def find_fixtures(path):
    files = []
    for r, d, f in os.walk(path):
        if 'apply_request.json' in f and 'skill.json' not in f:
        #if Path(r).name.startswith('activate'):
            def make(delay):
                return make_dialogovo_ammo(Path(r).name,
                                           read(os.path.join(
                                               r, 'apply_request.json')),
                                           read(os.path.join(
                                               r, 'run_response.json')),
                                           delay)

            print(make(1000))


def read(path):
    with open(path, 'r') as f:
        return json.load(f)


def make_dialogovo_ammo(name, apply_request, run_response, delay):
    case = name + "-" + str(delay)
    client_info = apply_request['base_request']['client_info']
    client_info['app_id'] = 'dialogovo.webhook'
    client_info['app_version'] = case

    headers = "Host: hostname.com\r\n" + \
              "User-Agent: tank\r\n" + \
              "Content-Type: application/json\r\n" + \
              "Accept: application/json\r\n" + \
              "Connection: Close"

    run_ammo = make_ammo("POST", '/megamind/run', headers, case,
                         json.dumps(apply_request).replace('"<TIMESTAMP>"', now))
    if 'apply_arguments' in run_response:
        apply_request['arguments'] = run_response['apply_arguments']
        apply_ammo = make_ammo("POST", '/megamind/apply', headers, case,
                               json.dumps(apply_request).replace('"<TIMESTAMP>"', now))
        res = (
            "%s\r\n"
            "%s"
        ) % (run_ammo, apply_ammo)
    else:
        res = run_ammo

    return res
    #return run_ammo


def make_ammo(method, url, headers, case, body):
    req_template_w_entity_body = (
        "%s %s HTTP/1.1\r\n"
        "%s\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s\r\n"
    )

    req = req_template_w_entity_body % (
        method, url, headers, len(body), body)

    ammo_template = (
        "%d %s\n"
        "%s"
    )

    return ammo_template % (len(req), case, req)


find_fixtures('../../dialogovo/src/test/resources/integration')
