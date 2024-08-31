#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import asyncio
import os
import re
from aiohttp import web
from pathlib import Path

import json


def init():
    default_port = os.environ.get('QLOUD_HTTP_PORT')
    default_port = int(default_port) if default_port else 8080

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', '-p', dest='port', default=default_port, help='server port')
    parser.add_argument('--dir', '-d', dest='dir', default='../dialogovo/src/test/resources/integration', help='cases dir')

    args = parser.parse_args()

    cases_path = args.dir
    if not cases_path:
        cases_path = os.path.join(os.path.dirname(__file__), '../dialogovo/src/test/resources/integration')
        print('process arg is undefined')

    cases_path = os.path.abspath(cases_path)
    print(f'loading cases from {cases_path}')

    cases = {}

    for r, _, f in os.walk(cases_path):
        if 'webhook_response.json' in f:
            with open(os.path.join(r, 'webhook_response.json'), 'r') as f:
                cases[Path(r).name] = json.load(f)

    print(f'found {len(cases)} cases')

    return cases, args.port


cases, port = init()

routes = web.RouteTableDef()


@routes.post('/{case_str:.*}')
async def webhook(request):
    case_str = request.match_info['case_str']
    print(f'request with case_str = {case_str}')

    case_name, delay_str = None, None

    if case_str:
        case_name, delay_str = (case_str.split('-') + [None, None])[:2]
    else:
        data = await request.json()
        if data and 'meta' in data and 'client_id' in data['meta']:
            client_id_str = data["meta"]["client_id"]
            print(f'client_id={client_id_str}')
            srch = re.search(r'^dialogovo.webhook\/(?P<case_name>[a-z_\d]+)-{0,1}(?P<delay_str>\d*)',
                             client_id_str)

            if srch:
                client_id = srch.groupdict()
                if 'case_name' in client_id:
                    case_name = client_id['case_name']
                if 'delay_str' in client_id:
                    delay_str = client_id['delay_str']

    if not case_name or case_name not in cases:
        print(f'case_name = {case_name}')
        return web.json_response(data={'message': 'case_name not found'}, status=500)

    delay = int(delay_str) if delay_str and delay_str.isdigit() else 0
    print(f'delay_str = {delay_str} so delay is {delay}ms')
    await asyncio.sleep(delay / 1000)

    return web.json_response(data=cases[case_name])


def main():
    app = web.Application()
    app.add_routes(routes)
    web.run_app(app, port=port, host='::')


if __name__ == "__main__":
    main()
