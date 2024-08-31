import alice.uniproxy.library.testing.mocks
from alice.uniproxy.library.testing.mocks import (
    FakeRtLog,
    FakeUniWebSocket,
    reset_mocks,
)

from alice.uniproxy.library import testing
from alice.uniproxy.library.backends_asr.yaldistream import YaldiStream
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.processors.asr import _prepare_asr_result
from alice.uniproxy.library.processors.vins import VoiceInput
from alice.uniproxy.library.unisystem.unisystem import UniSystem

import alice.uniproxy.library.backends_asr.yaldistream
import alice.uniproxy.library.unisystem.unisystem

import voicetech.library.proto_api.yaldi_pb2 as protos

import copy
import json
import pytest
import uuid


g_stream_id = 1


class FakeYaldiStream(YaldiStream):
    def read_add_data_response(self, *args, **kwargs):
        self.on_data(proto_to_json(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            messagesCount=4,
            recognition=[
                protos.Result(confidence=0.7, normalized="привет алиса", words=[
                    protos.Word(confidence=0.7, value="привет"),
                    protos.Word(confidence=0.7, value="алиса")
                ]),
                protos.Result(confidence=0.3, words=[], normalized=" "),
            ]
        )))


class FakeUniSystem(UniSystem):
    def write_directive(self, directive, processor=None):
        if not hasattr(self, "_directives"):
            self._directives = list()
        self._directives.append(directive)


@testing.ioloop_run
def ioloop_test_empty_normalized_dropping(event_payload, is_quasar):
    reset_mocks()

    web_socket = FakeUniWebSocket()
    system = web_socket.system
    system.exps_check = True
    system.process_json_message(json.dumps({
        "event": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeState",
                "messageId": "ffffffff-ffff-ffff-ffff-ffffffffffff",
            },
            "payload": {
                "disable_local_experiments": True,
                "uaas_tests": [0],
                "auth_token": "developers-simple-key",
                "uuid": "ffffffffffffffffffffffffffffffff",
                "request": {
                    "experiments": ["disregard_uaas"],
                },
                "key": "51ae06cc-5c8f-48dc-93ae-7214517679e6" if is_quasar else "bla-bla",
            }
        }
    }))

    global g_stream_id
    g_stream_id += 2
    event_payload.setdefault("topic", "quasar-general-gpu")
    msg_id = str(uuid.uuid4())
    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": msg_id,
            "streamId": g_stream_id,
        },
        "payload": event_payload,
    })

    rt_log = FakeRtLog()
    system.process_event(event, rt_log)
    assert len(system._processors) == 1

    proc = system._processors[f"vins.voiceinput-{msg_id}"]
    assert isinstance(proc, VoiceInput)
    assert "asr" in proc.streaming_backends

    yaldi = proc.streaming_backends["asr"]
    assert isinstance(yaldi, FakeYaldiStream)

    yaldi.read_add_data_response()

    count = 0
    for directive in system._directives:
        if directive.namespace == "ASR" and directive.name == "Result":
            count += len(directive.payload["recognition"])

    assert count == 1


def init_environment(monkeypatch):
    GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.testing.mocks.uni_web_socket, "UniSystem", FakeUniSystem)
    monkeypatch.setattr(alice.uniproxy.library.backends_asr.yaldistream, "YaldiStream", FakeYaldiStream)


@pytest.mark.parametrize("is_quasar", [True, False])
def test_empty_normalized_dropping(monkeypatch, is_quasar):
    init_environment(monkeypatch)
    ioloop_test_empty_normalized_dropping(
        event_payload={
            "request": {
            },
        },
        is_quasar=is_quasar,
    )


ASR_RESULT_EXAMPLE = {
    "recognition": [
        {
            "confidence": 0.6,
            "words": [
                {
                    "confidence": 0.6,
                    "value": "привет",
                },
                {
                    "confidence": 0.6,
                    "value": "алиса",
                },
            ],
            "normalized": "привет алиса",
            "endOfPhrase": False,
            "parentModel": "",
        },
        {
            "confidence": 0.2,
            "normalized": " ",
            "words": [],
            "endOfPhrase": False,
            "parentModel": "",
        },
        {
            "confidence": 0.1,
            "normalized": "",
            "words": [
                {
                    "confidence": 0.05,
                    "value": "нет",
                },
                {
                    "confidence": 0.05,
                    "value": "бориса",
                },
            ],
            "endOfPhrase": False,
            "parentModel": "",
        },
        {
            "confidence": 0.05,
            "words": [
                {
                    "confidence": 0.05,
                    "value": "нет",
                },
                {
                    "confidence": 0.05,
                    "value": "бориса",
                },
            ],
            "normalized": "нет бориса",
            "endOfPhrase": False,
            "parentModel": "",
        },
        {
            "confidence": 0.05,
            "normalized": "\n",
            "words": [],
            "endOfPhrase": False,
            "parentModel": "",
        },
    ],
    "responseCode": "OK",
    "endOfUtt": True,
    "messagesCount": 4,
    "bioResult": [],
    "earlyEndOfUtt": False,
    "contextRef": [],
    "durationProcessedAudio": "0",
    "isTrash": False,
    "trashScore": 0.0,
    "coreDebug": "",
    "thrownPartialsFraction": 0.0,
    "asr_partial_number": 0,
}


def test_prepare_asr_result_1():
    data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data["recognition"].pop(4)
    prepared_data["recognition"].pop(2)
    prepared_data["recognition"].pop(1)

    assert _prepare_asr_result(data, True, False) == prepared_data
    assert len(data["recognition"]) == 2


def test_prepare_asr_result_2():
    data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data["recognition"].pop(4)
    prepared_data["recognition"].pop(2)
    prepared_data["recognition"].pop(1)

    assert _prepare_asr_result(data, False, False) == prepared_data
    assert len(data["recognition"]) == 5


def test_prepare_asr_result_3():
    data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data["recognition"].pop(4)
    prepared_data["recognition"].pop(1)

    result = _prepare_asr_result(data, True, True)
    from sys import stderr as st
    st.write('\nRES: {}\n'.format(result))
    st.flush()

    assert _prepare_asr_result(data, True, True) == prepared_data
    assert len(data["recognition"]) == 3


def test_prepare_asr_result_4():
    data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data = copy.deepcopy(ASR_RESULT_EXAMPLE)
    prepared_data["recognition"].pop(4)
    prepared_data["recognition"].pop(1)

    assert _prepare_asr_result(data, False, True) == prepared_data
    assert len(data["recognition"]) == 5
