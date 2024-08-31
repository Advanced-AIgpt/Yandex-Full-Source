from .common import Cuttlefish, create_grpc_request
from .common.constants import ItemTypes, ServiceHandles
from .common.items import get_answer_item, create_ws_message, create_message_header, create_event_exception
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TRequestContext
from alice.cuttlefish.library.protos.wsevent_pb2 import TWsEvent, TEventHeader
import json
import pytest


@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def test_parse_synchronize_state_with_invalid_json(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.CONVERT_IN,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": TWsEvent(
                        Header=create_message_header(
                            {"namespace": "System", "name": "SynchronizeState", "messageId": "14"}
                        ),
                        Text="""{
                        "event": {
                            "headers": {"namespace": "System", "name": "SynchronizeState", "messageId": "14"},
                            "payload": {X}
                        }
                    }""",
                    ),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    directive = get_answer_item(response, ItemTypes.DIRECTIVE)
    assert directive.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert directive.Header.Name == TEventHeader.EMessageName.EVENT_EXCEPTION
    assert directive.Header.RefMessageId == "14"
    assert directive.Exception.Text == "Incorrect JSON"


# -------------------------------------------------------------------------------------------------
def test_parse_synchronize_state_with_invalid_uuid(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.CONVERT_IN,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": create_ws_message(
                        {
                            "event": {
                                "header": {"namespace": "System", "name": "SynchronizeState", "messageId": "14"},
                                "payload": {
                                    "uuid": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaQaaaaa",  # invalid
                                    "auth_token": "some-vaid-auth-token",
                                },
                            }
                        }
                    ),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    directive = get_answer_item(response, ItemTypes.DIRECTIVE)
    assert directive.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert directive.Header.Name == TEventHeader.EMessageName.EVENT_EXCEPTION
    assert directive.Header.RefMessageId == "14"
    assert directive.Exception.Text == "Invalid uuid"


# -------------------------------------------------------------------------------------------------
def test_parse_synchronize_state(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.CONVERT_IN,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": create_ws_message(
                        {
                            "event": {
                                "header": {"namespace": "System", "name": "SynchronizeState", "messageId": "14"},
                                "payload": {
                                    "uuid": "aaaaaaaa-AAAA-aaaa-AAAA-aaaaaaaaaaaa",
                                    "auth_token": "some-vaid-app-token",
                                },
                            }
                        }
                    ),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    event = get_answer_item(response, ItemTypes.SYNCRHONIZE_STATE_EVENT)
    assert event.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert event.Header.Name == TEventHeader.EMessageName.SYNCHRONIZE_STATE
    assert event.SyncState.UserInfo.Uuid == "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
    assert event.SyncState.AppToken == "some-vaid-app-token"


# -------------------------------------------------------------------------------------------------
def test_event_exception_directive_to_ws_message(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.CONVERT_OUT,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.DIRECTIVE,
                    "data": create_event_exception(
                        message_id="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa", text="My lovely error message"
                    ),
                }
            ]
        ),
    )

    assert len(response.Answers) == 1
    msg = get_answer_item(response, ItemTypes.WS_MESSAGE)
    assert json.loads(msg.Text) == {
        "directive": {
            "header": {
                "namespace": "System",
                "name": "EventException",
                "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
            },
            "payload": {"error": {"message": "My lovely error message", "type": "Error"}},
        }
    }


# -------------------------------------------------------------------------------------------------
ver0_spotter_rms = '[[], [1, 2, 3], [22.8, 13.37]]'
ver1_spotter_rms = '{"version": 2021, "channels": [{"name": "Perviy Kanal", "data": [5, 4, 3]}, {"name": "Rossiya 2", "data": [0.1, 0.01, 0.001]}]}'


def check_ver0(data):
    data = data.Ver0.RmsData
    assert len(data) == 3
    assert data[0].Values == []
    assert data[1].Values == [1.0, 2.0, 3.0]
    assert data[2].Values == [22.8, 13.37]


def check_ver1(data):
    data = data.Ver1
    assert data.Version == 2021

    data = data.RmsData
    assert set(data) == {"Perviy Kanal", "Rossiya 2"}
    assert data["Perviy Kanal"].Values == [5.0, 4.0, 3.0]
    assert data["Rossiya 2"].Values == [0.1, 0.01, 0.001]


@pytest.mark.parametrize(
    "spotter_rms, checker",
    [
        (ver0_spotter_rms, check_ver0),
        (ver1_spotter_rms, check_ver1),
    ],
)
def test_request_context_speakers(cuttlefish: Cuttlefish, spotter_rms, checker):
    # Event Log.Spotter creates TRequestContext, check on this event
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.WS_ADAPTER_IN,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": TWsEvent(
                        Header=create_message_header({"namespace": "Log", "name": "Spotter", "messageId": "13"}),
                        Text="""{
                            "event": {
                                "header": {"namespace": "Log", "name": "Spotter", "messageId": "13"},
                                "payload": {
                                    "request": {
                                        "additional_options": {
                                            "speakers_count": 100500,
                                            "spotter_rms": """
                        + spotter_rms
                        + """
                                        }
                                    }
                                }
                            }
                        }""",
                    ),
                },
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(),
                },
            ]
        ),
    )

    req_ctx = get_answer_item(response, ItemTypes.REQUEST_CONTEXT, proto=TRequestContext)
    assert req_ctx.AdditionalOptions.SpeakersCount == 100500
    checker(req_ctx.AdditionalOptions.SpotterFeatures)


# -------------------------------------------------------------------------------------------------
def test_request_context_header(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.WS_ADAPTER_IN,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": TWsEvent(
                        Header=create_message_header(
                            {
                                "namespace": "Vins",
                                "name": "TextInput",
                                "messageId": "228",
                                "streamId": 13,
                                "refStreamId": 20,
                            }
                        ),
                        Text="""{
                            "event": {
                                "header": {"namespace": "Vins", "name": "TextInput", "messageId": "228", "streamId": 13, "refStreamId": 20},
                                "payload": {
                                }
                            }
                        }""",
                    ),
                },
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        SessionId="227",
                    ),
                },
            ]
        ),
    )

    req_ctx = get_answer_item(response, ItemTypes.REQUEST_CONTEXT, proto=TRequestContext)
    header = req_ctx.Header
    assert header.SessionId == "227"
    assert header.MessageId == "228"
    assert header.StreamId == 13
    assert header.RefStreamId == 20
    assert header.FullName == "vins.textinput"


# -------------------------------------------------------------------------------------------------
def test_streamed_speechkit_request_merge(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.WS_ADAPTER_IN,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.WS_MESSAGE,
                    "data": create_ws_message(
                        {
                            "event": {
                                "header": {
                                    "namespace": "Vins",
                                    "name": "TextInput",
                                    "messageId": "14",
                                },
                                "payload": {
                                    "request": {
                                        "activation_type": "big-red-button",
                                    },
                                    "header": {
                                        "request_id": "my-req-id",
                                    },
                                    "lishnee-pole": "nenyzhnoe",
                                    "session": "my-session",
                                    "memento": "my-memento",
                                },
                            }
                        }
                    ),
                },
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(),
                },
            ]
        ),
    )

    mm_request = get_answer_item(response, ItemTypes.MEGAMIND_REQUEST)
    base = mm_request.RequestBase
    assert base.Header.RequestId == "my-req-id"
    assert base.Request.ActivationType == "big-red-button"
    assert base.Session == "my-session"
    assert base.MementoData == "my-memento"
