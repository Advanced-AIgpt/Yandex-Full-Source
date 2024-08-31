#!/usr/bin/env python3
from __future__ import absolute_import
import functools
import datetime
import json
import uuid
import struct
import sys
from tornado.websocket import websocket_connect
from tornado.ioloop import IOLoop
from alice.uniproxy.library.settings import config


class Session(object):
    _timeout = None
    terminate = False
    socket = None
    key = None
    uuid = 'f' * 16 + str(uuid.uuid4()).replace('-', '')[16:32]
    url = None
    data_check = False
    directive_check = False
    error = ""

    @classmethod
    def all_fine(cls):
        return cls.data_check and cls.directive_check

    @classmethod
    def fuckit(cls, msg):
        print(msg)
        IOLoop.current().stop()
        sys.exit(10)

    @classmethod
    def clear_timeout(cls):
        IOLoop.current().remove_timeout(cls._timeout)
        cls._timeout = None

    @classmethod
    def timeout(cls):
        cls.fuckit("Timeout: data_check: %s, directive_check: %s" % (
            cls.data_check,
            cls.directive_check,
        ))

    @classmethod
    def set_timeout(cls, timeout):
        cls._timeout = IOLoop.current().call_later(timeout, cls.timeout)


def file_request(audiofile, from_mime, to_mime):
    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeState",
                "messageId": str(uuid.uuid4()),
            },
            "payload": {
                "auth_token": Session.key,
                "uuid": Session.uuid,
                "lang": "ru-RU",
                "voice": "levitan",
            }
        }
    }))

    Session.message_id = str(uuid.uuid4())

    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "Audio",
                "name": "Convert",
                "messageId": Session.message_id,
                "streamId": 1,
            },
            "payload": {
                "from": from_mime,
                "to": to_mime,
            }
        }
    }))

    return convert_file(audiofile)


def convert_file(audiofile):
    while True:
        chunk = audiofile.read(4196)
        if not chunk:
            break
        prepend = struct.pack(">I", 1)
        Session.socket.write_message(prepend + chunk, binary=True)

    Session.socket.write_message(json.dumps({
        "streamcontrol": {
            "streamId": 1,
            "messageId": Session.message_id,
            "action": 0,
            "reason": 0,
        }
    }))


def on_message(msg):
    try:
        if msg is None:
            print("Socket closed")
            sys.exit(2)

        if len(msg) < 4:
            print("Got bad message")
            sys.exit(3)
        elif isinstance(msg, bytes):
            Session.data_check = True
            return
        else:
            try:
                res = json.loads(msg)
                if "streamcontrol" in res:
                    if res.get("streamcontrol", {}).get("streamId") != 1:
                        if Session.all_fine():
                            print("OK")
                            sys.exit(0)
                        else:
                            print(Session.error)
                            sys.exit(1)
                    else:
                        return
                elif "directive" not in res:
                    print("Got not a directive: %s" % (msg,))
                    sys.exit(4)
                directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                    res.get("directive").get("header", {}).get("name", "")
                if directive == "Audio.Stream":
                    Session.directive_check = True
                elif "StreamClosedError" in res.get("directive").get("payload", {}).get("error", {}).get("message", ""):
                    pass
                else:
                    print("Got bad directive: %s" % (msg))
                    sys.exit(42)
                return
            except Exception as test_exc:
                print("Exception in test: %s" % (test_exc))
                sys.exit(5)
    except Exception as exc:
        print("Exception: %s" % (exc,))
        sys.exit(6)


def connect(on_connect, on_message):
    def on_connect_save_socket(res):
        try:
            if res.exception():
                Session.fuckit("No connection to %s" % (Session.url,))
                return
            Session.socket = res.result()
        except Exception as exc:
            Session.fuckit("Exception on connection: %s" % (exc,))
            return
        on_connect()

    websocket_connect(Session.url, callback=on_connect_save_socket, on_message_callback=on_message)
    IOLoop.current().start()


def main(**kwargs):
    Session.key = kwargs.get("key", config.get("key", ""))
    Session.url = kwargs.get("url", "ws://localhost:%s/uni.ws" % (config.get("port", 80),))

    Session.set_timeout(kwargs.get("timeout", 15))

    audiofile = kwargs.get("file", open("/usr/lib/yandex/voice/uniproxy/monitoring/test.opus", "rb"))
    connect(functools.partial(
        file_request,
        audiofile,
        "audio/opus",
        "audio/x-pcm;bit=16;rate=16000"
    ), on_message)


if __name__ == "__main__":
    main()
