#!/usr/bin/env python
# -*- coding: utf-8 -*-

from alice.uniproxy.library.musicstream import MusicStream2
import base64
import alice.uniproxy.library.experiments as experiments
import functools
import io
import json
import os
import pytest
import struct
import sys
import tests.basic
import tornado.httpclient
import uuid


from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.backends_common.httpstream import HttpStream
from alice.uniproxy.library.backends_common.protostream import ProtoStream
from enum import Enum
from functools import lru_cache
from google.protobuf import text_format
from voicetech.library.proto_api.basic_pb2 import ConnectionResponse
from voicetech.library.proto_api.voiceproxy_pb2 import ConnectionRequest, AddData, AddDataResponse
from voicetech.library.proto_api.voiceproxy_pb2 import Result, Word, AlignInfo, Metainfo
from voicetech.library.proto_api.yaldi_pb2 import InitRequest, InitResponse
from voicetech.library.proto_api.yaldi_pb2 import AddData as YaldiAddData
from voicetech.library.proto_api.yaldi_pb2 import AddDataResponse as YaldiAddDataResponse
from voicetech.library.proto_api.yaldi_pb2 import Result as YaldiResult
from voicetech.library.proto_api.yaldi_pb2 import Word as YaldiWord
from alice.uniproxy.library.settings import config
from tempfile import TemporaryDirectory
from tests.basic import Session, server, connect
from tornado import gen
from tornado.concurrent import Future
from tornado.httputil import HTTPHeaders
from tornado.ioloop import IOLoop
from alice.uniproxy.library.async_http_client import HTTPResponse


@lru_cache()
def match_music_response():
    Logger.get().debug('read music response')
    with open("tests/match_music_success_result.json", 'rb') as f:
        return f.read()


def match_contacts_response():
    Logger.get().debug('read contats response')
    # return json.dumps({ 'data' : [ { 'contacts' : [] } ] }).encode('utf-8')
    with open("tests/data/contacts_set/book.json", 'r') as f:
        return json.dumps({'data': [ { 'contacts': json.load(f) } ] }).encode('utf-8')


@lru_cache()
def uaas_headers():
    flags = [
        [  # not used (garbage) flags
            {
                "HANDLER": "REPORT",
                "CONTEXT": {
                    "MAIN": {
                        "source": {"QUICK": {"rearr": ["scheme_Local/NewsFromQuickMiddle/UseNewsSerp=da"]}}},
                    "REPORT": {"flags": ["news_vert=1"], "test_id": ["51005"]}
                }
            }
        ],
        [  # useful flags
            {
                "HANDLER": "VOICE",
                "CONDITION": "uniproxy",
                "CONTEXT": {
                    "MAIN": {},
                    "VOICE": {
                        "flags": [
                            "how_much",
                        ],
                        "test_id": "666"
                    }
                }
            }
        ],
        [  # useful flags
            {
                "HANDLER": "VOICE",
                "CONDITION": "vins_app.get('app_id', '') == '1.2.3'",
                "CONTEXT": {
                    "MAIN": {},
                    "VOICE": {
                        "flags": {
                            "patch123": "patch123",
                        },
                        "test_id": "123"
                    }
                }
            }
        ],
        [  # useful flags
            {
                "HANDLER": "VOICE",
                "CONDITION": "vins_app.get('app_id', '') == '9.9.9'",
                "CONTEXT": {
                    "MAIN": {},
                    "VOICE": {
                        "flags": [
                            {"patch999": "patch999"},
                        ],
                        "test_id": "999"
                    }
                }
            }
        ]
    ]
    return HTTPHeaders({
        'X-Yandex-RandomUID': '3229049651502814897',
        'X-Yandex-LogstatUID': '12345678901234567',
        'X-Yandex-ExpConfigVersion': '5883',
        'X-Yandex-ExpBoxes': '51005,0,83;666,0,88;123,0,84;999,0,85',
        'X-Yandex-ExpFlags': ','.join(base64.encodebytes(json.dumps(f).encode('ascii')).decode('ascii') for f in flags),
    })


def dlogMethod(self, method):
    Logger.get().debug('{}.{}'.format(type(self).__name__, method))


