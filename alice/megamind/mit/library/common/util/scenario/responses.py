from alice.megamind.mit.library.common.names.item_names import AH_ITEM_SCENARIO_RESPONSE
from alice.megamind.mit.library.util import get_dummy_proto
from alice.megamind.protos.scenarios.response_pb2 import (
    TScenarioRunResponse,
    TScenarioApplyResponse,
)


def _generate_run_response_with_apply_arguments(apply_args=None) -> TScenarioRunResponse:
    response = TScenarioRunResponse()
    if not apply_args:
        apply_args = get_dummy_proto()
    response.ApplyArguments.Pack(apply_args)
    return response


def _generate_run_response_with_continue_arguments(continue_args=None) -> TScenarioRunResponse:
    response = TScenarioRunResponse()
    if not continue_args:
        continue_args = get_dummy_proto()
    response.ContinueArguments.Pack(continue_args)
    return response


def _generate_apply_response_with_utterance(utterance) -> TScenarioApplyResponse:
    response = TScenarioApplyResponse()
    response.ResponseBody.Layout.OutputSpeech = utterance
    return response


def generate_handler_run_response_with_apply_args(apply_args=None):
    def handler(ctx):
        ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, _generate_run_response_with_apply_arguments(apply_args))
    return handler


def generate_handler_apply_response_with_output_speech(utterance):
    def handler(ctx):
        ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, _generate_apply_response_with_utterance(utterance))
    return handler


def generate_handler_run_response_with_continue_args(continue_args=None):
    def handler(ctx):
        ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, _generate_run_response_with_continue_arguments(continue_args))
    return handler
