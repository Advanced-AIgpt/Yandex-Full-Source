#!/usr/bin/env python
# encoding: utf-8

import pytest
from tornado.ioloop import IOLoop
from tornado import gen
import functools
import uuid
import json
import re
import datetime
import time
from alice.uniproxy.library.settings import config
from tests.basic import connect as connect_and_run_ioloop, server, exit_ioloop, spawn_all
from tests.basic import write_json, write_data, read_message, write_stream, read_stream
from tests.mock_backend import mock_contacts
import unisystem


SYNC_STATE = {
    "event": {
        "header": {
            "namespace": "System",
            "name": "SynchronizeState",
            "messageId": str(uuid.uuid4()),
        },
        "payload": {
            "auth_token": config["key"],
            "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
            "vins": {
                "application": {
                    "app_id": "uniproxy.test",
                    "device_manufacturer": "Yandex",
                    "app_version": "1.2.3",
                    "os_version": "6.0.1",
                    "device_model": "Station",
                    "platform": "android",
                    "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
                    "lang": "ru-RU",
                },
            },
        }
    }
}


VOICE_INPUT = {
    "event": {
        "header": {
            "namespace": "VINS",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()),
            "streamId": 1,
        },
        "payload": {
            "header": {
                "request_id": str(uuid.uuid4()),
                "state_id": 0,
                "try_id": 0
            },
            "request": {
                "event": {
                    "type": "voice_input",
                    "voice_session": True,
                },
                "experiments": [
                    'contact_asr_help',
                    'e2e_merge_always_second',
                ],
                "session": {}
            },
            "application": {
                "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                "timezone": "Europe/Moscow",
                "timestamp": "%d" % (datetime.datetime.now().timestamp()),
            },
            "lang": "ru-RU",
            "topic": "dialogeneral+dialog-general-gpu",
            "format": "audio/x-pcm;bit=16;rate=16000",
            "key": "developers-simple-key",
            "advancedASROptions": {
                "partial_results": True,
            },
            "asr_balancer": "yaldi.alice.yandex.net",
            "vins_partial": True
        }
    }
}


def new_voice_input():
    VOICE_INPUT["event"]["header"]["messageId"] = str(uuid.uuid4())
    VOICE_INPUT["event"]["payload"]["header"]["state_id"] += 1
    return VOICE_INPUT


def _test_voice_input(server, file_name, asr_text, vins_should_answer, check=False):
    def _run(socket):
        _stop_streaming = False
        _complete = 0

        def stop_streaming():
            nonlocal _stop_streaming
            _stop_streaming = True

        def should_stop_streaming():
            nonlocal _stop_streaming
            return _stop_streaming

        def complete():
            nonlocal _complete
            _complete += 1
            if _complete == 2:
                exit_ioloop()

        @gen.coroutine
        def _write():
            write_json(socket, SYNC_STATE)
            write_json(socket, new_voice_input())
            yield write_stream(socket, file_name, 1, str(uuid.uuid4()), should_stop_streaming, chunk_size=16000, delay=0.5)
            complete()

        @gen.coroutine
        def _read():
            while True:
                # read partials
                message = yield read_message(socket)
                if message["directive"]["header"]["namespace"] == "Messenger":
                    continue
                if message["directive"]["payload"]["endOfUtt"]:
                    if isinstance(asr_text, (list, tuple)):
                        assert message["directive"]["payload"]["recognition"][0]["normalized"] in asr_text
                    else:
                        assert message["directive"]["payload"]["recognition"][0]["normalized"] == asr_text
                    break
            stop_streaming()
            assert (yield read_message(socket))["streamcontrol"]["streamId"] == 1
            vins = (yield read_message(socket))
            assert vins["directive"]["header"]["name"] == "VinsResponse"
            vins_text = vins["directive"]["payload"]["response"]["card"]["text"]
            #TODO: remove when vins will fix it's logic
            if check:
                assert re.match(".*(" + vins_should_answer + ").*", vins_text)
            assert (yield read_message(socket))["directive"]["header"]["name"] == "Speak"
            speech_len, stream_control = (yield read_stream(socket))
            assert stream_control["streamcontrol"]["streamId"] == 2
            assert speech_len > 2000
            complete()

        spawn_all(_write, _read)
    connect_and_run_ioloop(server, _run, None, 25)

testdata = [
    ("tests/data/contacts_set/1.wav", [ "позвони низару жозе", "позвони нижару хо и дезе" ]),
    ("tests/data/contacts_set/2.wav", [ "набери шок катценберг", ]),
    ("tests/data/contacts_set/3.wav", [ "позвони андрею плахову", "позвони андрею плахова", ]),
    ("tests/data/contacts_set/4.wav", [ "позвонить дмитрию конягину", "позвони дмитрию конягина", "позвони дмитрию кониагину", ]),
    ("tests/data/contacts_set/5.wav", [ "позвонить черил шульц", "позвонить черил шултз",]),
    ("tests/data/contacts_set/6.wav", [ "позвони анастасии звенко", "позвони анастасии зенкову", "позвони анастасии вику", ]),
    ("tests/data/contacts_set/7.wav", [ "позвони юлии бабович", ]),
    ("tests/data/contacts_set/8.wav", [ "набери александра шульгина", ]),
    ("tests/data/contacts_set/9.wav", [ "позвони диме сиянову", ]),
    ("tests/data/contacts_set/10.wav", [ "позвони маше шиковская", ]),
    ("tests/data/contacts_set/call_to_alex.wav", [ "позвони долгавину алексею", ]),
]


def uni_get_oauth_token(unused):
    return 'oauthtoken'


@pytest.mark.parametrize("sound,answer", testdata)
def test_success(server, monkeypatch, sound, answer):

    start_time = time.time()
    mock_contacts(monkeypatch)
    monkeypatch.setattr(unisystem.UniSystem, 'get_oauth_token', uni_get_oauth_token)

    _test_voice_input( server, sound, answer, ".*")

    print("Elapsed", time.time() - start_time)


def test_contact_timeout(server, monkeypatch):
    start_time = time.time()
    mock_contacts(monkeypatch, timeout=1.0)
    monkeypatch.setattr(unisystem.UniSystem, 'get_oauth_token', uni_get_oauth_token)

    _test_voice_input( server, "tests/data/contacts_set/2.wav", "набери шок катценберг", ".*")

    print("Elapsed", time.time() - start_time)
