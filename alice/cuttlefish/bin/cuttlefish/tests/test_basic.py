from .common import Cuttlefish, create_grpc_request
from .common.items import create_synchronize_state_event, get_answer_item
from .common.utils import find_http_request_draft
import logging
import pytest
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext
from alice.cuttlefish.library.python.testing.constants import ServiceHandles, ItemTypes


@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def test_invalid_synchronize_state(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(handle=ServiceHandles.SYNCHRONIZE_STATE_PRE, request=create_grpc_request())

    assert len(response.Answers) == 0
    assert len(response.Exception) > 0
    logging.info(f"TServiceResponse.Exception: {response.Exception}")


# -------------------------------------------------------------------------------------------------
def test_synchronize_state_pre(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SYNCRHONIZE_STATE_EVENT,
                    "data": create_synchronize_state_event(app_token="token-absent-in-whitelist"),
                },
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
            ]
        ),
    )

    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.AppId == "some.application.id"

    apikeys_http_request = get_answer_item(response, ItemTypes.APIKEYS_HTTP_REQUEST)
    logging.info(f"ApiKeys HTTP request: {apikeys_http_request}")

    assert find_http_request_draft(response, ItemTypes.FLAGS_JSON_HTTP_REQUEST) is None
