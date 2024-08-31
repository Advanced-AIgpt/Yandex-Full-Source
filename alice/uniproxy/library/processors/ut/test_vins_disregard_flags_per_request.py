from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger

from alice.uniproxy.library.testing.mocks import (
    FakeAsyncHttpStream,
    FakeFlagsJsonClient,
    FakePersonalDataHelper,
    FakeRtLog,
    FakeUniWebSocket,
    FakeYabioStream,
    reset_mocks,
)

from alice.uniproxy.library.testing.wrappers import (
    WrappedVoiceInput,
    WrappedVinsRequest,
    WrappedVinsApplyRequest,
    reset_wrappers,
)

import alice.uniproxy.library.global_state as global_state
import alice.uniproxy.library.processors
import alice.uniproxy.library.vins.vinsadapter
import alice.uniproxy.library.vins.vinsrequest

from library.python import resource

import json
import uuid


@testing.ioloop_run
def ioloop_test(event_payload):
    reset_mocks()
    reset_wrappers()

    rt_log = FakeRtLog()
    web_socket = FakeUniWebSocket()
    system = web_socket.system

    ss = Event({
        "header": {
            "namespace": "System",
            "name": "SynchronizeState",
            "messageId": "ffffffff-ffff-ffff-ffff-ffffffffffff",
        },
        "payload": {
            "uaas_tests": [0],
            "auth_token": "developers-simple-key",
            "uuid": "ffffffffffffffffffffffffffffffff",
            "request": {
            },
        }
    })

    ss_processor = alice.uniproxy.library.processors.create_event_processor(system, ss, rt_log)
    ss_processor.flags_json_client_class = FakeFlagsJsonClient
    ss_processor.process_event(ss)

    stream_id = 1
    event_payload.setdefault("topic", "quasar-general-gpu")
    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()),
            "streamId": stream_id,
        },
        "payload": event_payload,
    })
    system.process_event(event, rt_log)


def init_environment(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, "PersonalDataHelper", FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, "YabioStream", FakeYabioStream)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, "VinsRequest", WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, "VinsApplyRequest", WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, "AsyncHttpStream", FakeAsyncHttpStream)
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, "vins.voiceinput", WrappedVoiceInput)


def test_vins_voice_input_regard_biometry_flag(monkeypatch):
    init_environment(monkeypatch)
    ioloop_test({
        "request": {
            "experiments": {
                "enable_biometry_scoring": "1",
                "disregard_uaas": "1",
            },
        },
    })
    assert len(FakeYabioStream.STREAMS) == 1
    assert FakeYabioStream.STREAMS[0].stream_type == FakeYabioStream.YabioStreamType.Score


def test_vins_voice_input_regard_biometry_flag_via_list(monkeypatch):
    init_environment(monkeypatch)
    ioloop_test({
        "request": {
            "experiments": [
                "enable_biometry_scoring",
                "disregard_uaas",
            ],
        },
    })
    assert len(FakeYabioStream.STREAMS) == 1
    assert FakeYabioStream.STREAMS[0].stream_type == FakeYabioStream.YabioStreamType.Score


def test_vins_voice_input_disregard_biometry_flag(monkeypatch):
    init_environment(monkeypatch)
    ioloop_test({
        "request": {
            "experiments": {
                "enable_biometry_scoring": "1",
            },
        },
    })
    assert len(FakeYabioStream.STREAMS) == 0


@testing.ioloop_run
def ioloop_test_local_exps(event_payload, key=None):
    reset_mocks()
    reset_wrappers()

    _macro_list = json.loads(resource.find("/vins_experiments.json"))
    _experiments_list = json.loads(resource.find("/experiments_rtc_production.json"))
    experiments = Experiments(_experiments_list, _macro_list, mutable_shares=True)

    rt_log = FakeRtLog()
    web_socket = FakeUniWebSocket()
    system = web_socket.system

    ss = Event({
        "header": {
            "namespace": "System",
            "name": "SynchronizeState",
            "messageId": "ffffffff-ffff-ffff-ffff-ffffffffffff",
        },
        "payload": {
            "uaas_tests": [0],
            "auth_token": "developers-simple-key",
            "uuid": "ffffffffffffffffffffffffffffffff",
            "request": {
            },
            "key": key,
        }
    })

    ss_processor = alice.uniproxy.library.processors.create_event_processor(system, ss, rt_log)
    ss_processor._experiments = experiments
    ss_processor.flags_json_client_class = FakeFlagsJsonClient
    ss_processor.process_event(ss)

    stream_id = 1
    event_payload.setdefault("topic", "quasar-general-gpu")
    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()),
            "streamId": stream_id,
        },
        "payload": event_payload,
    })

    vins_processor = alice.uniproxy.library.processors.create_event_processor(system, event, rt_log)
    vins_processor.process_event(event)


def test_vins_voice_input_regard_biometry_classify_flag(monkeypatch):
    init_environment(monkeypatch)
    ioloop_test_local_exps(
        {
            "request": {
                "experiments": {
                    "disregard_uaas": "1",
                },
            },
        },
        key="51ae06cc-5c8f-48dc-93ae-7214517679e6",
    )
    assert len(FakeYabioStream.STREAMS) == 1
    assert FakeYabioStream.STREAMS[0].stream_type == FakeYabioStream.YabioStreamType.Score