class FakeAsyncHTTPClient:
    def __init__(self, url, code=200, content=None, headers=None, timeout=None):
        dlogMethod(self, '__init__(...)')
        self.io_loop = IOLoop.current()
        self.url = url
        self.resp_code = code
        self.resp_headers = headers
        self.resp_content = content
        self.timeout = timeout

    def __del__(self):
        dlogMethod(self, '__del__()')

    @tornado.gen.coroutine
    def fetch(self, request, process_result_cb=None):
        dlogMethod(self, 'fetch(...)')
        self.request = request
        if self.timeout:
            yield tornado.gen.sleep(self.timeout)

        if process_result_cb is None:
            return HTTPResponse(self.resp_code, body=self.resp_content)

        self.process_result_cb = process_result_cb
        if self.request.body_producer is not None:
            fut = self.request.body_producer(self.fake_write)
        else:
            IOLoop.current().spawn_callback(self.on_result)
            return

        if fut is not None:
            fut = gen.convert_yielded(fut)

            def on_body_written(fut):
                dlogMethod(self, '<callback>on_body_written(...)')
                fut.result()
                self.on_result()

            self.io_loop.add_future(fut, on_body_written)
            return
        else:
            raise Exception('Mocking: Can not create feature')

    def on_result(self):
        self.process_result_cb(tornado.httpclient.HTTPResponse(self, self.resp_code,
                                                            buffer=io.BytesIO(self.resp_content),
                                                            headers=self.resp_headers))

    def close(self):
        dlogMethod(self, 'close()')

    def fake_write(self, chunk):
        dlogMethod(self, 'fake_write(len={})'.format(len(chunk)))
        future = Future()
        future.set_result(None)
        return future


class Stage(Enum):
    HttpGET = 0
    Http101 = 1
    ConnectionRequest = 2
    ConnectionResponse = 3
    AddData = 5
    AddDataResponse = 4


class YaldiStage(Enum):
    HttpGET = 0
    Http101 = 1
    InitRequest = 2
    InitResponse = 3
    AddData = 5
    AddDataResponse = 4


class FakeMusicWebSocketClient:
    def __init__(self):
        dlogMethod(self, '__init__(...)')
        # self.io_loop = IOLoop.current()
        self.count_audio = 0
        self.stage = 0
        self.stage1_count = 8000 * 2 * 2

    def __del__(self):
        dlogMethod(self, '__del__()')

    def connect(self, master, req):
        self.master = master
        self.req = req

    def write_message(self, pcm, binary):
        dlogMethod(self, 'write_message()')
        self.count_audio += len(pcm)
        if self.stage == 0 and self.count_audio >= self.stage1_count:
            self.stage = 1
            self.master.on_message("""
{
  "directive": {
    "header": {
      "name":"recognition"
    },
    "payload": {
      "result": {
        "match": {
          "id": 2424145,
          "title": "Under A Violet Moon"
        }
      }
    }
  }
}
""")

    def close(self):
        dlogMethod(self, 'close()')


