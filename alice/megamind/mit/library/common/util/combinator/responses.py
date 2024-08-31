from alice.megamind.mit.library.common.names.item_names import AH_ITEM_COMBINATOR_RESPONSE
from alice.megamind.mit.library.util import get_dummy_proto
from alice.megamind.protos.scenarios.combinator_response_pb2 import (
    TCombinatorResponse,
)


def _generate_combinator_response_with_continue_args(continue_args=None) -> TCombinatorResponse:
    response = TCombinatorResponse()
    if not continue_args:
        continue_args = get_dummy_proto()
    response.Response.ContinueArguments.Pack(continue_args)
    return response


def _generate_combinator_response() -> TCombinatorResponse:
    response = TCombinatorResponse()
    return response


def generate_handler_combinator_response_with_continue_args(continue_args=None):
    def handler(ctx):
        ctx.add_protobuf_item(AH_ITEM_COMBINATOR_RESPONSE, _generate_combinator_response_with_continue_args(continue_args))
    return handler


def generate_handler_combinator_response():
    def handler(ctx):
        ctx.add_protobuf_item(AH_ITEM_COMBINATOR_RESPONSE, _generate_combinator_response())
    return handler
