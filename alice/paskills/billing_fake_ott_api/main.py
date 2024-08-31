import argparse
import asyncio
import logging
import logging.config
import json
import os
import sys
import random

import library.python.resource as resourcelib
from aiohttp import web


DEFAULT_PORT = int(os.environ.get('QLOUD_HTTP_PORT', '8080'))
RESPONSES = {}
DELAYS = {}


logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)


class Delay:
    def get_delay(self):
        raise NotImplementedError()


class FixedDelay(Delay):

    def __init__(self, value):
        self.value = value

    def get_delay(self):
        return self.value


class RandomDelay(Delay):

    def __init__(self, mean, std):
        self.mean = mean
        self.std = std

    def get_delay(self):
        return random.normalvariate(self.mean, self.std)


async def handle_request(request):
    handle = request.match_info.get('handle')
    handle_key = 'content_' + handle
    # todo: sleep
    if handle_key in RESPONSES:
        delay = DELAYS[handle_key]
        await asyncio.sleep(delay.get_delay() / 1000.)
        response = RESPONSES[handle_key]
        return web.Response(text=response, headers={
            'Content-Type': 'application/json',
        })
    else:
        raise web.HTTPNotFound()


async def handle_ping(request):
    return web.Response(text='pong')


def setup_responses():
    for key in resourcelib.iterkeys():
        if key.endswith('.json'):
            filename = os.path.splitext(key)[0]
            response = resourcelib.find(key)
            # validate json
            json.loads(response)
            RESPONSES[filename] = response.decode('utf-8')


def get_delay(delay_json):
    if delay_json['type'] == 'fixed':
        return FixedDelay(delay_json['value'])
    elif delay_json['type'] == 'random':
        return RandomDelay(delay_json['mean'], delay_json['std'])


def read_config(config_path):
    with open(config_path) as f:
        config = json.loads(f.read())
        for handle in ['content_available', 'content_options']:
            DELAYS[handle] = get_delay(config[handle]['delay'])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', required=True)
    parser.add_argument('--port', type=int, default=DEFAULT_PORT)
    args = parser.parse_args()
    setup_responses()
    read_config(args.config)
    app = web.Application()
    app.add_routes([
        web.get('/content/{id}/{handle}', handle_request),
        web.get('/ping', handle_ping),
    ])
    logger.info(f'Starting at port ${args.port}')
    web.run_app(
        app,
        host='::',
        port=args.port,
        access_log_format='%a %t "%r" %s %b "%{Referer}i" "%{User-Agent}i" %Tf'
    )


if __name__ == '__main__':
    main()
