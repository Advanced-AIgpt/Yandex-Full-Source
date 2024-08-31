#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import random
from argparse import ArgumentParser
from typing import NamedTuple

import requests


class Request(NamedTuple):
    method: str
    url: str
    params: object
    data: object
    files: object
    tag: str
    weight: int


def print_request(tag, request):
    req = "{method} {path_url} HTTP/1.1\r\n{headers}\r\n".format(
        method=request.method,
        path_url=request.path_url,
        headers=''.join('{0}: {1}\r\n'.format(k, v) for k, v in request.headers.items())
    ).encode()
    if request.body:
        req = b''.join([req, request.body, b'\n'])

    return b''.join([
        "{req_size} {tag}\n".format(req_size=len(req), tag=tag).encode(),
        req,
        b'\n'
    ])


# POST multipart form data
def post_multipart(method, url, params, headers, data, files, tag):
    req = requests.Request(
        method,
        'http://some.host{url}'.format(
            url=url,
        ),
        params=params,
        headers=headers,
        data=data,
        files=files
    )
    prepared = req.prepare()
    return print_request(tag, prepared)


def main(oauth):
    headers = {
        'Host': 'dialogs.yandex.net',
        'Authorization': 'OAuth ' + oauth
    }

    requests = [
        Request(
            'GET', '/api/v1/status', {'delay': 100},
            None, None,
            'status', 5
        ),
        Request(
            'GET', '/api/v1/status', {'delay': 200},
            None, None,
            'status', 2
        ),

        Request(
            'GET', '/api/v1/skills/some-skill-id/images', {'delay': 100},
            None, None,
            'images_list', 2
        ),
        Request(
            'GET', '/api/v1/skills/some-skill-id/images', {'delay': 150},
            None, None,
            'images_list', 1
        ),
        Request(
            'POST', '/api/v1/skills/some-skill-id/images', {'delay': 1000},
            None, {
                'file': ('100px.jpg', os.urandom(3000))
            },
            'image', 3
        ),
        Request(
            'POST', '/api/v1/skills/some-skill-id/images', {'delay': 1200},
            None, {
                'file': ('400px.png', os.urandom(25000))
            },
            'image', 2
        ),

        Request(
            'GET', '/api/v1/skills/some-skill-id/sounds', {'delay': 200},
            None, None,
            'sounds_list', 2
        ),
        Request(
            'POST', '/api/v1/skills/some-skill-id/sounds', {'delay': 2000},
            None, {
                'file': ('some.mp3', os.urandom(250000))
            },
            'status', 2
        ),

        Request(
            'POST', '/api/v1/skills/some-skill-id/callback', {'delay': 100},
            '{"status":"ok","request_id":"adfab7fa-556d-4696-b68a-a2cab7252c1c","rooms":[{'
            '"id":"f2a450c5-5d2a-4f0a-bb7a-758b682b3d5d","name":"Кабинет","devices":[{'
            '"id":"956be9c7-b5f2-4558-919a-04a10bb30f8c","name":"Кондиционер","type":"devices.types.thermostat.ac",'
            '"capabilities":[{"retrievable":true,"type":"devices.capabilities.on_off","state":null,"parameters":{}}],'
            '"groups":[],"skill_id":"T"},{"id":"ca3981e5-474d-4caa-ba24-332d2a549ee1","name":"Лампа сяоми",'
            '"type":"devices.types.light","capabilities":[{"retrievable":true,"type":"devices.capabilities.on_off",'
            '"state":{"instance":"on","value":true},"parameters":{}}],"groups":[],'
            '"skill_id":"ad26f8c2-fc31-4928-a653-d829fda7e6c2"},{"id":"e3a0b5e9-272e-48f2-9dbc-6cadc4b15366",'
            '"name":"Лампа филипс","type":"devices.types.light","capabilities":[{"retrievable":true,'
            '"type":"devices.capabilities.on_off","state":{"instance":"on","value":true},"parameters":{}}],'
            '"groups":[],"skill_id":"4a8cbce2-61d3-4e58-9f7b-6b30371d265c"},'
            '{"id":"bd3b10bb-2a50-49ac-ace0-7d4695756328","name":"Пульт","type":"devices.types.hub","capabilities":['
            '],"groups":[],"skill_id":"T"}]}],"groups":[],"unconfigured_devices":[]}'.encode(), None,
            'bulbasaur_callback', 100000
        )
    ]

    lst = []
    for i, req in enumerate(requests):
        for c in range(req.weight):
            lst.append(i)
    random.shuffle(lst)

    ammo = open('ammo', 'wb')
    for i in lst:
        r = requests[i]
        ammo.write(post_multipart(r.method, r.url, r.params, headers, r.data, r.files, r.tag))
    ammo.close()


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "-t", "--token",
        dest="oauth",
        help="OAuth token from https://oauth.yandex.ru/authorize?response_type=token&client_id=c473ca268cd749d3a8371351a8f2bcbd",
        required=True
    )
    args = parser.parse_args()
    main(args.oauth)
