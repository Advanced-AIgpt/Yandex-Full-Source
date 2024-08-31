#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gc
import inspect
import io
import json
import logging
import os
import pytest
import struct
import sys
import time
import uuid

# from pympler import tracker, classtracker
# from pympler.web import start_profiler, start_in_background
from alice.uniproxy.library.extlog.sessionlog import LogEvent as LogEvent
from alice.uniproxy.library.extlog import SessionLogger, AccessLogger
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from tests.basic import server, recursive_merge_dict
from tests.mock_backend import mock_music_and_asr
from tornado import gen
from tornado.concurrent import Future
from tornado.ioloop import IOLoop
from tornado.httpclient import AsyncHTTPClient
from tornado.websocket import websocket_connect


_PROC_STATUS = '/proc/{}/status'.format(os.getpid())

_SCALE = {'kB': 1024, 'mB': 1024*1024,
          'KB': 1024, 'MB': 1024*1024}


def _VmB(VmKey):
    global _PROC_STATUS, _SCALE
    # get pseudo file  /proc/<pid>/status
    try:
        t = open(_PROC_STATUS)
        v = t.read()
        t.close()
    except:
        return 0  # non-Linux?
    # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
    i = v.index(VmKey)
    v = v[i:].split(None, 3)  # whitespace
    if len(v) < 3:
        return 0  # invalid format?
    # convert Vm value to bytes
    return int(v[1]) * _SCALE[v[2]]


def memory(since=0):
    """
        Return memory usage in bytes.
    """
    return _VmB('VmSize:') - since


def resident(since=0):
    """
        Return resident memory usage in bytes.
    """
    return _VmB('VmRSS:') - since


class SessionResult:
    def __init__(self, error=None):
        self.error = error

    def has_error(self):
        return self.error is not None


class Session:
    def __init__(self, ctx):
        self.ctx = ctx
        self.num = ctx.cur_num
        self.error = None
        self.socket = None
        self.timeout = None
        self.audio_stream = io.BytesIO(ctx.audio_data)
        self.music_request = ctx.music_request
        self.timeout = 0.01
        self.expected_directives = set(('ASR.Result',))
        if self.music_request:
            self.expected_directives.add('ASR.MusicResult')
        Logger.get().debug('========== Session #{} =========='.format(self.num))

    def __del__(self):
        Logger.get().debug('========== ~Session #{} =========='.format(self.num))
        # sys.stderr.write('~Session {}\n'.format(self.num))

    def run(self):
        self.result_future = Future()
        self.connect()
        return self.result_future

    def connect(self):
        websocket_connect(
            "ws://localhost:%s/uni.ws" % (config["port"],),
            callback=self.on_connect,
            on_message_callback=self.on_message)

    def on_connect(self, fut):
        try:
            if fut.exception():
                self.fail(fut.exception())
                return
            self.socket = fut.result()
        except:
            log.ERR('connect failed')
            IOLoop.current().stop()
            return

        self.socket.write_message(json.dumps({
            "event": {
                "header": {
                    "namespace": "System",
                    "name": "SynchronizeState",
                    "messageId": str(uuid.uuid4()),
                },
                "payload": {
                    "auth_token": config["key"],
                    "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
                }
            }
        }))

        self.message_id = str(uuid.uuid4())
        event = {
            "event": {
                "header": {
                    "namespace": "ASR",
                    "name": "Recognize",
                    "messageId": self.message_id,
                    "streamId": 1
                },
                "payload": {
                    "lang": "ru-RU",
                    "topic": "queries",
                    "application": "test",
                    "format": "audio/x-pcm;bit=16;rate=16000",
                    "key": "developers-simple-key",
                }
            }
        }
        if self.music_request:
            event["event"]["payload"]["format"] = "audio/opus"
            recursive_merge_dict(event, {
                "event": {
                    "payload": {
                        ("music_request" if self.music_request == 1 else "music_request2"): {
                            "headers": {
                                "Content-Type": "audio/opus",
                            },
                            "time_limit": 2
                        }
                    }
                }
            })
        self.socket.write_message(json.dumps(event))
        IOLoop.current().call_later(self.timeout, self.send_more_data)

    def fail(self, error):
        self.finish()
        self.error = error
        self.result_future.set_result(SessionResult(error=error))

    def on_message(self, msg):
        Logger.get().debug('on_message')
        if msg is None:
            if self.socket is None:
                return
            self.fail("Socket closed unexpected")
            return

        if isinstance(msg, bytes):
            self.fail("Server returned some binary data")
            return
        try:
            res = json.loads(msg)
            if "directive" not in res:
                self.fail("Server returned unwanted message")
            else:
                directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                    res.get("directive").get("header", {}).get("name", "")
                if directive == 'ASR.Result':
                    end_of_utt = res.get("directive").get("payload", {}).get("endOfUtt")
                    if end_of_utt is not None and not end_of_utt:
                        Logger.get().debug('skip not finally ASR.Result event')
                        return  # skip not finally event
                if directive not in self.expected_directives:
                    self.fail("Wrong directive: %s" % (directive,))
                    return
                Logger.get().debug('receive directive {}'.format(directive))
                self.expected_directives.remove(directive)
                if len(self.expected_directives) == 0:
                    # receive last directive, so finish
                    self.finish()
                    self.result_future.set_result(SessionResult())
        except Exception as e:
            log.EXC(e)
            self.fail("Exception in test: %s" % (e,))

    def send_more_data(self):
        chunk = self.audio_stream.read(32000)
        if chunk:
            prepend = struct.pack(">I", 1)
            self.socket.write_message(prepend + chunk, binary=True)
            IOLoop.current().call_later(self.timeout, self.send_more_data)
        else:
            self.socket.write_message(json.dumps({
                "streamcontrol": {
                    "streamId": 1,
                    "action": 0,
                    "reason": 0,
                    "messageId": self.message_id
                }
            }))

    def finish(self):
        self.audio_stream = None  # for more quick memory deallocation
        if self.socket is not None:
            self.socket.close()
            self.socket = None
            pass


