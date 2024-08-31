#!/usr/bin/env python3
from __future__ import absolute_import
import functools
import datetime
import json
import uuid
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
    tts_check = False
    data_check = False
    vins_check = False
    error = ""

    @classmethod
    def all_fine(cls):
        return cls.tts_check and cls.data_check and cls.vins_check

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
        cls.fuckit("Timeout: tts_check: %s, data_check: %s, vins_check: %s" % (
            cls.tts_check,
            cls.data_check,
            cls.vins_check
        ))

    @classmethod
    def set_timeout(cls, timeout):
        cls._timeout = IOLoop.current().call_later(timeout, cls.timeout)


def text_request(text):
    # print(Session.uuid)
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
    # print("sent synchronize")

    file_format = 'audio/opus'

    Session.message_id = str(uuid.uuid4())

    now = datetime.datetime.now()

    Session.socket.write_message(json.dumps({
        "event": {
            "header": {
                "name": "TextInput",
                "messageId": Session.message_id,
                "namespace": "Vins"
            },
            "payload": {
                "application": {
                    "app_id": "uniproxy.monitoring",
                    "app_version": "1.2.3",
                    "os_version": "5.0",
                    "platform": "android",
                    "uuid": Session.uuid,
                    "lang": "ru-RU",
                    "client_time": now.strftime("%Y%m%dT%H%M%S"),
                    "timezone": "Europe/Moscow",
                    "timestamp": "%d" % (now.timestamp()),
                },
                "header": {
                    "request_id": Session.uuid,
                },
                "request": {
                    "additional_options": {
                        "bass_options": {
                            "filtration_level": 1,
                            "user_agent": "monitoring"
                        }
                    },
                    "event": {
                        "text": text,
                        "type": "text_input"
                    },
                    "experiments": [
                        ""
                    ],
                    "location": {
                        "accuracy": 140,
                        "lat": 52.26052093505859,
                        "lon": 104.1884078979492,
                        "recency": 192321
                    },
                    "reset_session": False,
                    "voice_session": True
                },
            }
        }
    }))


def on_message(msg):
    try:
        # print(msg)
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
                if directive == "TTS.Speak":
                    Session.tts_check = True
                elif directive == "Vins.VinsResponse":
                    Session.vins_check = True
                elif "StreamClosedError" in res.get("directive").get("payload", {}).get("error", {}).get("message", ""):
                    pass
                else:
                    print("Got bad directive: %s" % (msg))
                    sys.exit(42)
                return
            except Exception as e:
                print("Exception in test: %s" % (e))
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
    connect(functools.partial(text_request, u"Привет."), on_message)


if __name__ == "__main__":
    main()
