from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.events.streamcontrol import StreamControl
from alice.uniproxy.library.global_counter import GlobalCounter
import alice.uniproxy.library.global_state as global_state
from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.processors
from alice.uniproxy.library.testing.mocks import FakeRtLog, FakeUniWebSocket, FakeAsyncHttpStream, FakePersonalDataHelper
from alice.uniproxy.library.testing.mocks import FakeSpotterStream, FakeYaldiStream, fake_get_yaldi_stream_type, reset_mocks
from alice.uniproxy.library.testing.wrappers import WrappedVoiceInput, WrappedVinsRequest, WrappedVinsApplyRequest, reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
import alice.uniproxy.library.vins.vinsrequest
import datetime
import json
import tornado.gen
import tornado.ioloop
import uuid


@testing.ioloop_run
def ioloop_test_vins_voice_input_spotter_ok():
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
    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()),
            "streamId": stream_id,
        },
        "payload": {
            "topic": "quasar-general-gpu",
            "enable_spotter_validation": True,
        },
    })
    rt_log = FakeRtLog()
    system.process_event(event, rt_log)
    assert len(FakeSpotterStream.STREAMS) == 1
    assert len(FakeYaldiStream.STREAMS) == 0
    assert len(WrappedVoiceInput.PROCESSORS) == 1

    # apply end of spotter marker
    system.process_json_message(json.dumps({
        "streamcontrol": {
            "action": StreamControl.ActionType.SPOTTER_END,
            "messageId": "65d0b2d1-4d47-49cc-b52e-4479a9001365",
            "reason": 0,
            "streamId": stream_id,
        }
    }))
    assert len(FakeYaldiStream.STREAMS) == 1
    asr_result = {
        "endOfUtt": True,
        "recognition": [
            {
                "normalized": "Челябинская область",
                "words": [
                    {
                        "value": "челябинская область"
                    }
                ]
            }
        ]
    }
    # call VinsVoiceInput.on_spotter_result
    FakeYaldiStream.STREAMS[0].callback(asr_result)
    assert len(WrappedVinsRequest.REQUESTS) == 1  # got request to VINS

    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    yield FakeAsyncHttpStream.REQUESTS[0].return_result({
        "response": {
            "directives": [
                {
                    "type": "uniproxy_action",
                    "name": "defer_apply",
                    "payload": {
                        "session": {}
                    }
                },
            ]
        }
    })  # got eou VINS response with apply

    # set success spotter validation
    WrappedVoiceInput.PROCESSORS[0].on_spotter_result(True)
    yield tornado.gen.sleep(0)  # give time for executing coroutine blocked on spotter future

    # check processor run apply request
    assert WrappedVoiceInput.PROCESSORS[0]._spotter_validation_result is True
    yield tornado.gen.sleep(0)  # give a bit more time
    assert len(WrappedVinsApplyRequest.REQUESTS) == 1


def test_vins_voice_input_spotter_ok(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'PersonalDataHelper', FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YaldiStream', FakeYaldiStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'SpotterStream', FakeSpotterStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'get_yaldi_stream_type', fake_get_yaldi_stream_type)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, 'vins.voiceinput', WrappedVoiceInput)
    ioloop_test_vins_voice_input_spotter_ok()
