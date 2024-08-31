import os
import time
import json
import shutil
import asyncio
import tornado.httpclient
from alice.cuttlefish.library.python.test_utils import Process, Daemon, run_daemon  # noqa


def deepupdate(target, update):
    for key, value in update.items():
        if isinstance(value, dict) and isinstance(target.get(key), dict):
            deepupdate(target[key], value)
        elif value is None:
            target.pop(key, None)
        else:
            target[key] = value
    return target


def get_at(x, *path, default=None):
    try:
        for p in path:
            x = x[p]
        return x
    except KeyError:
        return default


def ensure_dir(p):
    if os.path.isdir(p):
        return
    if os.path.exists(p):
        os.remove(p)
    os.makedirs(p)


def remove_dir(p):
    if os.path.islink(p):
        os.remove(p)
    elif os.path.isdir(p):
        shutil.rmtree(p)


async def intrusive_fetch(url, timeout=3, logger=None):
    deadline = time.monotonic() + timeout
    while True:
        try:
            resp = await tornado.httpclient.AsyncHTTPClient().fetch(url)
            if resp.code == 200:
                return resp
        except Exception as err:
            if time.monotonic() > deadline:
                raise
            if logger is not None:
                logger.debug(f"Request to '{url}' failed and will be retried: {err}")
            await asyncio.sleep(0.1)


def read_json(*path):
    with open(os.path.join(*path), "r") as f:
        return json.load(f)


def write_json(val, *path, sort_keys=False):
    with open(os.path.join(*path), "w") as f:
        json.dump(val, f, indent=4, sort_keys=sort_keys)


def read_text(path):
    with open(path, "r") as f:
        return f.read()
