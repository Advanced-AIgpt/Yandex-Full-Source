from tests.mocks.yaldi_server import YaldiServerMock, VoiceGenerator, handle_yaldi_stream
from alice.uniproxy.library.extlog.mocks import SessionLogMock
from tests.mocks.websocket import WebsocketMock
from tests.mocks import tvm_client_mock, subway_client_mock, no_lass
from tests.checks import match, ListMatcher

import voicetech.library.proto_api.yaldi_pb2 as protos
import alice.uniproxy.library.testing

from alice.uniproxy.library.global_counter import GlobalCounter

import tornado.gen
import logging
import uuid
import pytest


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')
GlobalCounter.init()


def make_voice_input_event(
    lang="ru-RU",
    topic="dialoggeneral",
    message_id=None,
    stream_id=1,
    spotter=False,
    spotter_phrase="алиса",
    ignore_spotter=False
) :
    payload = {
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
    if spotter:
        payload["enable_spotter_validation"] = True
        payload["spotter_phrase"] = spotter_phrase
        if ignore_spotter:
            payload["disable_spotter_validation"] = True

    return {
        "header": {
            "namespace": "VINS",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()) if (message_id is None) else message_id,
            "streamId": stream_id
        },
        "payload": payload
    }


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_get_asr_result():
    with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi)
        ws = WebsocketMock()

        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_voice_input_event(stream_id=1))
        ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            recognition=[
                protos.Result(confidence=0.9, normalized="привет алиса", words=[
                    protos.Word(confidence=0.9, value="привет"),
                    protos.Word(confidence=0.9, value="алиса")
                ])
            ]
        )))

        asr_result = yield ws.pop_directive()
        assert match(asr_result, {
            "header": {"namespace": "ASR", "name": "Result"},
            "payload": {
                "responseCode": "OK",
                "recognition": [{
                    "words": [{"value": "привет"},{"value": "алиса"}],
                    "normalized": "привет алиса"
                }],
                "endOfUtt": True
            }
        })


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_single_hypothesis_with_empty_normalized():
    """ Send any ASR.Result 'as is' if it contains only one hypothesis
    """

    with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi)

        for eou in (True, False):
            for normalized in ("", " ", "\t"):
                ws = WebsocketMock()
                ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
                ws.send_event(make_voice_input_event(stream_id=1))

                ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
                    responseCode=protos.OK,
                    endOfUtt=eou,
                    recognition=[
                        protos.Result(confidence=0.9, normalized=normalized, words=[])
                    ]
                )))

                asr_result = yield ws.pop_directive()
                assert match(asr_result, {
                    "header": {"namespace": "ASR", "name": "Result"},
                    "payload": {
                        "responseCode": "OK",
                        "endOfUtt": eou,
                        "recognition": [
                            {"normalized": normalized}
                        ]
                    }
                })


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_dont_check_partials():
    """ Send partial ASR.Result 'as is' whatever it contains
    """

    with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi)
        ws = WebsocketMock()
        ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
        ws.send_event(make_voice_input_event(stream_id=1))

        # first hypothesis contains empty "normalized"
        ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
            responseCode=protos.OK,
            recognition=[
                protos.Result(confidence=0.9, normalized="", words=[]),
                protos.Result(confidence=0.5, normalized="вал", words=[
                    protos.Word(confidence=0.5, value="вал")
                ]),
            ]
        )))
        asr_result = yield ws.pop_directive()
        assert match(asr_result, {"payload": {
            "endOfUtt": False,
            "recognition": [
                {"words": [], "normalized": ""},
                {"words": [{"value": "вал"}], "normalized": "вал"}
            ]
        }})

        # second hypothesis contains empty "normalized"
        ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
            responseCode=protos.OK,
            recognition=[
                protos.Result(confidence=0.9, normalized="привет", words=[
                    protos.Word(confidence=0.9, value="привет")
                ]),
                protos.Result(confidence=0.5, normalized="", words=[
                    protos.Word(confidence=0.5, value="валет")
                ]),
            ]
        )))
        asr_result = yield ws.pop_directive()
        assert match(asr_result, {"payload": {
            "endOfUtt": False,
            "recognition": [
                {"words": [{"value": "привет"}], "normalized": "привет"},
                {"words": [{"value": "валет"}], "normalized": ""}
            ]
        }})

        # second hypothesis contains whitespaced "normalized"
        ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
            responseCode=protos.OK,
            recognition=[
                protos.Result(confidence=0.9, normalized="привет а", words=[
                    protos.Word(confidence=0.9, value="привет"),
                    protos.Word(confidence=0.9, value="а")
                ]),
                protos.Result(confidence=0.5, normalized=" ", words=[
                    protos.Word(confidence=0.5, value="валет"),
                    protos.Word(confidence=0.5, value="а"),
                ]),
            ]
        )))
        asr_result = yield ws.pop_directive()
        assert match(asr_result, {"payload": {
            "endOfUtt": False,
            "recognition": [
                {"words": [{"value": "привет"}, {"value": "а"}], "normalized": "привет а"},
                {"words": [{"value": "валет"}, {"value": "а"}], "normalized": " "}
            ]
        }})

        # both hypothesis are fine
        ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
            responseCode=protos.OK,
            recognition=[
                protos.Result(confidence=0.9, normalized="привет алиса", words=[
                    protos.Word(confidence=0.9, value="привет"),
                    protos.Word(confidence=0.9, value="алиса")
                ]),
                protos.Result(confidence=0.5, normalized="валет лариса", words=[
                    protos.Word(confidence=0.5, value="валет"),
                    protos.Word(confidence=0.5, value="лариса"),
                ])
            ]
        )))
        asr_result = yield ws.pop_directive()
        assert match(asr_result, {"payload": {
            "endOfUtt": False,
            "recognition": [
                {"words": [{"value": "привет"}, {"value": "алиса"}], "normalized": "привет алиса"},
                {"words": [{"value": "валет"}, {"value": "лариса"}], "normalized": "валет лариса"}
            ]
        }})


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_eou__first_with_empty_normalized():
    """ For ASR.Result with EndOffUtt:
    don't drop first hypothesis with empty or whitespaced "normalized"
    """

    with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi)

        for normalized in ("", " ", "\t"):
            ws = WebsocketMock()
            ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
            ws.send_event(make_voice_input_event(stream_id=1))

            ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
                responseCode=protos.OK,
                endOfUtt=True,
                recognition=[
                    protos.Result(confidence=0.9, normalized=normalized, words=[]),
                    protos.Result(confidence=0.5, normalized="белый шум", words=[
                        protos.Word(confidence=0.5, value="белый"),
                        protos.Word(confidence=0.5, value="шум"),
                    ])
                ]
            )))
            asr_result = yield ws.pop_directive()
            assert match(asr_result, {
                "header": {"namespace": "ASR", "name": "Result"},
                "payload": {
                    "endOfUtt": True,
                    "recognition": [
                        {"normalized": normalized},
                        {"normalized": "белый шум"},
                    ]
                }
            })


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_eou__second_with_empty_normalized():
    """ For ASR.Result with EndOffUtt:
    drop not first hypothesis with empty or whitespaced "normalized"
    """

    with YaldiServerMock() as yaldi, SessionLogMock() as session_log:
        handle_yaldi_stream(yaldi)

        for normalized in ("", " ", "\t"):
            ws = WebsocketMock()
            ws.send_event({"header": {"name": "SynchronizeState", "namespace": "System"}})
            yield tornado.gen.sleep(0.1)

            ws.send_event(make_voice_input_event(stream_id=1))
            yield tornado.gen.sleep(0.1)

            ws.send_data(stream_id=1, data=VoiceGenerator.protobuf_message(protos.AddDataResponse(
                responseCode=protos.OK,
                endOfUtt=True,
                recognition=[
                    protos.Result(confidence=0.9, normalized="привет алиса", words=[]),
                    protos.Result(confidence=0.5, normalized=normalized, words=[])
                ]
            )))
            asr_result = yield ws.pop_directive()
            assert match(asr_result, {
                "header": {"namespace": "ASR", "name": "Result"},
                "payload": {
                    "endOfUtt": True,
                    "recognition": ListMatcher([{"normalized": "привет алиса"}], exact_length=True)
                }
            })
