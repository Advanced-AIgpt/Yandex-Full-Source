from tests.mocks.yaldi_server import YaldiServerMock, VoiceGenerator, handle_yaldi_stream
from alice.uniproxy.library.extlog.mocks import SessionLogMock
import alice.uniproxy.library.testing

from processors.vins import VoiceInput
from unisystem import UniSystem
from alice.uniproxy.library.events import Event, StreamControl
from alice.uniproxy.library.global_counter import GlobalCounter
import voicetech.library.proto_api.yaldi_pb2 as protos
from rtlog import null_logger

import tornado.gen
from tornado.concurrent import Future

import logging
from datetime import timedelta
from uuid import uuid4


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')
GlobalCounter.init()
us = UniSystem(None)
us.srcrwr_headers = {"ASR": None}


def make_voice_input_event(
    lang="ru-RU",
    topic="dialoggeneral",
    message_id=None,
    stream_id=1,
    spotter=False,
    spotter_phrase="алиса",
    ignore_spotter=False
) :
    event = Event({
        "header": {
            "namespace": "VINS",
            "name": "VoiceInput",
            "messageId": str(uuid4()) if message_id is None else message_id,
            "streamId": stream_id
        },
        "payload": {
            "header": { "request_id": "" },
            "request": {
                "event": {
                    "type": "voice_input",
                    "voice_session": True
                }
            },
            "application": { "timezone": "Europe/Moscow" },
            "lang": lang,
            "topic": topic,
            "format": "audio/pcm16",
            "key": "developers-simple-key"
        }
    })
    if spotter:
        event.payload["enable_spotter_validation"] = True
        event.payload["spotter_phrase"] = spotter_phrase
        if ignore_spotter:
            event.payload["disable_spotter_validation"] = True
    return event


@alice.uniproxy.library.testing.ioloop_run
def test_without_spotter():
    with YaldiServerMock() as yaldi:
        handle_yaldi_stream(yaldi, timeout=timedelta(seconds=2))
        processor = VoiceInput(us, null_logger())

        processor.process_event(make_voice_input_event(spotter=False))

        processor.add_data(VoiceGenerator.add_data_response(words=["эй"], eou=False))
        yield tornado.gen.sleep(0.05)
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "алиса"], eou=False))
        yield tornado.gen.sleep(0.05)

        spotter = yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)
        assert spotter is None


@alice.uniproxy.library.testing.ioloop_run
def test_invalid_spotter():
    with YaldiServerMock() as yaldi:
        handle_yaldi_stream(yaldi, timeout=timedelta(seconds=2))

        # spotter validation is enabled and not ignored
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса"))
        processor.add_data(VoiceGenerator.add_data_response(words=["эй"], eou=False))
        yield tornado.gen.sleep(0.05)
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "лариса"], eou=True))
        yield tornado.gen.sleep(0.05)

        spotter = yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)
        assert spotter == False

        # spotter validation is enabled but ignored
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса", ignore_spotter=True))
        processor.add_data(VoiceGenerator.add_data_response(words=["эй"], eou=False))
        yield tornado.gen.sleep(0.05)
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "лариса"], eou=True))
        yield tornado.gen.sleep(0.05)

        spotter = yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)
        assert spotter == True


