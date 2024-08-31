#!/usr/bin/env python3
import sys
import requests
from tornado.ioloop import IOLoop
from collections import namedtuple
import os
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.backends_common.storage import MdsStorage
import urllib
import time


if config["mds"].get("disabled"):
    sys.exit(0)

result = 0
try:
    res = requests.request("GET", config["mds"]["upload"] + "ping", timeout=3.0)
    if res.status_code != 200:
        raise Exception("bad status code %s" % (res.status_code,))
    print("MDS OK")
except Exception as exc:
    print("MDS unavalilable: %s" % (exc,))
    result |= 1

s = MdsStorage()


def check_mds_delete(f):
    global result
    try:
        res = f.result()
        print("MDS delete OK")
    except Exception as exc:
        print("MDS delete exception: %s" % (exc,))
        result |= 4
    IOLoop.current().stop()
    sys.exit(result)


def check_mds_save(f):
    global result
    global s
    try:
        res = f.result()
        name = res[res.rindex("/", 0, res.rindex("/")) + 1:]
        print("MDS save OK")
        IOLoop.current().add_future(s.delete(name), check_mds_delete)
    except Exception as exc:
        print("MDS save exception: %s" % (exc,))
        result |= 2
        IOLoop.current().stop()
        sys.exit(result)


def timeout():
    global result
    print("MDS request timeout")
    result |= 8
    IOLoop.current().stop()
    sys.exit(result)


IOLoop.current().add_future(
    s.save("{}_test_mds_save_{}".format(os.uname().nodename, time.time()), b"data"),
    check_mds_save
)
IOLoop.current().call_later(5.0, timeout)
IOLoop.current().start()
