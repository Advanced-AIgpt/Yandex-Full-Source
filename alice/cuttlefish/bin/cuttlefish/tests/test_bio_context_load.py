import pytest
from .common import Cuttlefish, create_grpc_request
from alice.cachalot.api.protos.cachalot_pb2 import (
    TYabioContextRequest,
    TYabioContextResponse,
    TYabioContextLoad,
    TYabioContextKey,
    TYabioContextSuccess,
    TYabioContextError,
    NOT_FOUND,
    INTERNAL_ERROR,
)
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item
from alice.megamind.protos.speechkit.directives_pb2 import TDirective as TSpeechkitDirective
from voicetech.library.proto_api.yabio_pb2 import YabioContext


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def make_speechkit_directive(name, payload=None):
    directive = TSpeechkitDirective()
    directive.Name = name
    if payload:
        directive.Payload.MergeFrom(payload)
    return directive


class TestBioContextLoadPost:

    DEFAULT_ITEMS = [
        {
            "type": ItemTypes.REQUEST_CONTEXT,
            "data": TRequestContext(
                AudioOptions=TAudioOptions(
                    Format="audio/opus",
                ),
                Header=TRequestContext.THeader(
                    SessionId="my-cool-session-id",
                    MessageId="my-cool-message-id",
                    StreamId=13,
                ),
            ),
        },
        {
            "type": ItemTypes.YABIO_CONTEXT_REQUEST,
            "data": TYabioContextRequest(
                Load=TYabioContextLoad(
                    Key=TYabioContextKey(
                        GroupId="test-group-id",
                        DevModel="test-dev-model",
                        DevManuf="test-dev=manuf",
                    ),
                ),
            ),
        },
    ]
    RESPONSE_EMPTY_CONTEXT = {
        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
        "data": TYabioContextResponse(
            Success=TYabioContextSuccess(
                # TODO:? Context = 1;
                Ok=True,
            ),
        ),
    }
    RESPONSE_WITH_CONTEXT = {
        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
        "data": TYabioContextResponse(
            Success=TYabioContextSuccess(
                Context=YabioContext(
                    group_id='test2-group-id',
                ).SerializeToString(),
                Ok=True,
            ),
        ),
    }
    RESPONSE_ERROR_NOT_FOUND_CONTEXT = {
        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
        "data": TYabioContextResponse(
            Error=TYabioContextError(
                Text="test-text",
                Status=NOT_FOUND,
            ),
        ),
    }
    RESPONSE_ERROR_INTERNAL = {
        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
        "data": TYabioContextResponse(
            Error=TYabioContextError(
                Text="test-text-2",
                Status=INTERNAL_ERROR,
            ),
        ),
    }

    def test_not_found_context1(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_LOAD_POST,
            create_grpc_request(items=self.DEFAULT_ITEMS + [self.RESPONSE_EMPTY_CONTEXT]),
        )
        assert len(response.Answers) == 1
        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT)
        assert req
        assert req.group_id == 'test-group-id'
        assert len(req.users) == 0
        assert len(req.enrolling) == 0

    def test_not_found_context2(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_LOAD_POST,
            create_grpc_request(items=self.DEFAULT_ITEMS + [self.RESPONSE_ERROR_NOT_FOUND_CONTEXT]),
        )
        assert len(response.Answers) == 1
        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT)
        assert req
        assert req.group_id == 'test-group-id'
        assert len(req.users) == 0
        assert len(req.enrolling) == 0

    def test_success(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_LOAD_POST,
            create_grpc_request(items=self.DEFAULT_ITEMS + [self.RESPONSE_WITH_CONTEXT]),
        )
        assert len(response.Answers) == 1
        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT)
        assert req
        assert req.group_id == 'test2-group-id'
        assert len(req.users) == 0
        assert len(req.enrolling) == 0

    def test_error(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_LOAD_POST,
            create_grpc_request(items=self.DEFAULT_ITEMS + [self.RESPONSE_ERROR_INTERNAL]),
        )
        assert len(response.Answers) == 1
        req = get_answer_item(response, ItemTypes.DIRECTIVE)
        assert req
        assert req.HasField('Exception')
