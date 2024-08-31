import json
import logging
import requests
import os
import base64

import yatest.common as yc

from alice.library.python.eventlog_wrapper.log_dump_record import LogDumpRecord, ApphostEventlogDumpType
from alice.megamind.protos.common.data_source_type_pb2 import EDataSourceType
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest, TScenarioApplyRequest
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse, TScenarioApplyResponse, \
    TScenarioCommitResponse, TScenarioContinueResponse

logger = logging.getLogger(__name__)


def convert_header_pairs_to_dict(header_pairs):
    d = {}
    for pair in header_pairs:
        d[pair[0]] = pair[1]
    return d


class HttpBaseDump:
    def __init__(self, json_obj):
        self._headers = convert_header_pairs_to_dict(json_obj['headers'])
        self._content = json_obj['content']
        try:
            decoded_content = base64.b64decode(self._content)
            self._content = decoded_content
            try:
                self._content = self._content.decode()
            except:
                pass
        except:
            pass

    @property
    def headers(self):
        return self._headers

    @property
    def content(self):
        return self._content


class HttpRequestDump(HttpBaseDump):
    def __init__(self, json_obj):
        super().__init__(json_obj)
        self._path = json_obj['path']

    @property
    def path(self):
        return self._path


class HttpResponseDump(HttpBaseDump):
    def __init__(self, json_obj):
        super().__init__(json_obj)
        self._status_code = json_obj['status_code']

    @property
    def status_code(self):
        return self._status_code


class GrpcBaseDump:
    def __init__(self, json_obj):
        self._json_obj = json_obj

    @property
    def answers(self):
        return self._json_obj.get('answers', [])

    def get_json_answer(self, type):
        answer = self._get_answer(type)
        if not answer or answer.get('__content_type') != 'json':
            return None
        return answer.get('binary', {})

    def get_protobuf_answer(self, message_type, type):
        answer = self._get_answer(type)
        if not answer or answer.get('__content_type') != 'protobuf':
            return None
        proto = message_type()
        proto.ParseFromString(base64.b64decode(answer.get('binary', '')))
        return proto

    def _get_answer(self, type):
        for answer in self.answers:
            if answer.get('type') == type:
                return answer
        return None


class GrpcRequestDump(GrpcBaseDump):
    def __init__(self, json_obj):
        super().__init__(json_obj)


class GrpcResponseDump(GrpcBaseDump):
    def __init__(self, json_obj):
        super().__init__(json_obj)


class SourcesDump:
    def __init__(self, eventlog_json_lines):
        self._http_requests = {}
        self._http_responses = {}

        self._grpc_requests = {}
        self._grpc_responses = {}

        for eventlog_line in eventlog_json_lines.split('\n'):
            self._process_eventlog_line(eventlog_line)

    def _process_eventlog_line(self, eventlog_line):
        try:
            eventlog_obj = json.loads(eventlog_line)
        except:
            return
        event_body = eventlog_obj.get('EventBody')
        if not event_body:
            return

        event_type = event_body.get('Type', '')
        event_fields = event_body.get('Fields', {})
        source = event_fields.get('Source', '')
        json_obj = event_fields.get('Json', {})

        if event_type == 'TSourceRequest':
            try:
                self._http_requests[source] = HttpRequestDump(json_obj)
            except:
                pass
            try:
                self._grpc_requests[source] = GrpcRequestDump(json_obj)
            except:
                pass
        elif event_type in {'TSourceResponse', 'THttpSourceResponse'}:
            try:
                self._http_responses[source] = HttpResponseDump(json_obj)
            except:
                pass
            try:
                self._grpc_responses[source] = GrpcResponseDump(json_obj)
            except:
                pass

    @property
    def http_requests(self):
        return self._http_requests

    def get_http_request(self, node_name):
        return self._http_requests.get(node_name)

    @property
    def http_responses(self):
        return self._http_responses

    def get_http_response(self, node_name):
        return self._http_responses.get(node_name)

    @property
    def grpc_requests(self):
        return self._grpc_requests

    def get_grpc_request(self, node_name):
        return self._grpc_requests.get(node_name)

    @property
    def grpc_responses(self):
        return self._grpc_responses

    def get_grpc_response(self, node_name):
        return self._grpc_responses.get(node_name)


