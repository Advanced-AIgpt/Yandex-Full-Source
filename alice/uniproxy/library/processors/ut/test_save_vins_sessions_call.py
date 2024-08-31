from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalCounter
import alice.uniproxy.library.global_state as global_state
from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.processors
from alice.uniproxy.library.testing.mocks import FakeRtLog, FakeUniWebSocket, FakeAsyncHttpStream, FakePersonalDataHelper
from alice.uniproxy.library.testing.mocks import FakeSpotterStream, FakeYaldiStream, fake_get_yaldi_stream_type, reset_mocks
from alice.uniproxy.library.testing.wrappers import WrappedVoiceInput, WrappedVinsRequest, WrappedVinsApplyRequest, reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
import alice.uniproxy.library.vins.vinsrequest
import base64
from cityhash import hash64 as CityHash64
import json
from tornado.concurrent import Future
import uuid


class FakeVinsContextAccessor:
    def __init__(self):
        self.save_finished = Future()

    async def save_base64(self, sessions):
        self.save_finished.set_result(True)

    async def save_base64_ex(self, items):
        items_bytes = {}
        items_infos = {}
        for k, v in items.items():
            data = base64.b64decode(v)
            items_bytes[k] = data
            items_infos[k] = {
                'size': len(data) if data else 0,
                'hash': CityHash64(data) if data else 0,
            }
        self.save_finished.set_result(True)
        return items_infos


@testing.ioloop_run
def ioloop_test_save_vins_sessions_call():
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
    processor = system.process_event(event, rt_log)
    assert len(FakeSpotterStream.STREAMS) == 1
    assert len(FakeYaldiStream.STREAMS) == 0
    assert len(WrappedVoiceInput.PROCESSORS) == 1
    processor.vins_context_accessor = FakeVinsContextAccessor()
    processor.stateless = False
    raw_response = {
        "sessions": {
            "session": "data"
        }
    }
    yield processor.on_vins_response(raw_response=raw_response)
    yield processor.vins_context_accessor.save_finished
    # save called, - SUCCESS!


def test_save_vins_sessions_call(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)
    monkeypatch.setattr(alice.uniproxy.library.vins_context_storage, 'VinsContextAccessor', FakeVinsContextAccessor)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'PersonalDataHelper', FakePersonalDataHelper)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YaldiStream', FakeYaldiStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'SpotterStream', FakeSpotterStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'get_yaldi_stream_type', fake_get_yaldi_stream_type)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setitem(alice.uniproxy.library.processors.__event_processors, 'vins.voiceinput', WrappedVoiceInput)
    ioloop_test_save_vins_sessions_call()
