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
                "experiments": [],
                "session": {}
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
            "vins_partial": True
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
            yield write_stream(socket, file_name, 1, str(uuid.uuid4()), should_stop_streaming)
            complete()

        @gen.coroutine
        def _read():
            while True:
                # read partials
                message = yield read_message(socket)
                print(message)
                if message["directive"]["header"]["namespace"] == "Messenger":
                    continue
                if message["directive"]["payload"].get("endOfUtt", False):
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


def test_vins(server):
    start_time = time.time()
    VOICE_INPUT["event"]["payload"]["advancedASROptions"]["utterance_silence"] = 60
    _test_voice_input(
        server,
        "tests/data/fast.wav",
        [
            "быстрый правильный результат 5 + 5",
            "быстро правильный результат 5 + 5",
        ],
        ".*",
    )
    VOICE_INPUT["event"]["payload"]["advancedASROptions"]["utterance_silence"] = 120
    # _test_voice_input(server, "tests/data/long.wav", "слушать запрос курская область",  "Включаю")
    _test_voice_input(server, "tests/data/rec_long.wav", "какой курс доллара", "рубл")
    VOICE_INPUT["event"]["payload"]["vins_partial"] = False
    _test_voice_input(server, "tests/data/rec_long.wav", "какой курс доллара", "рубл")
    VOICE_INPUT["event"]["payload"]["vins_partial"] = True
    VOICE_INPUT["event"]["payload"]["application"]["app_id"] = "ru.yandex.quasar.services"
    # _test_voice_input(server, "tests/data/budilnik.wav", "поставь будильник на 10:35", "Не проспите|Хорошо|Готово", check=True)
    VOICE_INPUT["event"]["payload"]["application"]["app_id"] = "uniproxy.test"
    _test_voice_input(server, "tests/data/start.wav", "давай сыграем в города", "Отлично|Давайте|называете")
    _test_voice_input(server, "tests/data/rec.wav", "какой курс доллара", "Как-то|Кажется|похоже")
    _test_voice_input(server, "tests/data/mos.wav", "Москва", "А")
    _test_voice_input(server, "tests/data/mos.wav", "Москва", "А")
    _test_voice_input(server, "tests/data/kr.wav", "Краснодар", "так нельзя|не подходит|кажется|назвали")
    print("Elapsed", time.time() - start_time)
