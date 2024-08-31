#!/usr/bin/env python
import sys

from tornado.ioloop import IOLoop
import json
import pytest
import functools
import struct
import uuid
from tests.basic import Session, server, connect
from alice.uniproxy.library.settings import config


_TIMEOUT = 0.1
_WAV_FILE = "tests/listen_yandex.wav"
_OGG_FILE = "tests/listen_yandex.ogg"


def test_convert_pcm2opus(server):
    global _TIMEOUT
    _TIMEOUT = 0.1
    wav_file = open(_WAV_FILE, "rb")
    connect(server, functools.partial(on_connect_convert, wav_file), on_message_convert, 7)


def on_connect_convert(test_file, socket):
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
                "namespace": "Audio",
                "name": "Convert",
                "messageId": message_id,
                "streamId": 1
            },
            "payload": {
                "from": "audio/x-pcm;bit=16;rate=16000",
                "to": "audio/ogg;codecs=opus",
            }
        }
    }
    socket.write_message(json.dumps(event))
    IOLoop.current().call_later(_TIMEOUT, functools.partial(send_more_data, test_file, 1, message_id))


_total_sent = 0


def send_more_data(test_file, stream_id, message_id):
    global _total_sent
    chunk = test_file.read(32000)
    _total_sent += len(chunk)
    if chunk:
        prepend = struct.pack(">I", stream_id)
        Session.socket().write_message(prepend + chunk, binary=True)
        IOLoop.current().call_later(_TIMEOUT, functools.partial(send_more_data, test_file, stream_id, message_id))
    else:
        Session.socket().write_message(json.dumps({
            "streamcontrol": {
                "streamId": stream_id,
                "action": 0,
                "reason": 0,
                "messageId": message_id
            }
        }))
        test_file.close()


_result = 0


def on_message_convert(msg, expected_directives=['Audio.Stream', ], ignored_directives=[]):
    global _result, _total_sent
    if msg is None:
        Session.fuckit("Socket closed unexpected")
        return

    if isinstance(msg, bytes):
        _result += len(msg) - 4
        return
    try:
        res = json.loads(msg)
        if "streamcontrol" in res:
            if _result < _total_sent / 8 or _result > _total_sent / 2:
                Session.fuckit("converted file doesn't look good")
            else:
                IOLoop.current().stop()
            return
        if "directive" not in res:
            Session.fuckit("Server returned unwanted message")
            return
    except Exception as e:
        Session.fuckit("Exception in test: %s" % (e,))
        return