@alice.uniproxy.library.testing.ioloop_run
def test_logging_and_counting():
     with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi, timeout=timedelta(seconds=2))
        fail_counter = GlobalCounter.SPOTTER_VALIDATION_FAIL_SUMM
        ok_counter = GlobalCounter.SPOTTER_VALIDATION_OK_SUMM

        # successfull validation
        orig_counter_value = ok_counter.value()
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса"))
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "алиса"]))
        yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)

        record = session_log.records[-1]
        assert record["Directive"]["directive"]["payload"]["result"] == 1
        assert record["Directive"]["directive"]["payload"]["valid"] == 1
        assert ok_counter.value() == orig_counter_value + 1

        # failed validation
        orig_counter_value = fail_counter.value()
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса"))
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "лариса"]))
        yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)

        record = session_log.records[-1]
        assert record["Directive"]["directive"]["payload"]["result"] == 0
        assert record["Directive"]["directive"]["payload"]["valid"] == 0
        assert fail_counter.value() == orig_counter_value + 1

        # failed validation is ignored
        orig_counter_value = fail_counter.value()
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса", ignore_spotter=True))
        processor.add_data(VoiceGenerator.add_data_response(words=["эй", "лариса"]))
        yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)

        record = session_log.records[-1]
        assert record["Directive"]["directive"]["payload"]["result"] == 1
        assert record["Directive"]["directive"]["payload"]["valid"] == 0
        assert fail_counter.value() == orig_counter_value + 1


@alice.uniproxy.library.testing.ioloop_run
def test_valid_spotter():
    with YaldiServerMock() as yaldi:
        handle_yaldi_stream(yaldi, timeout=timedelta(seconds=2))
        processor = VoiceInput(us, null_logger())

        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="алиса"))

        processor.add_data(VoiceGenerator.add_data_response(words=["алиса"], eou=False))
        yield tornado.gen.sleep(0.05)
        processor.add_data(VoiceGenerator.add_data_response(words=["алиса", "эй"], eou=True))
        yield tornado.gen.sleep(0.05)

        spotter = yield tornado.gen.with_timeout(timedelta(seconds=1), processor.spotter_future)
        assert spotter == True


@alice.uniproxy.library.testing.ioloop_run
def test_topics_mapping_for_quasar():
    timeout = timedelta(seconds=2)
    init_request_topic = Future()

    def on_init_request(init_request : protos.InitRequest) -> protos.InitRequest:
        init_request_topic.set_result(init_request.topic)
        return protos.InitResponse(responseCode=protos.OK, hostname="localhost", topic="some-topic")

    with YaldiServerMock() as yaldi:
        handle_yaldi_stream(yaldi, timeout=timeout, on_init_request=on_init_request)

        # topic for "алис" phrase
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="эй алиса", topic="some-quasartopic"))

        topic = yield init_request_topic
        assert topic == "quasar-spotter-gpu"
        init_request_topic = Future()

        # topic for "яндекс" phrase
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="яндекс послушай", topic="somequasar-topic"))

        topic = yield init_request_topic
        assert topic == "dialog-general-gpu"
        init_request_topic = Future()

        # default topic
        processor = VoiceInput(us, null_logger())
        processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="иоанн", topic="somequasartopic-gpu"))

        topic = yield init_request_topic
        assert topic == "quasar-spotter-gpu"


@alice.uniproxy.library.testing.ioloop_run
def test_topics_mapping_for_newbies():
    timeout = timedelta(seconds=2)
    init_request_topic = Future()

    def on_init_request(init_request : protos.InitRequest) -> protos.InitRequest:
        init_request_topic.set_result(init_request.topic)
        return protos.InitResponse(responseCode=protos.OK, hostname="localhost", topic=init_request.topic)

    with YaldiServerMock() as yaldi:
        handle_yaldi_stream(yaldi, timeout=timeout, on_init_request=on_init_request)

        # mobile clients, desktop browser & anything
        for req_topic in ("dialog-general", "dialogeneral", "desktopgeneral", "very-strange-topic"):
            processor = VoiceInput(us, null_logger())
            processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="эй алиса", topic=req_topic))

            topic = yield init_request_topic
            assert topic == "dialog-general-gpu"
            init_request_topic = Future()

        # navigator & auto
        for req_topic in ("dialog-maps", "autolauncher"):
            processor = VoiceInput(us, null_logger())
            processor.process_event(make_voice_input_event(spotter=True, spotter_phrase="эй алиса", topic=req_topic))

            topic = yield init_request_topic
            assert topic == "dialogmapsgpu"
            init_request_topic = Future()

