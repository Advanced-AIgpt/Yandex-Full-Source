from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.events.streamcontrol import StreamControl
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
import alice.uniproxy.library.global_state as global_state
from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.perf_tester.events as events
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


def get_metrics_dict():
    metrics = GlobalTimings.get_metrics()
    metrics_dict = {}
    for it in metrics:
        print('A'*40, it[0])
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def count_events(metric_name, metrics_dict):
    return sum([it[1] for it in metrics_dict[metric_name]])


@testing.ioloop_run
def ioloop_test_vins_apply_request_unistat(robot=False):
    reset_mocks()
    reset_wrappers()

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
                "uuid": "ffffffffffffffffffffffffffffffff" if robot else "afffffffffffffffffffffffffffffff",
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
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventLastVinsApplyRequestDurationSec.NAME) is None

    yield tornado.gen.sleep(0)  # give more time for spotter
    assert len(WrappedVinsApplyRequest.REQUESTS) == 1
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsApplyRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    yield FakeAsyncHttpStream.REQUESTS[1].return_result({
        "response": {
        }
    })
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventSynchronizeState.NAME) is not None
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventSynchronizeStateFinished.NAME) is not None
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventStartProcessVoiceInput.NAME) is not None
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventLastVinsApplyRequestDurationSec.NAME) is not None
    assert WrappedVoiceInput.PROCESSORS[0].vins_adapter.vins_timings.to_dict().get(events.EventStartVinsApplyRequest.NAME) is not None
    metrics_dict = get_metrics_dict()
    useful_requests = 0 if robot else 1
    assert count_events('useful_vins_apply_prepare_request_duration_hgram', metrics_dict) == useful_requests
    assert count_events('useful_vins_apply_request_duration_hgram', metrics_dict) == useful_requests
    assert count_events('vins_apply_request_duration_hgram', metrics_dict) == 1


def test_vins_apply_request_unistat(monkeypatch):
    global_state.GlobalState.init()
    UniproxyCounter.init()
    UniproxyTimings.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'PersonalDataHelper', FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YaldiStream', FakeYaldiStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'SpotterStream', FakeSpotterStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'get_yaldi_stream_type', fake_get_yaldi_stream_type)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, 'vins.voiceinput', WrappedVoiceInput)
    ioloop_test_vins_apply_request_unistat()
    UniproxyCounter.init()
    UniproxyTimings.init()
    ioloop_test_vins_apply_request_unistat(robot=True)
