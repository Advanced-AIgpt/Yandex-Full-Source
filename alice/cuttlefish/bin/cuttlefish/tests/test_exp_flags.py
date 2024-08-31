import pytest
from .common import Cuttlefish, create_grpc_request
from .common.constants import ItemTypes, ServiceHandles
from .common.items import get_answer_item, create_message_header
from .common.utils import find_http_request_draft, get_synchronize_state_event
from alice.cuttlefish.library.protos.session_pb2 import (
    TRequestContext,
    TSessionContext,
)
from alice.cuttlefish.library.protos.wsevent_pb2 import TWsEvent
from alice.megamind.protos.speechkit.request_pb2 import TSpeechKitRequestProto
from alice.megamind.protos.common.experiments_pb2 import TExperimentsProto


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def test_without_exp_flags(cuttlefish: Cuttlefish):
    event = get_synchronize_state_event(
        cuttlefish, {"vins": {"application": {"device_model": "station"}}, "request": {"experiments": {}}}
    )

    response = cuttlefish.make_grpc_request(
        ServiceHandles.SYNCHRONIZE_STATE_PRE,
        create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {"type": ItemTypes.SYNCRHONIZE_STATE_EVENT, "data": event},
            ]
        ),
    )

    # DeviceModel has original value
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.DeviceInfo.DeviceModel == "station"

    assert find_http_request_draft(response, ItemTypes.FLAGS_JSON_HTTP_REQUEST) is None


# -------------------------------------------------------------------------------------------------
def test_request_context_exps_flags(cuttlefish: Cuttlefish):
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
                                        "experiments": {
                                            "telegram": "cloudy_district",
                                            "lang": "C++",
                                            "numeric_exp": 123456,
                                            "temp": "+24 C",
                                            "time": "18:00"
                                        }
                                    }
                                }
                            }
                        }""",
                    ),
                },
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        RequestBase=TSpeechKitRequestProto(
                            Request=TSpeechKitRequestProto.TRequest(
                                Experiments=TExperimentsProto(
                                    Storage={
                                        "old_exp_1": TExperimentsProto.TValue(String="old_val_1"),
                                        "old_exp_2": TExperimentsProto.TValue(Boolean=True),
                                    },
                                ),
                            ),
                        ),
                    ),
                },
            ]
        ),
    )

    req_ctx = get_answer_item(response, ItemTypes.REQUEST_CONTEXT, proto=TRequestContext)
    flags = req_ctx.ExpFlags

    assert set(flags.keys()) == {"telegram", "time", "numeric_exp", "lang", "temp", "old_exp_1", "old_exp_2"}
    assert flags["time"] == "18:00"
    assert flags["numeric_exp"] == "123456"
    assert flags["telegram"] == "cloudy_district"
    assert flags["lang"] == "C++"
    assert flags["temp"] == "+24 C"
    assert flags["old_exp_1"] == "old_val_1"
    assert flags["old_exp_2"] == "1"
