import logging
import json

from typing import List, Optional, Tuple

from alice.library.python.eventlog_wrapper.log_dump_record import ApphostAnswerStub, Json, LogDumpRecord, THttpResponse


logger = logging.getLogger(__name__)

EVENT_KEY = 'Event'


class ApphostEventlogWrapper:

    def __init__(self, records: List[LogDumpRecord]):
        self._records: List[LogDumpRecord] = records

    def get_response_record(self, source_name: str) -> Optional[LogDumpRecord]:
        for record in self._records:
            if record.is_source_response() and record.get_source_name() == source_name:
                return record
        return None

    def get_request_record(self, source_name: str) -> Optional[LogDumpRecord]:
        for record in self._records:
            if record.is_source_request() and record.get_source_name() == source_name:
                return record
        return None

    def get_http_response_record(self, source_name: str) -> Optional[LogDumpRecord]:
        for record in self._records:
            if record.is_http_source_response() and record.get_source_name() == source_name:
                return record
        return None

    def get_source_responses(self, source_name: str) -> Optional[List[ApphostAnswerStub]]:
        record = self.get_response_record(source_name)
        return record.get_answers() if record else None

    def get_source_response_raw(self, source_name: str, type_name: str) -> Json:
        record = self.get_response_record(source_name)
        return record.get_source_response_raw(type_name) if record else None

    def get_source_requests(self, source_name: str) -> Optional[List[ApphostAnswerStub]]:
        record = self.get_request_record(source_name)
        return record.get_answers() if record else None

    def get_source_request_raw(self, source_name: str, type_name: str) -> Json:
        record = self.get_request_record(source_name)
        return record.get_source_request_raw(type_name) if record else None

    def get_http_source_response(self, source_name: str) -> Optional[THttpResponse]:  # type: ignore
        record = self.get_http_response_record(source_name)
        return record.get_http_source_response() if record else None

    @property
    def records(self) -> List[LogDumpRecord]:
        return self._records


def split_raw_response_to_http_response_and_eventlog(raw_response: str, delimiter: str = '//DEBUGINFO') -> Tuple[Json, Json]:
    items = raw_response.split(delimiter)
    assert len(items) == 2
    http_response = json.loads(items[0])
    eventlog = json.loads(items[1])
    return (http_response, eventlog)


def parse_eventlog_from_raw_response(raw_response: str) -> ApphostEventlogWrapper:
    eventlog = split_raw_response_to_http_response_and_eventlog(raw_response)[1]
    assert isinstance(eventlog, list)
    return parse_eventlog(eventlog)


def parse_eventlog_from_string(eventlog_str: str) -> ApphostEventlogWrapper:
    eventlog = json.loads(eventlog_str)
    assert isinstance(eventlog, list)
    return parse_eventlog(eventlog)


def parse_eventlog(eventlog: list) -> ApphostEventlogWrapper:
    records: List[LogDumpRecord] = []
    for event in eventlog:
        patched_event = event
        if isinstance(event, dict) and EVENT_KEY in event.keys():
            patched_event = event[EVENT_KEY]
        if isinstance(event, dict):
            records.append(LogDumpRecord(patched_event))
        else:
            logger.error(f'Patched event is invalid. Full event: {event}')
    records = [record for record in records if record.is_supported_record()]
    return ApphostEventlogWrapper(records)
