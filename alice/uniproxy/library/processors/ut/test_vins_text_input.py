from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalCounter
import alice.uniproxy.library.global_state as global_state
from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.processors
from alice.uniproxy.library.testing.mocks import FakeRtLog, FakeUniWebSocket, FakeAsyncHttpStream, FakePersonalDataHelper
from alice.uniproxy.library.testing.mocks import reset_mocks
from alice.uniproxy.library.testing.wrappers import WrappedTextInput, WrappedVinsRequest, WrappedVinsApplyRequest, reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
import alice.uniproxy.library.vins.vinsrequest
import datetime
import base64
import json
import tornado.gen
import tornado.ioloop
import uuid
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo


@testing.ioloop_run
def ioloop_test_vins_text_input():
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
            "name": "TextInput",
            "messageId": str(uuid.uuid4()),
            "streamId": stream_id,
        },
        "payload": {
            "topic": "quasar-general-gpu",
            "header": {
                "request_id": "ffffffffffffffffffffffffffffffff"
            },
            "request": {
                "event": {
                    "type": "text_input",
                    "text": "Кто такой Владимир Путин?",
                    "voice_session": True
                }
            },
        },
    })
    rt_log = FakeRtLog()
    system.process_event(event, rt_log)
    for future in system.opened_streams[event.stream_id].futures.values():
        yield future

    # call VinsVoiceInput.on_spotter_result
    assert len(WrappedVinsRequest.REQUESTS) == 1  # got request to VINS

    assert WrappedVinsRequest.REQUESTS[0]._request['request']['smart_home'] == 'fake_response'

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

    assert len(WrappedVinsApplyRequest.REQUESTS) == 1  # we MUST use apply request to VINS


def test_vins_text_input(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'PersonalDataHelper', FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, 'vins.textinput', WrappedTextInput)

    def fake_can_use_smart_home(*args, **kwargs):
        return True

    @tornado.gen.coroutine
    def fake_get_smart_home(*args, **kwargs):
        yield tornado.gen.sleep(0)
        return base64.b64encode(TIoTUserInfo().SerializeToString()).decode('utf-8'), 'fake_response'

    monkeypatch.setattr(alice.uniproxy.library.processors.vins.VinsProcessor, '_can_use_smart_home', fake_can_use_smart_home)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins.VinsProcessor, '_get_smart_home', fake_get_smart_home)

    ioloop_test_vins_text_input()