class FakeYaldiStream:
    def __init__(self, final_response='<SPOKEN_NOISE>'):
        dlogMethod(self, '__init__(...)')
        self.is_closed = False
        self.stage = YaldiStage.HttpGET
        self.response_queue = []
        self.last_chunk = False
        self.finalDataResponse = YaldiAddDataResponse(
            responseCode=200,
            recognition=[
                YaldiResult(
                    confidence=1.0,
                    words=[
                        YaldiWord(
                            confidence=1.0,
                            value=final_response,
                            align_info=AlignInfo(
                                start_time=0.0,
                                end_time=0.85,
                                acoustic_score=0.0,
                            ),
                        ),
                    ],
                    normalized=final_response + ' ',
                    align_info=AlignInfo(
                        start_time=0.0,
                        end_time=1.8,
                        acoustic_score=-172.0,
                        graph_score=141.0,
                        lm_score=0.0,
                        total_score=-31.0,
                    ),
                ),
            ],
            endOfUtt=True,
            messagesCount=2,
            metainfo=Metainfo(
                minBeam=12.71049976348877,
                maxBeam=13.0,
                topic="general",
                lang="ru",
                version="0.11.0-test",
                load_timestamp="Tue May 23 08:01:04 2017",
            )
        )

    def __del__(self):
        dlogMethod(self, '__del__()')

    def close(self):
        dlogMethod(self, 'close()')
        self.is_closed = True
        return False

    def closed(self):
        dlogMethod(self, 'closed()')
        return self.is_closed

    def write(self, data, callback=None):
        if self.stage == YaldiStage.HttpGET:
            dlogMethod(self, 'write(size={}):\n{}'.format(len(data), data.decode('utf-8')))
            self.stage = YaldiStage.Http101
        elif self.stage == YaldiStage.InitRequest:
            msg = InitRequest()
            msg.ParseFromString(data[data.index(b"\r\n") + 2:])
            self.stage = YaldiStage.InitResponse
            dlogMethod(self, 'write(size={}){}'.format(
                len(data),
                ' protobuf {}:\n{}'.format(type(msg).__name__, text_format.MessageToString(msg, as_utf8=True))
            ))
        elif self.stage == YaldiStage.AddData:
            msg = AddData()
            msg.ParseFromString(data[data.index(b"\r\n") + 2:])
            msg.audioData = b'...'
            self.last_chunk = msg.lastChunk  # pylint: disable=no-member
            self.stage = YaldiStage.AddDataResponse
            dlogMethod(self, 'write(size={}){}'.format(
                len(data),
                ' protobuf {}:\n{}'.format(type(msg).__name__, text_format.MessageToString(msg, as_utf8=True))
            ))
        else:
            raise Exception('FakeYaldiStream error')

        if callback is not None:
            callback()
        else:
            future = Future()  # TracebackFuture()
            future.add_done_callback(lambda f: f.exception())
            future.set_result(None)
            return future

    def read_until(self, separator):
        dlogMethod(self, 'read_until({})'.format(repr(separator)))
        future = Future()
        if self.response_queue:
            future.set_result(self.response_queue[0])
            self.response_queue = self.response_queue[1:]
            return future
        if self.stage == YaldiStage.Http101:
            http101 = b'HTTP/1.1 101 Switching Protocols\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n' \
                b'Content-Length: 0\r\nContent-Type: text/html\r\nDate: Fri, 02 Jun 2017 15:00:44 MSK\r\n'\
                b'Server: YaVoiceProxy2\r\nX-YaReqFinish: 1496404844.992396\r\nX-YaRequestId: 1bc286a8-478b-11e7-996d-fa163e1f2518\r\n\r\n'
            future.set_result(http101)
            dlogMethod(self, 'set_response() http101:\n{}'.format(http101.decode('utf-8')))
            self.stage = YaldiStage.InitRequest
        elif self.stage == YaldiStage.InitResponse:
            message = InitResponse(responseCode=200)
            self.set_response(future, message)
            self.stage = YaldiStage.AddData
        elif self.stage == YaldiStage.AddDataResponse:
            if self.last_chunk:
                message = self.finalDataResponse
            else:
                message = YaldiAddDataResponse(responseCode=200, endOfUtt=False, messagesCount=1)
            self.set_response(future, message)
            self.stage = YaldiStage.AddData
        return future

    def read_bytes(self, size):
        future = Future()
        if not self.response_queue:
            raise Exception('Invalid read')
        logS = ''
        resp = self.response_queue.pop(0)
        if isinstance(resp, tuple):
            logS, resp = resp
        if len(resp) != size:
            raise Exception('Invalid read (size)')
        dlogMethod(self, 'read_bytes({}){}'.format(size, logS))
        future.set_result(resp)
        return future

    def set_response(self, future, message):
        logS = ' protobuf {}:\n{}'.format(
            type(message).__name__,
            text_format.MessageToString(message, as_utf8=True))
        data = message.SerializeToString()
        sz = hex(len(data))[2:].encode("utf-8") + b'\r\n'
        future.set_result(sz)
        dlogMethod(self, 'set_response(): {}'.format(sz))
        self.response_queue = [(logS, data)]


