#!/usr/bin/env python
import sys

from tornado.ioloop import IOLoop
import json
import pytest
import functools
import struct
import unittest
import uuid
from tests.basic import Session, server, connect, recursive_merge_dict
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


_TIMEOUT = 0.1
_PCM_FILE = "tests/test.pcm"


@pytest.yield_fixture
def music_file():
    _file = open("tests/music_record.ogg", "rb")
    yield _file
    _file.close()


class SessionContext:
    def __init__(self):
        self.data_generator = None

    def on_connect_asr(self, test_file, socket, music_request=0):
        socket.write_message(json.dumps({
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

        message_id = str(uuid.uuid4())
        event = {
            "event": {
                "header": {
                    "namespace": "ASR",
                    "name": "Recognize",
                    "messageId": message_id,
                    "streamId": 1
                },
                "payload": {
                    "lang": "ru-RU",
                    "topic": "dialogeneral",
                    "application": "test",
                    "format": "audio/x-pcm;bit=16;rate=16000",
                    "key": "developers-simple-key",
                    "advancedASROptions": {
                        "partial_results": False
                    }
                }
            }
        }
        if music_request:
            event["event"]["payload"]["format"] = "audio/opus"
            recursive_merge_dict(event, {
                "event": {
                    "payload": {
                        ("music_request" if music_request == 1 else "music_request2"): {
                            "headers": {
                                "Content-Type": "audio/opus",
                            }
                        },
                    },
                }
            })
        self.message_id = message_id
        self.stream_id = 1
        socket.write_message(json.dumps(event))
        IOLoop.current().call_later(_TIMEOUT, functools.partial(self.send_more_data, test_file, 1, message_id))

    def on_connect_last_chunk(self, socket):
        socket.write_message(json.dumps({
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

        message_id = str(uuid.uuid4())
        event = {
            "event": {
                "header": {
                    "namespace": "ASR",
                    "name": "Recognize",
                    "messageId": message_id,
                    "streamId": 1
                },
                "payload": {
                    "lang": "ru-RU",
                    "topic": "dialogeneral",
                    "application": "test",
                    "format": "audio/x-pcm;bit=16;rate=16000",
                    "key": "developers-simple-key",
                    "advancedASROptions": {
                        "partial_results": False
                    }
                }
            }
        }
        socket.write_message(json.dumps(event))
        socket.write_message(json.dumps({
            "streamcontrol": {
                "streamId": 1,
                "action": 0,
                "reason": 0,
                "messageId": message_id
            }
        }))
        self.message_id = None

    def send_more_data(self, test_file, stream_id, message_id):
        chunk = test_file.read(32000)
        if chunk:
            prepend = struct.pack(">I", stream_id)
            Session.socket().write_message(prepend + chunk, binary=True)
            self.data_generator = IOLoop.current().call_later(_TIMEOUT, functools.partial(self.send_more_data, test_file, stream_id, message_id))
        else:
            self.data_generator = None
            self.close_stream()
            # test_file.close()

    def close_stream(self):
        if self.message_id is not None:
            Logger.get().debug('send_streamcontrol')
            Session.socket().write_message(json.dumps({
                "streamcontrol": {
                    "streamId": self.stream_id,
                    "action": 0,
                    "reason": 0,
                    "messageId": self.message_id
                }
            }))
            self.message_id = None

    def on_message_asr(self, msg, expected_directives=['ASR.Result', ], ignored_directives=['Messenger.BackendVersions', ]):
        if msg is None:
            Session.fuckit("Socket closed unexpected")
            return

        if isinstance(msg, bytes):
            Session.fuckit("Server returned some binary data")
            return
        try:
            res = json.loads(msg)
            if "directive" not in res:
                Session.fuckit("Server returned unwanted message")
                return
            else:
                directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                    res.get("directive").get("header", {}).get("name", "")
                Logger.get().debug('DIRECTIVE: ' + str(res))
                if directive == 'ASR.Result':
                    end_of_utt = res.get("directive").get("payload", {}).get("endOfUtt")
                    if end_of_utt is not None and not end_of_utt:
                        Logger.get().debug('skip not finally ASR.Result event')
                        return
                if directive == 'ASR.MusicResult':
                    payload = res.get("directive").get("payload", {})
                    result = payload.get("result", {})
                    if result and isinstance(result, str):
                        if result == 'music':
                            Logger.get().debug('skip not finally ASR.MusicResult event')
                            return
                    match = result.get("match", {}) if isinstance(result, dict) else result
                    if len(match) == 0 or (isinstance(result, str) and result != 'success'):
                        Logger.get().debug('Bad recognition result')
                        Session.fuckit("Bad recognition result: " + str(result))
                        return
                if directive not in expected_directives:
                    if directive not in ignored_directives:
                        Session.fuckit("Wrong directive [1]: %s" % (directive,))
                    return
                print(directive)
                if len(expected_directives) == 1:
                    self.success_finish()
                return directive
            return
        except Exception as e:
            Session.fuckit("Exception in test: %s" % (e,))
            return

    def success_finish(self):
        Logger.get().debug('success_finish')
        if self.data_generator:
            IOLoop.current().remove_timeout(self.data_generator)
            self.data_generator = None
            self.close_stream()
        IOLoop.current().call_later(5, IOLoop.current().stop)
