import base64
import logging

from enum import Enum
from typing import Optional, List, Union

from apphost.lib.proto_answers.http_pb2 import THttpResponse


Json = Union[dict, list, str, int, float, bool, None]
logger = logging.getLogger(__name__)


EVENT_BODY_KEY = 'EventBody'
EVENT_TYPE_KEY = 'Type'
EVENT_FIELDS_KEY = 'Fields'
EVENT_SOURCE_KEY = 'Source'
JSON_KEY = 'Json'
ANSWERS_KEY = 'answers'
ANSWER_TYPE_KEY = 'type'
ANSWER_CONTENT_KEY = 'binary'
ANSWER_CONTENT_TYPE_KEY = '__content_type'


class ApphostEventlogDumpType(Enum):
    source_response = 'TSourceResponse'
    source_request = 'TSourceRequest'
    http_source_response = 'THttpSourceResponse'

    def __str__(self) -> str:
        return self.value


SUPPORTED_DUMP_TYPES = set(
    supported_type.value for supported_type in ApphostEventlogDumpType)


class ApphostAnswerContentType(Enum):
    protobuf = 'protobuf'
    json = 'json'

    def __str__(self) -> str:
        return self.value


SUPPORTED_CONTENT_TYPES = set(
    supported_type.value for supported_type in ApphostAnswerContentType
)


class ApphostAnswerStub:

    def __init__(self, raw_answer: Json):
        self._raw_answer: dict = raw_answer if isinstance(
            raw_answer, dict) else dict()

    def get_type(self) -> Optional[str]:
        return self._raw_answer.get(ANSWER_TYPE_KEY)

    def get_content_raw(self) -> Json:
        return self._raw_answer.get(ANSWER_CONTENT_KEY)

    def get_content_type(self) -> Optional[ApphostAnswerContentType]:
        type = self._raw_answer.get(ANSWER_CONTENT_TYPE_KEY)
        return ApphostAnswerContentType(type) if type and type in SUPPORTED_CONTENT_TYPES else None


class LogDumpRecord:

    def __init__(self, raw_event: dict):
        self._raw_event: dict = raw_event
        event_body: Optional[dict] = LogDumpRecord._get_event_body(raw_event)
        self._fields: dict = LogDumpRecord._get_fields(
            event_body) if event_body else dict()

        if not event_body or not self._fields:
            logger.error(f"Can't parse event: {raw_event}")

        self._event_type: Optional[ApphostEventlogDumpType] = LogDumpRecord._get_type(
            event_body) if event_body else None
        self._answers: List[ApphostAnswerStub] = LogDumpRecord._get_source_answers(
            self._fields) if self.is_source_request() or self.is_source_response() else []
        self._http_response: Optional[THttpResponse] = LogDumpRecord._get_http_response(  # type: ignore
            self._fields) if self.is_http_source_response() else None

    @staticmethod
    def _get_event_body(raw_event: dict) -> Optional[dict]:
        if not isinstance(raw_event, dict):
            logger.error('Raw event is not dict')
            return None
        event_body = raw_event.get(EVENT_BODY_KEY)
        if not event_body or not isinstance(event_body, dict):
            logger.error('Event body is not presented or invalid')
            return None
        return event_body

    @staticmethod
    def _get_type(event_body: dict) -> Optional[ApphostEventlogDumpType]:
        event_type = event_body.get(EVENT_TYPE_KEY)
        is_supported = event_type in SUPPORTED_DUMP_TYPES
        return ApphostEventlogDumpType(event_type) if is_supported else None

    @staticmethod
    def _get_fields(event_body: dict) -> dict:
        fields = event_body.get(EVENT_FIELDS_KEY)
        if not fields or not isinstance(fields, dict):
            logger.error('Fields field is not presented invalid')
            return dict()
        return fields

    @staticmethod
    def _get_source_answers(fields: dict) -> List[ApphostAnswerStub]:
        answers: List[ApphostAnswerStub] = []
        json_field = fields.get(JSON_KEY)
        if not json_field or not isinstance(json_field, dict):
            logger.error(
                f'Json field is not presented or invalid, fields: {fields}')
            return answers
        answers_field = json_field.get(ANSWERS_KEY)
        if not answers_field or not isinstance(answers_field, list):
            logger.error(
                f'Answers field is not presented or invalid, fields: {fields}')
            return answers

        for answer in answers_field:
            answers.append(ApphostAnswerStub(answer))
        return answers

    @staticmethod
    def _get_http_response(fields: dict) -> Optional[THttpResponse]:  # type: ignore
        json_field = fields.get(JSON_KEY)
        if not json_field or not isinstance(json_field, dict):
            logger.error(
                f'Json field is not presented or invalid, fields: {fields}')
            return None

        http_response = THttpResponse()
        http_response.StatusCode = json_field['status_code']
        http_response.Content = base64.b64decode(json_field.get('content', ''))
        return http_response

    def get_event_type(self) -> Optional[ApphostEventlogDumpType]:
        return self._event_type

    def is_supported_record(self) -> bool:
        return self.get_event_type() is not None

    def is_source_request(self) -> bool:
        return self.get_event_type() == ApphostEventlogDumpType.source_request

    def is_source_response(self) -> bool:
        return self.get_event_type() == ApphostEventlogDumpType.source_response

    def is_http_source_response(self) -> bool:
        return self.get_event_type() == ApphostEventlogDumpType.http_source_response

    def get_source_name(self) -> Optional[str]:
        return self._fields.get(EVENT_SOURCE_KEY)

    def get_answers(self) -> List[ApphostAnswerStub]:
        return self._answers

    def get_answer(self, answer_type: str) -> Optional[ApphostAnswerStub]:
        for answer in self._answers:
            if answer.get_type() == answer_type:
                return answer
        logger.info(f'No answer of type {answer_type}')
        return None

    def get_http_source_response(self) -> Optional[THttpResponse]:  # type: ignore
        assert self.is_http_source_response()
        return self._http_response

    def get_answer_content_raw(self, answer_type: str) -> Json:
        answer = self.get_answer(answer_type)
        return answer.get_content_raw() if answer else None

    def get_source_response_raw(self, response_type: str) -> Json:
        assert self.is_source_response()
        return self.get_answer_content_raw(response_type)

    def get_source_request_raw(self, request_type: str) -> Json:
        assert self.is_source_request()
        return self.get_answer_content_raw(request_type)

    @property
    def raw_event(self) -> dict:
        return self._raw_event
