from aiohttp import web

from abc import ABC, abstractmethod
import logging
import json
from jsonschema import Draft7Validator, ValidationError
from typing import Dict, Tuple

logger = logging.getLogger(__name__)


class ValidatingHandler(ABC):
    @abstractmethod
    async def _handle(self, payload: Dict, app: web.Application) -> web.Response:
        pass

    @abstractmethod
    def _validate_request(self, payload: Dict) -> Tuple[bool, str]:
        pass

    def _validate_response(self, response_body: str) -> Tuple[bool, str]:
        return True

    async def handle(self, request: web.BaseRequest) -> web.Response:
        payload = await request.json()

        request_valid, reason = self._validate_request(payload)
        if not request_valid:
            logger.error('Payload %s is invalid. Reason: %s', payload, reason)
            raise web.HTTPBadRequest(reason=reason)

        response = await self._handle(payload=payload, app=request.app)

        response_valid, reason = self._validate_response(response.text)
        if not response_valid:
            logger.error('Response %s is invalid. Reason: %s', response.text, reason)
            raise web.HTTPInternalServerError(reason=reason)

        return response


class JsonSchemaValidateHandler(ValidatingHandler):
    def __init__(self) -> None:
        super().__init__()

        if self.request_schema is not None:
            self.request_validator = Draft7Validator(self.request_schema)

        if self.response_schema is not None:
            self.response_validator = Draft7Validator(self.response_schema)

    @property
    @abstractmethod
    def request_schema(self) -> str:
        pass

    def _validate_request(self, payload: Dict) -> Tuple[bool, str]:
        if self.request_schema is None:
            return True, None

        try:
            self.request_validator.validate(payload)
        except ValidationError as ve:
            return False, ve.message

        return True, None

    @property
    def response_schema(self) -> str:
        return None

    def _validate_response(self, response_body: str) -> Tuple[bool, str]:
        if self.response_schema is None:
            return True, None

        response_decoded = json.loads(response_body)
        try:
            self.response_validator.validate(response_decoded)
        except ValidationError as ve:
            return False, ve.message

        return True, None
