import os
import logging

from google.protobuf import text_format

import yatest.common as yc
from alice.hollywood.library.python.testing.it2.scenario_responses import ScenarioResponses
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest, TScenarioApplyRequest
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse, TScenarioApplyResponse, \
    TScenarioContinueResponse, TScenarioCommitResponse
from alice.hollywood.library.python.testing.dump_collector import DumpCollector
from alice.library.python.testing.megamind_request.input_dialog import AbstractInput, Text, Voice
from alice.hollywood.library.python.testing.megamind_requester import make_request_id_base, make_request_id
from alice.hollywood.library.python.testing.scenario_requester import ScenarioRequester
from alice.tests.library.vins_response import HollywoodResponse

logger = logging.getLogger(__name__)


class AliceTestsRunner:
    def __init__(self, servers, scenario_handle, srcrwr_params,
                 scenario_name, tests_data_path, test_path, oauth_token, evo_like, is_voice):
        logger.info('Initializing Alice in RUNNER mode...')
        self._scenario_requester = ScenarioRequester(servers.apphost, oauth_token)
        self._scenario_handle = scenario_handle
        self._srcrwr_params = srcrwr_params
        self._scenario_name = scenario_name
        self._alice_commands_count = 0
        self._requests_abs_path = yc.source_path(os.path.join(tests_data_path, test_path))
        self._scenario_runtime = servers.scenario_runtime
        self._request_id_base = make_request_id_base(app_preset='', test_name=test_path)
        self._dump_collector = DumpCollector(servers.apphost)
        self._evo_like = evo_like
        self._is_voice = is_voice

    def __call__(self, input_obj):
        if not isinstance(input_obj, AbstractInput):
            if self._is_voice:
                input_obj = Voice(input_obj)
            elif self._evo_like:
                input_obj = Text(input_obj)
            else:
                raise Exception(f'input_dialog is type={type(input_obj)}, expected an AbstractInput child instance')

        self._alice_commands_count += 1
        scenario_name = input_obj.scenario.name if input_obj.scenario else self._scenario_name
        scenario_handle = input_obj.scenario.handle if input_obj.scenario else self._scenario_handle
        run_request = self._find_current_request_for(scenario_handle, 'run')
        scenario_responses = self._request_scenario_runtime(run_request, scenario_name, scenario_handle)
        logger.info(f'{self._scenario_runtime.name} response #{self._alice_commands_count} is {str(scenario_responses)}')
        return HollywoodResponse(scenario_responses, scenario_name) if self._evo_like else scenario_responses

    def skip(self, hours=0, minutes=0, seconds=0):
        pass

    def update_current_screen(self, screen_name):
        pass

    def update_audio_player_current_stream_stream_id(self, stream_id):
        pass

    def update_audio_player_duration_ms(self, duration_ms):
        pass

    def grant_permissions(self, permission_name):
        pass

    def clear_session(self):
        pass

    def _find_current_request_for(self, scenario_handle, scenario_action):
        return self._find_request_for(self._alice_commands_count - 1, scenario_handle, scenario_action)

    def _find_request_for(self, alice_command_index, scenario_handle, scenario_action):
        request_filename = f'{alice_command_index:02d}_{scenario_handle}_{scenario_action}.pb.txt'
        with open(os.path.join(self._requests_abs_path, request_filename), 'r') as f:
            request = f.read()
        if scenario_action == 'run':
            request_proto = TScenarioRunRequest()
            text_format.Merge(request, request_proto)
        elif scenario_action in ['apply', 'continue', 'commit']:
            request_proto = TScenarioApplyRequest()
            text_format.Merge(request, request_proto)
        else:
            raise Exception(f'Unexpected scenario_action={scenario_action}')
        return request_proto

    def _request_scenario_runtime(self, scenario_run_request, scenario_name, scenario_handle):
        result = ScenarioResponses()
        result.run_response = self._scenario_requester.request(graph_name=f'{scenario_handle}_run',
                                                               request_proto=scenario_run_request,
                                                               scenario_stage='run',
                                                               response_cls=TScenarioRunResponse,
                                                               srcrwr_params=self._srcrwr_params)

        if result.run_response.HasField('ApplyArguments'):
            scenario_apply_request = self._find_current_request_for(scenario_handle, 'apply')
            result.apply_response = self._scenario_requester.request(graph_name=f'{scenario_handle}_apply',
                                                                     request_proto=scenario_apply_request,
                                                                     scenario_stage='apply',
                                                                     response_cls=TScenarioApplyResponse,
                                                                     srcrwr_params=self._srcrwr_params)

        if result.run_response.HasField('ContinueArguments'):
            scenario_continue_request = self._find_current_request_for(scenario_handle, 'continue')
            result.continue_response = self._scenario_requester.request(graph_name=f'{scenario_handle}_continue',
                                                                        request_proto=scenario_continue_request,
                                                                        scenario_stage='continue',
                                                                        response_cls=TScenarioContinueResponse,
                                                                        srcrwr_params=self._srcrwr_params)

        if result.run_response.HasField('CommitCandidate'):
            scenario_commit_request = self._find_current_request_for(scenario_handle, 'commit')
            result.commit_response = self._scenario_requester.request(graph_name=f'{scenario_handle}_commit',
                                                                      request_proto=scenario_commit_request,
                                                                      scenario_stage='commit',
                                                                      response_cls=TScenarioCommitResponse,
                                                                      srcrwr_params=self._srcrwr_params)

        request_id = make_request_id(self._request_id_base, self._alice_commands_count - 1)
        _, _, sources_dump = \
            self._dump_collector.extract_requests_and_responses_from_eventlog(scenario_name, request_id)
        result.sources_dump = sources_dump

        return result