class DumpCollector:
    def __init__(self, apphost):
        self._apphost = apphost

    def extract_requests_and_responses_from_eventlog(self, scenario_name, request_id, flush_eventlog=True):
        logger.info(f'scenario_name={scenario_name}, request_id={request_id}, flush_eventlog={flush_eventlog}')
        if flush_eventlog:
            self._flush_app_host_eventlog()
        eventlog_json_lines = self._extract_eventlog_json_lines(request_id)
        requests = self._extract_requests(scenario_name, eventlog_json_lines)
        responses = self._extract_responses(scenario_name, eventlog_json_lines)
        sources_dump = SourcesDump(eventlog_json_lines)
        return requests, responses, sources_dump

    def extract_requests_from_eventlog(self, scenario_name, request_id, flush_eventlog=True):
        if flush_eventlog:
            self._flush_app_host_eventlog()
        eventlog_json_lines = self._extract_eventlog_json_lines(request_id)
        requests = self._extract_requests(scenario_name, eventlog_json_lines)
        return requests

    def _flush_app_host_eventlog(self):
        flush_url = f'http://localhost:{self._apphost.port}/admin?action=flush'
        apphost_flush_response = requests.get(flush_url)
        logger.info(f'{flush_url} returned {apphost_flush_response}')

    def _extract_eventlog_json_lines(self, request_id):
        eventlog_filepath = os.path.join(
            self._apphost.local_app_host_dir, 'ALICE', f'eventlog-{self._apphost.port}',
        )
        assert os.path.exists(eventlog_filepath), f'Eventlog not found {eventlog_filepath}'
        event_log_dump_binary = yc.binary_path('apphost/tools/event_log_dump/event_log_dump')
        cmd_params = [
            event_log_dump_binary,
            '--json',
            '--dump-mode', 'Json',
            '-c', f'TReqID:ID:{request_id}',
            '-i', 'TSourceRequest,TSourceResponse,THttpSourceResponse',
            eventlog_filepath,
        ]
        execute_result = yc.execute(
            cmd_params,
            stdout=yc.output_path('event_log_dump.out'),
            stderr=yc.output_path('event_log_dump.err'),
        )
        return execute_result.stdout.decode('utf-8')

    def _extract_requests(self, scenario_name, eventlog_json_lines):
        log_filters = self._make_log_filters(scenario_name)
        result = {}
        for eventlog_line in eventlog_json_lines.split('\n'):
            matched_filter = next((_ for _ in log_filters if _.log_filter in eventlog_line), None)
            if matched_filter:
                eventlog_obj = json.loads(eventlog_line)
                record = LogDumpRecord(eventlog_obj)
                if record.get_event_type() == ApphostEventlogDumpType.source_request:
                    result[matched_filter.scenario_stage] = self._extract_scenario_request(
                        record,
                        matched_filter.scenario_request_proto_class
                    )

        return result

    def _extract_responses(self, scenario_name, eventlog_json_lines):
        log_filters = self._make_log_filters(scenario_name)
        result = {}
        for eventlog_line in eventlog_json_lines.split('\n'):
            matched_filter = next((log_filter for log_filter in log_filters if log_filter.log_filter in eventlog_line),
                                  None)
            if matched_filter:
                eventlog_obj = json.loads(eventlog_line)
                record = LogDumpRecord(eventlog_obj)
                if record.get_event_type() == ApphostEventlogDumpType.source_response:
                    result[matched_filter.scenario_stage] = self._extract_scenario_response(
                        record,
                        matched_filter.scenario_response_proto_class
                    )

        return result

    def _make_log_filters(self, scenario_name):
        class LogFilter:
            def __init__(self, log_filter, scenario_stage, scenario_request_proto_class, scenario_response_proto_class):
                self.log_filter = log_filter
                self.scenario_stage = scenario_stage
                self.scenario_request_proto_class = scenario_request_proto_class
                self.scenario_response_proto_class = scenario_response_proto_class

        scenario_name_upper = scenario_name.upper()
        return [
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_RUN"', 'run',
                      TScenarioRunRequest, TScenarioRunResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_CONTINUE"', 'continue',
                      TScenarioApplyRequest, TScenarioContinueResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_APPLY"', 'apply',
                      TScenarioApplyRequest, TScenarioApplyResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_COMMIT"', 'commit',
                      TScenarioApplyRequest, TScenarioCommitResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_APP_HOST_COPY_RUN"', 'run',
                      TScenarioRunRequest, TScenarioRunResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_APP_HOST_COPY_CONTINUE"', 'continue',
                      TScenarioApplyRequest, TScenarioContinueResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_APP_HOST_COPY_APPLY"', 'apply',
                      TScenarioApplyRequest, TScenarioApplyResponse),
            LogFilter(f'"Source": "SCENARIO_{scenario_name_upper}_APP_HOST_COPY_COMMIT"', 'commit',
                      TScenarioApplyRequest, TScenarioCommitResponse),
        ]

    def _extract_scenario_request(self, log_dump_record, TScenarioXXXRequest):
        answer = log_dump_record.get_answer('mm_scenario_request')
        binary = base64.b64decode(answer.get_content_raw())
        data_sources_binaries = self._extract_datasource_item_binaries_from_eventlog_obj(log_dump_record)
        return self._scenario_request_binary_to_proto(binary,
                                                      data_sources_binaries,
                                                      TScenarioXXXRequest)

    def _extract_scenario_response(self, log_dump_record, TScenarioXXXResponse):
        answer = log_dump_record.get_answer('mm_scenario_response')
        binary = base64.b64decode(answer.get_content_raw())
        return self._scenario_response_binary_to_proto(binary, TScenarioXXXResponse)

    def _extract_datasource_item_binaries_from_eventlog_obj(self, log_dump_record):
        result = {}
        answers = log_dump_record.get_answers()
        for answer in answers:
            if not answer.get_type().startswith('datasource_'):
                continue
            data_source_binary = base64.b64decode(answer.get_content_raw())

            data_source_type = EDataSourceType.Value(answer.get_type()[len('datasource_'):])
            result[data_source_type] = data_source_binary
        return result

    def _scenario_request_binary_to_proto(self, scenario_request_binary, data_sources_binaries, TScenarioXXXRequest):
        # NOTE: We don't need mm_scenario_request_meta to reconstruct the request as it was in the old it2 tests
        scenario_request = TScenarioXXXRequest()
        scenario_request.ParseFromString(scenario_request_binary)
        for data_source_type, data_source_binary in data_sources_binaries.items():
            data_source = scenario_request.DataSources.get_or_create(data_source_type)
            data_source.ParseFromString(data_source_binary)
            if data_source_type == EDataSourceType.Value('GUEST_OPTIONS') and data_source.GuestOptions.HasField('OAuthToken'):
                logger.info('Obfuscating OAuth token in GUEST_OPTIONS datasource')
                data_source.GuestOptions.OAuthToken = '**OBFUSCATED**'
        return scenario_request

    def _scenario_response_binary_to_proto(self, scenario_response_binary, TScenarioXXXResponse):
        scenario_response = TScenarioXXXResponse()
        scenario_response.ParseFromString(scenario_response_binary)
        scenario_response.Version = 'trunk@******'  # Rewriting version because we want reproducible results
        return scenario_response