class SessionsContext:
    def __init__(self, rep_limit, audio_file, music_request=True, leak_limit=5000, warm_rep=2000):
        self.rep_limit = rep_limit
        self.warm_rep = warm_rep
        self.cur_num = 0
        with open(audio_file, "rb") as f:
            self.audio_data = f.read()
        self.vm = 0
        self.rss = 0
        self.music_request = music_request
        self.closing = False
        self.error = None
        self.base_rss = None
        self.memory_limit = None
        self.leak_limit = leak_limit
        self.log_mem()

    def log_mem(self, vm=None, rss=None):
        if vm is None:
            vm = memory()
        if rss is None:
            rss = resident()
        log.INFO('#{} vm={} ({}) rss={} ({})'.format(self.cur_num, vm, vm-self.vm, rss, rss-self.rss))
        self.vm = vm
        self.rss = rss

    def run(self):
        IOLoop.current().add_future(self.process(), self.on_finish)
        IOLoop.current().start()
        self.log_mem()
        if self.error is not None:
            Logger.get().warning(self.error)
        assert(self.error is None)

    def on_finish(self, fut):
        gc.collect()
        if not self.error:
            err = fut.exception()
            if err is not None:
                self.error = str(err)
        Logger.get().debug('stop IOLoop')
        IOLoop.current().stop()
        fut.result()

    @gen.coroutine
    def process(self):
        while True:
            session = Session(self)
            session_result = yield session.run()
            if session_result.has_error():
                self.error = session_result.error
                raise gen.Return(None)

            # check/log memory consumption
            rss = resident()
            vm = memory()
            if self.base_rss is None:
                if self.cur_num == self.warm_rep:
                    # after execute first warm_rep Session's store resident memory as base value
                    self.base_rss = rss
                    self.memory_limit = self.leak_limit * (self.rep_limit - self.warm_rep) + self.base_rss
            elif self.memory_limit:
                over_base_rss = rss - self.base_rss
                if rss > self.memory_limit:
                    self.error = 'over_base_rss={} overrun memory_limit={} after {} requests:'\
                        ' (total_rss={} (base_rss={} + over_base_rss={})) leak={} bytes per request (>limit={})'.format(
                            over_base_rss, self.memory_limit, self.cur_num,
                            rss, self.base_rss, over_base_rss, int(over_base_rss/(self.cur_num-self.warm_rep)), self.leak_limit)
                    raise gen.Return(None)  # FAIL on memory overusage
            if self.cur_num % 100 == 0:
                self.log_mem(vm=vm, rss=rss)

            if self.cur_num >= self.rep_limit:
                over_base_rss = rss - self.base_rss
                log.INFO('test Ok: after {} requests'\
                    ' (total_rss={} (base_rss={} + over_base_rss={})) avg. leak={} bytes per request, less than limit={}'.format(
                        self.cur_num,
                        rss, self.base_rss, over_base_rss, int(over_base_rss/(self.cur_num-self.warm_rep)), self.leak_limit))
                raise gen.Return(None)  # SUCCESSFULLY finish testing
            self.cur_num += 1

    def print_referrers_tree(self, obj, indent=0):
        """
            helper for search referrers to given object
            (which preventig it deletion)
        """
        if not indent:
            print('#####', type(obj).__name__, obj)
            gc.collect()
        if indent >= 13:
            return
        refs = gc.get_referrers(obj)
        for r in refs:
            if isinstance(r, SessionsContext):
                print('\t'*indent, 'SELF')
            elif inspect.ismethod(r):
                print('\t'*indent, type(r).__name__, r)
                self.print_referrers_tree(r, indent+1)
            elif inspect.isframe(r):
                if inspect.getframeinfo(r).function == 'print_referrers_tree':
                    continue
                print('\t'*indent, type(r).__name__, inspect.getframeinfo(r))
                # print('\t'*indent, inspect.getsourcelines(r))
            elif isinstance(r, list) and inspect.isframe(r[0]):
                continue
            elif isinstance(r, Future):
                continue
            else:
                if isinstance(r, dict) and 'audio_data' in r:
                    print('\t'*indent, 'SELF-DICT')
                else:
                    print('\t'*indent, type(r).__name__, r)
                    self.print_referrers_tree(r, indent+1)
        if not indent:
            print("=====")


@pytest.fixture(scope="module",
                params=[1, 2])
def music_api_ver(request):
    return request.param


def test_music_asr_memory_consumption(server, monkeypatch, music_api_ver):
    AsyncHTTPClient.configure(None, max_clients=1000)

    mock_music_and_asr(monkeypatch)

    # suppress most debug info output
    from log import _g_log, _g_access_log
    _g_log.setLevel(logging.INFO)
    _g_access_log.setLevel(logging.ERROR)

    # suppress accesslog record printing (to stdout)
    def mock_end(self, code=200, size=0):
        pass
    monkeypatch.setattr(AccessLogger, 'end', mock_end)

    # suppress network logging writing
    def mock_start_stream(self, message_id, stream_id, format_="pcm16"):
        pass
    monkeypatch.setattr(SessionLogger, 'start_stream', mock_start_stream)

    # mock known memleak (call yson.dumps)
    def mock_save(self, action, data={}):
        pass
    monkeypatch.setattr(LogEvent, '_save', mock_save)

    rt = SessionsContext(100000, "tests/music_record1.ogg", music_request=music_api_ver, leak_limit=100)
    rt.run()
