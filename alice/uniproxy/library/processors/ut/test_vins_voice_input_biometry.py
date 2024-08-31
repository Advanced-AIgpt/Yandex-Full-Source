from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger

from alice.uniproxy.library.testing.mocks import (
    FakeAsyncHttpStream,
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

import json
import uuid


SCORING_RESULT_1 = {'result key': 'result value', 'partial_number': 0}


@testing.ioloop_run
def ioloop_test_vins_voice_input_biometry_flags(event_payload):
    reset_mocks()
    reset_wrappers()

    web_socket = FakeUniWebSocket()
    system = web_socket.system

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
                    "session": {}
                },
            }
        }
    }))

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
    rt_log = FakeRtLog()
    system.process_event(event, rt_log)


def get_fake_set_scoring_result(expected_res):

    def bar(smth, res, chunks):
        assert expected_res == res
    return bar


def init_environment(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, "PersonalDataHelper", FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, "YabioStream", FakeYabioStream)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, "VinsRequest", WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, "VinsApplyRequest", WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, "AsyncHttpStream", FakeAsyncHttpStream)
    monkeypatch.setattr(alice.uniproxy.library.vins.VinsAdapter, "set_scoring_result", get_fake_set_scoring_result(SCORING_RESULT_1))
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, "vins.voiceinput", WrappedVoiceInput)


def test_vins_voice_input_biometry_flags(monkeypatch):
    init_environment(monkeypatch)
    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "disable_biometry_scoring": "1",
                "disable_biometry_classify": "1",
            },
        },
    })
    assert len(FakeYabioStream.STREAMS) == 0

    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "enable_biometry_scoring": "1",
            },
        },
        "biometry_classify": "1",
    })
    assert len(FakeYabioStream.STREAMS) == 2
    assert (set(stream.stream_type for stream in FakeYabioStream.STREAMS) ==
            set([FakeYabioStream.YabioStreamType.Score, FakeYabioStream.YabioStreamType.Classify]))

    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "enable_biometry_scoring": "1",
            },
            "predefined_bio_scoring_result": SCORING_RESULT_1
        },
    })
    assert len(FakeYabioStream.STREAMS) == 0

    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "enable_biometry_classify": "1",
            },
            "predefined_bio_classify_result": {}
        },
    })
    assert len(FakeYabioStream.STREAMS) == 0

    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "disable_biometry_scoring": "1",
            },
        },
        "biometry_classify": "1",
    })
    assert len(FakeYabioStream.STREAMS) == 1
    assert FakeYabioStream.STREAMS[0].stream_type == FakeYabioStream.YabioStreamType.Classify

    ioloop_test_vins_voice_input_biometry_flags({
        "request": {
            "experiments": {
                "enable_biometry_scoring": "1",
                "disable_biometry_classify": "1",
            },
        },
    })
    assert len(FakeYabioStream.STREAMS) == 1
    assert FakeYabioStream.STREAMS[0].stream_type == FakeYabioStream.YabioStreamType.Score
