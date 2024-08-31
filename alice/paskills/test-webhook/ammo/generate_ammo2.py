#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
from pathlib import Path
from datetime import datetime

import json

now = str(int(datetime.timestamp(datetime.now())))


def find_fixtures(path):
    oauth = '<OAUTH_TOKEN_HERE>'
    headers = "X-Forwarded-For: 2a02:2168:89c0:8d00:1cbc:f2c3:d8d2:e2f6, 2a02:6b8:c1b:240a:10b:3def:bb22:0, 2a02:6b8:c1b:2b93:0:41e7:bba6:0\r\n" + \
              "X-Req-Id: 1580803926147543-1268037155-megamind-sas-19-vins\r\n" + \
              "Host: billing-rc.quasar.common.yandex.net\r\n" + \
              "User-Agent: Mozilla/5.0 (Linux; arm; Android 6.0.1; Station Build/MOB30J; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/78.0.3904.7 YandexStation/2.3.4.18.576129343.20191225 (YandexIO) Safari/537.36\r\n" + \
              "Content-Type: application/json\r\n" + \
              "Authorization: OAuth "+oauth+"\r\n" + \
              "Connection: Close"

    print(make_ammo("POST",
                         '/billing/requestContentPlay?deviceId=04007884c9140411008f&contentItem=%7B%22kinopoisk%22:%7B%22id%22:%22243be43511807e3b3a01c38f6916f4be8%22%7D,%22type%22:%22film%22%7D&startPurchaseProcess=false',
                         headers, 'requestContentPlay',
                         '{"contentPlayPayload":{}}'))

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


find_fixtures('')
