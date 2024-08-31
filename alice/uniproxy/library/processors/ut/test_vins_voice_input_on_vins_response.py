from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.perf_tester import events
import alice.uniproxy.library.processors.vins
from alice.uniproxy.library.testing.mocks import (
    FakeUniWebSocket,
    reset_mocks,
)
from alice.uniproxy.library.testing.wrappers import reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
import alice.uniproxy.library.vins.vinsrequest
from rtlog import null_logger
import tornado.gen
import uuid


class VoiceInputWrapper1(alice.uniproxy.library.processors.vins.VoiceInput):
    CALLED_BIO_COROUTINE = 0
    CALLED_SAY = 0

    @staticmethod
    def reset():
        VoiceInputWrapper1.CALLED_BIO_COROUTINE = 0
        VoiceInputWrapper1.CALLED_SAY = 0

    @tornado.gen.coroutine
    def create_or_update_user_coro(self, params):
        VoiceInputWrapper1.CALLED_BIO_COROUTINE += 1

    def say(self, what, uniproxy_tts_timings=None):
        VoiceInputWrapper1.CALLED_SAY += 1


def create_event_processor1(system, event, rt_log=null_logger(), **kwargs):
    return VoiceInputWrapper1(
        system=system,
        rt_log=rt_log or null_logger(),
        init_message_id=event.message_id,
        **kwargs
    )


@testing.ioloop_run
def ioloop_test_on_vins_response():
    """ VOICESERV-3498 test executing vins directives"""
    reset_mocks()
    reset_wrappers()
    VoiceInputWrapper1.reset()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {
            "biometry_classify": "gender,children",
            "need_scoring": True,
            "request": {
                "session": {}
            },
        },
    })
    web_socket = FakeUniWebSocket()
    system = web_socket.system
    processor = create_event_processor1(system, event)
    processor.payload_with_session_data.update({
        "request": {
            "experiments": {
                "uniproxy_vins_timings": 1
            }
        }
    })
    processor.event = event

    assert VoiceInputWrapper1.CALLED_BIO_COROUTINE == 0
    assert VoiceInputWrapper1.CALLED_SAY == 0
    assert processor._lag_storage._t0.get(events.EventFinishExecuteVinsDirectives.NAME) is None
    assert processor._lag_storage._t0.get(events.EventVinsResponseSent.NAME) is None
    yield processor.on_vins_response(
        raw_response={
            "a": "b"
        },
        what_to_say='абырвалг',
        vins_directives=[
            {
                'name': 'save_voiceprint',
            }
        ],
    )
    assert VoiceInputWrapper1.CALLED_BIO_COROUTINE == 1
    assert VoiceInputWrapper1.CALLED_SAY == 1
    assert processor._lag_storage._t0.get(events.EventFinishExecuteVinsDirectives.NAME) is not None
    assert processor._lag_storage._t0.get(events.EventVinsResponseSent.NAME) is not None


def test_on_vins_response(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    monkeypatch.setattr(alice.uniproxy.library.processors, 'create_event_processor', create_event_processor1)
    ioloop_test_on_vins_response()
    UniproxyCounter.init()
    UniproxyTimings.init()
