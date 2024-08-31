import json
import pytest

from yatest import common as yc

from alice.library.python.eventlog_wrapper.eventlog_wrapper import ApphostEventlogWrapper, parse_eventlog_from_raw_response
from alice.library.python.eventlog_wrapper.log_dump_record import ApphostAnswerContentType

GENERAL_CONVERSATION_NODE = 'SCENARIO_GENERALCONVERSATION_RUN'
RANDOM_NUMBER_NODE = 'SCENARIO_RANDOMNUMBER_RUN'
UTTERANCE_POLYGLOT_HTTP_NODE = 'UTTERANCE_POLYGLOT_HTTP'


def _create_wrapper(megamind_test_eventlog_path: str) -> ApphostEventlogWrapper:
    with open(yc.source_path(megamind_test_eventlog_path), 'r') as f:
        return parse_eventlog_from_raw_response(f.read())


@pytest.mark.parametrize('megamind_test_eventlog_path', ['alice/library/python/eventlog_wrapper/tests/data/megamind_eventlog'])
class TestWrapper:

    @pytest.mark.parametrize('node_name, expected_response', [
        (GENERAL_CONVERSATION_NODE, 'kek_general_response'),
        (RANDOM_NUMBER_NODE, 'kek_random_response')
    ])
    def test_response(self, node_name: str, expected_response: str, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        assert len(wrapper.get_source_responses(node_name)) > 0
        response = wrapper.get_source_response_raw(
            node_name, 'mm_scenario_response')
        assert response == expected_response

    def test_http_response(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        http_response = wrapper.get_http_source_response(UTTERANCE_POLYGLOT_HTTP_NODE)
        assert http_response
        assert http_response.StatusCode == 200
        assert http_response.Content == b'kek_http_response_content'

    @pytest.mark.parametrize('node_name, expected_request', [
        (GENERAL_CONVERSATION_NODE, 'kek_general_request')
    ])
    def test_request(self, node_name: str, expected_request: str, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        assert len(wrapper.get_source_requests(node_name)) > 0
        request = wrapper.get_source_request_raw(
            node_name, 'mm_scenario_request')
        assert request == expected_request

    def test_response_record(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        log_record = wrapper.get_response_record(GENERAL_CONVERSATION_NODE)
        assert log_record
        assert log_record.is_supported_record()
        assert log_record.is_source_response()
        assert len(log_record.get_answers()) == 2
        scenario_response = log_record.get_answer('mm_scenario_response')
        assert scenario_response
        assert scenario_response.get_type() == 'mm_scenario_response'
        assert scenario_response.get_content_raw() == 'kek_general_response'
        assert scenario_response.get_content_type() == ApphostAnswerContentType.protobuf

    def test_request_record(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        log_record = wrapper.get_request_record(GENERAL_CONVERSATION_NODE)
        assert log_record
        assert log_record.is_supported_record()
        assert log_record.is_source_request()
        assert len(log_record.get_answers()) == 2
        scenario_request = log_record.get_answer('mm_scenario_request')
        assert scenario_request
        assert scenario_request.get_type() == 'mm_scenario_request'
        assert scenario_request.get_content_raw() == 'kek_general_request'

    def test_all_records_is_supported(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        for record in wrapper._records:
            assert record.is_supported_record()

    def test_hack_response(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        response = wrapper.get_source_response_raw(
            'HACK_TEST', 'hack_response_type')
        assert response
        assert response == 'hack_response'

    def test_get_raw_response_from_log_dump_record(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        log_record = wrapper.get_response_record('TEST_RAW_RESPONSE')
        assert log_record
        raw_event = log_record.raw_event
        expected_str = '''
            {
              "EventBody": {
                "Type": "TSourceResponse",
                "Fields": {
                  "Source": "TEST_RAW_RESPONSE",
                  "Json": "kek"
                }
              }
            }
        '''
        assert raw_event == json.loads(expected_str)

    def test_records_property(self, megamind_test_eventlog_path: str):
        wrapper = _create_wrapper(megamind_test_eventlog_path)
        assert len(wrapper.records)
