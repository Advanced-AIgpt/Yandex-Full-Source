#!/usr/bin/env python
from tornado.ioloop import IOLoop
import json
import uuid
from tests.basic import connect, server, Session
import pytest
from alice.uniproxy.library.settings import config


def test_yamb(server):
    connect(server, on_connect_yamb, on_message_yamb, 4)


def on_connect_yamb(socket):
    socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeState",
                "messageId": str(uuid.uuid4()),
            },
            "payload": {
                "auth_token": config["key"],
                "oauth_token": open("tests/test.token", "r").read().strip(),
                "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
            }
        }
    }))
    socket.write_message(json.dumps({
        "event": {
            "header": {
                "namespace": "Messaging",
                "name": "YambClientMessage",
                "messageId": str(uuid.uuid4()),
            },
            "payload": {
                "request": {
                    "params": {
                        'payload': {
                            'timezone': 180,
                            'text': 'some_text',
                            'message_id': 'some_message_id',
                            'chat_id': '0/0/067df275-f1f8-4358-bd94-cb716cbe2ade',
                            'type': 'message',
                        },
                        #"uid": "1130000000658130",
                    },
                    "method": "payload_proxy",
                },
            },
        }
    }))


def on_message_yamb(msg):
    try:
        if msg is None:
            Session.fuckit("Socket closed unexpected")
            return

        if len(msg) < 4:
            Session.fuckit("Something nasty")
            return
        elif isinstance(msg, bytes):
            Session.fuckit("Got bytes. Don't want bytes")
            return
        else:
            try:
                res = json.loads(msg)
                if "directive" not in res:
                    Session.fuckit("Got not a directive: %s" % (msg,))
                directive = res.get("directive").get("header", {}).get("namespace", "") + "." + \
                    res.get("directive").get("header", {}).get("name", "")
                if directive == "Messenger.BackendVersions":
                    return
                elif directive != "Messaging.YambResponse":
                    Session.fuckit("Wrong directive: %s" % (directive,))
                    return
                else:
                    IOLoop.current().stop()
                return
            except Exception as e:
                Session.fuckit("Exception in test: %s" % (e,))
    except Exception as e:
        Session.fuckit("Exception in test: %s" % (e,))
