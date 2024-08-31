#!/usr/bin/env python3

import os
import sys
import json
import requests

import tornado.ioloop
import tornado.gen
import tornado.httpclient

import time
import uuid
import socket

import unimemcached
import alice.uniporxy.library.settings as settings


# --------------------------------------------------------------------------------------------------------------------
g_hosts = []
g_location = os.environ.get('QLOUD_DISCOVERY_INSTANCE', socket.getfqdn())


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def check_hosts():
    global g_hosts

    enabled = settings.config.get('memcached', {}).get('enabled', False)
    if not enabled:
        raise tornado.gen.Return(True)

    url = settings.config.get('memcached', {}).get('hosts_url')
    if not url:
        raise Exception('configuration issue: memcached enabled, but bo hosts url set')

    http_client = tornado.httpclient.AsyncHTTPClient()
    try:
        response = yield http_client.fetch(url)

        g_hosts = json.loads(response.body.decode('utf-8')).get('hosts', [])
    except tornado.httpclient.HTTPError as ex:
        g_hosts = []
        raise Exception('failed to fetch data from /hosts: {}'.format(ex))
    except Exception as ex:
        g_hosts = []
        raise Exception('some error during response parsing: {}'.format(ex))


# --------------------------------------------------------------------------------------------------------------------
def make_memcached_check(host, location):
    @tornado.gen.coroutine
    def check_memcached():
        salt = str(uuid.uuid1())
        keyvalue = 'memcached-check-{}-{}'.format(salt, g_location)
        client = unimemcached.Client([host], pool_size=1)
        yield client.connect()

        result = yield client.set(keyvalue, g_location, exptime=60)
        if not result:
            raise Exception('setting failed for host {}'.format(host))

        result = yield client.get(keyvalue)
        if not result:
            raise Exception('getting failed for host {}'.format(host))

        if result != g_location:
            raise Exception('got value is not expected on host {}'.format(host))

    return check_memcached


# --------------------------------------------------------------------------------------------------------------------
ioloop = tornado.ioloop.IOLoop().current()

try:
    ioloop.run_sync(check_hosts)
except Exception as ex:
    print('stats/hosts check failed, {}'.format(ex))
    sys.exit(1)

errors = 0
for host in g_hosts:
    try:
        ioloop.run_sync(make_memcached_check(host, g_location))
    except Exception as ex:
        print('memcached check failed: {}'.format(ex))
        errors += 1

if errors > 0:
    sys.exit(errors)

print('memcached OK')
