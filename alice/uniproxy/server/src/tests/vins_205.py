#!/usr/bin/env python
# encoding: utf-8

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
            "streamId": 1
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
            },
            "application": {
                "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                "timezone": "Europe/Moscow",
                "timestamp": "%d" % (datetime.datetime.now().timestamp()),
            },
            "lang": "ru-RU",
            "topic": "desktopgeneral",
            "format": "audio/x-pcm;bit=16;rate=16000",
            "key": "developers-simple-key",
            "advancedASROptions": {
                "partial_results": True,
                "utterance_silence": 120
            },
            "vins_partial": True,
            "force_eou": True,
        }
    }
}


def new_voice_input():
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
            yield write_stream(socket, file_name, 1, str(uuid.uuid4()), should_stop_streaming, chunk_size=6400, delay=0.4)
            complete()

        @gen.coroutine
        def _read():
            got_streamcontrol = False
            while True:
                # read partials
                message = yield read_message(socket)
                print('RECV FROM UNIPROXY:', message)
                if "streamcontrol" in message:
                    got_streamcontrol = True
                    assert message["streamcontrol"]["streamId"] == 1
                    stop_streaming()
                elif message["directive"]["header"]["namespace"] == "Messenger":
                    continue
                elif message["directive"]["payload"]["endOfUtt"]:
                    assert asr_text in message["directive"]["payload"]["recognition"][0]["normalized"]
                    break
            assert(got_streamcontrol)
            vins = (yield read_message(socket))
            if "streamcontrol" in vins:
                vins = (yield read_message(socket))
            assert vins["directive"]["header"]["name"] == "VinsResponse"
            vins_text = vins["directive"]["payload"]["response"]["card"]["text"]
            if check:
                assert re.match(".*(" + vins_should_answer + ").*", vins_text)
            assert (yield read_message(socket))["directive"]["header"]["name"] == "Speak"
            speech_len, stream_control = (yield read_stream(socket))
            assert stream_control["streamcontrol"]["streamId"] == 2
            assert speech_len > 2000
            complete()

        spawn_all(_write, _read)
    connect_and_run_ioloop(server, _run, None, 10)


def test_vins(server):
    start_time = time.time()
    VOICE_INPUT["event"]["payload"]["advancedASROptions"]["utterance_silence"] = 1600
    _test_voice_input(server, "tests/data/early_stop.wav", "стоп",  ".*", check=True)
    print("Elapsed", time.time() - start_time)
