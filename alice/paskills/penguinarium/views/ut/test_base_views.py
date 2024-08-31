import pytest
from aiohttp import web
from alice.paskills.penguinarium.views.base import ValidatingHandler, JsonSchemaValidateHandler


@pytest.mark.asyncio
async def test_base_abc_methods():
    assert await ValidatingHandler._handle(None, None, None) is None
    assert ValidatingHandler._validate_request(None, None) is None
    assert ValidatingHandler._validate_response(None, None) is True
    assert JsonSchemaValidateHandler.request_schema.fget(None) is None


class FakeReq:
    app = None

    async def json(self):
        return {}


class FakeResp:
    text = ''


class InvalidReqHandler(ValidatingHandler):
    async def _handle(self, payload, app):
        return FakeResp()

    def _validate_request(self, payload):
        return False, ''


class InvalidRespHandler(ValidatingHandler):
    async def _handle(self, payload, app):
        return FakeResp()

    def _validate_request(self, payload):
        return True, None

    def _validate_response(self, response_body):
        return False, ''


@pytest.mark.asyncio
async def test_invalid_req():
    with pytest.raises(web.HTTPBadRequest):
        await InvalidReqHandler().handle(FakeReq())


@pytest.mark.asyncio
async def test_invalid_resp():
    with pytest.raises(web.HTTPInternalServerError):
        await InvalidRespHandler().handle(FakeReq())


class ValidJsonHandler(JsonSchemaValidateHandler):
    async def _handle(self, payload, app):
        return FakeResp()

    @property
    def request_schema(self):
        return None


@pytest.mark.asyncio
async def test_valid_json_handler():
    vjh = ValidJsonHandler()
    await vjh.handle(FakeReq())
    assert vjh._validate_request(None) == (True, None)
