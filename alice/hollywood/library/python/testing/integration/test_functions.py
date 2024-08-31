import logging

from google.protobuf import text_format
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse, TScenarioApplyResponse
from alice.megamind.protos.scenarios.response_pb2 import TScenarioCommitResponse, TScenarioContinueResponse
from alice.megamind.protos.scenarios.request_pb2 import TScenarioApplyRequest

logger = logging.getLogger(__name__)


def request_and_prepare_canondata_result(scenario_requester, scenario_run_request, scenario_handle, srcrwr_params={}):
    run_response, apply_response, commit_response, continue_response = request_all(
        scenario_requester, scenario_run_request, scenario_handle=scenario_handle, srcrwr_params=srcrwr_params)
    return prepare_canondata_result(run_response, apply_response, commit_response, continue_response)


def request_all(scenario_requester, scenario_run_request, scenario_handle, srcrwr_params={}):
    scenario_run_response = scenario_requester.request(graph_name=f'{scenario_handle}_run',
                                                       request_proto=scenario_run_request,
                                                       scenario_stage='run',
                                                       response_cls=TScenarioRunResponse,
                                                       srcrwr_params=srcrwr_params)
    scenario_apply_response = None
    scenario_commit_response = None
    scenario_continue_response = None

    if scenario_run_response.HasField('ApplyArguments'):
        scenario_apply_request = _prepare_scenario_apply_request(scenario_run_request,
                                                                 scenario_run_response.ApplyArguments)
        scenario_apply_response = scenario_requester.request(graph_name=f'{scenario_handle}_apply',
                                                             request_proto=scenario_apply_request,
                                                             scenario_stage='apply',
                                                             response_cls=TScenarioApplyResponse,
                                                             srcrwr_params=srcrwr_params)

    if scenario_run_response.HasField('ContinueArguments'):
        scenario_continue_request = _prepare_scenario_apply_request(scenario_run_request,
                                                                    scenario_run_response.ContinueArguments)
        scenario_continue_response = scenario_requester.request(graph_name=f'{scenario_handle}_continue',
                                                                request_proto=scenario_continue_request,
                                                                scenario_stage='continue',
                                                                response_cls=TScenarioContinueResponse,
                                                                srcrwr_params=srcrwr_params)

    if scenario_run_response.HasField('CommitCandidate'):
        scenario_commit_request = _prepare_scenario_apply_request(scenario_run_request,
                                                                  scenario_run_response.CommitCandidate.Arguments)
        scenario_commit_response = scenario_requester.request(graph_name=f'{scenario_handle}_commit',
                                                              request_proto=scenario_commit_request,
                                                              scenario_stage='commit',
                                                              response_cls=TScenarioCommitResponse,
                                                              srcrwr_params=srcrwr_params)

    return (scenario_run_response, scenario_apply_response, scenario_commit_response, scenario_continue_response)


def prepare_canondata_result(run_response, apply_response=None, commit_response=None, continue_response=None):
    canondata_result = '# TScenarioRunResponse:\n'
    canondata_result += text_format.MessageToString(run_response, as_utf8=True)
    if apply_response is not None:
        canondata_result += '\n# TScenarioApplyResponse:\n'
        canondata_result += text_format.MessageToString(apply_response, as_utf8=True)
    if commit_response is not None:
        canondata_result += '\n# TScenarioCommitResponse:\n'
        canondata_result += text_format.MessageToString(commit_response, as_utf8=True)
    if continue_response is not None:
        canondata_result += '\n# TScenarioContinueResponse:\n'
        canondata_result += text_format.MessageToString(continue_response, as_utf8=True)

    return canondata_result


def _prepare_scenario_apply_request(scenario_run_request, apply_arguments):
    scenario_apply_request = TScenarioApplyRequest()
    scenario_apply_request.Arguments.CopyFrom(apply_arguments)
    scenario_apply_request.BaseRequest.CopyFrom(scenario_run_request.BaseRequest)
    scenario_apply_request.Input.CopyFrom(scenario_run_request.Input)
    return scenario_apply_request