def mock_music_and_asr(monkeypatch):
    # use FakeAsyncHTTPClient for request to music recognition service
    old_create_client = HttpStream.create_client
    def mock_create_client(self, url):
        # url sample 'https://match.music.yandex.net/match/api/upload-json'
        if 'match.music.yandex.net' in url:
            Logger.get().debug('### MOCK-ing HttpStream client to {}'.format(url))
            self.client = FakeAsyncHTTPClient(url, content=match_music_response())
        else:
            Logger.get().debug('### SKIP MOCK-ing http client to {}'.format(url))
            old_create_client(self, url)
    monkeypatch.setattr(HttpStream, 'create_client', mock_create_client)

    # use FakeMusicWebSocketClient for request to music recognition service (API v2.0)
    old_create_ws_client = MusicStream2.connect
    def mock_connect_ws_client(self):
        # url sample 'ws://match-int.music.qa.yandex.net:32351/match/websocket'
        if 'yandex.net:32351/match/websocket' in self.url:
            Logger.get().debug('### MOCK-ing WebSocket client to {}'.format(self.url))
            conn = FakeMusicWebSocketClient()
            conn.connect(self, None)
            future = Future()
            future.set_result(conn)
            return future
        else:
            Logger.get().debug('### SKIP MOCK-ing websocket client to {}'.format(self.url))
            return self.old_connect_ws_client()
    monkeypatch.setattr(MusicStream2, 'connect', mock_connect_ws_client)

    # use fake FakeYaldiStream for request to ASR service
    def mock_connect(self):
        Logger.get().debug('### MOCK connect to {}:{}{}'.format(self.host, self.port, self.uri))
        self.stream = FakeYaldiStream('бнопня')
        self._connected = True
        IOLoop.current().spawn_callback(self._make_handshake, 52)

    monkeypatch.setattr(ProtoStream, 'connect', mock_connect)


def mock_uaas(monkeypatch):
    Logger.get().debug('### MOCK-ing experiments config to UaaS')
    config.set_by_path('experiments', {
        'uaas': {
            'enabled': True,
            'url': 'http://uaas.search.yandex.net'
        }
    })
    # disable local experiments
    monkeypatch.setattr(experiments, 'experiments', experiments.Experiments(None))

    # use FakeAsyncHTTPClient for request to UaaS
    old_create_uaas_client = backend.uaas.UaasClient.create_client
    def mock_create_uaas_client(self, url):
        # url sample 'http://uaas.search.yandex.net/speechkitvoiceproxy'
        if 'http://uaas.search.yandex.net' in url:
            Logger.get().debug('### MOCK-ing UaasClient to {}'.format(url))
            self.client = FakeAsyncHTTPClient(url, content=b"USERSPLIT", headers=uaas_headers())
        else:
            Logger.get().debug('### SKIP MOCK-ing http client to {}'.format(url))
            self.old_create_uaas_client(url)
    monkeypatch.setattr(backend.uaas.UaasClient, 'create_client', mock_create_uaas_client)


def mock_experiments(monkeypatch):
    # create tmpdir with experiment config
    with TemporaryDirectory() as tmpdir:
        with open(os.path.join(tmpdir, 'exp666.json'), 'w') as f:
            json.dump({
                'id': 'exp#666',
                'share': 1.,  # <<< apply experiment to 100% sessions (with zero control share)
                'control_share': 0.,
                'flags': [
                    ["if_event_type", "System.EchoRequest", "set", "/patch", "patch_data"],
                ]
            }, f)
        with open(os.path.join(tmpdir, 'exp5.json'), 'w') as f:
            json.dump({
                'id': 'exp#5',
                'pool': 'pool2',
                'share': .5,  # <<< apply experiment to 50% sessions (with 10% control share)
                'control_share': 0.1,
                'flags': [
                    ["if_event_type", "System.EchoRequest", "set", "/patch2", "patch2_data"],
                ]
            }, f)
        exps = experiments.Experiments(tmpdir)
    Logger.get().debug('### MOCK experiments')
    monkeypatch.setattr(experiments, 'experiments', exps)


def mock_contacts(monkeypatch, timeout=None):
    # disable local experiments
    monkeypatch.setattr(experiments, 'experiments', experiments.Experiments(None))

    # use FakeAsyncHTTPClient for request to get Contacts
    old_get_client = backend.contacts._get_client
    def mock_get_client(url):
        return FakeAsyncHTTPClient(url, content=match_contacts_response(), timeout=timeout)

    monkeypatch.setattr(backend.contacts, '_get_client', mock_get_client)
