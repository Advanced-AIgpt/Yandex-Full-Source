# coding: utf-8

import enum

import attr


@enum.unique
class Type(enum.Enum):
    NEXT_REQUEST = 'next_request'
    END = 'end'
    EVO_TESTS_STARTED = 'evo_tests_started'
    REQUEST_EXCEPTION = 'request_exception'
    UNIPROXY_SETTINGS = 'uniproxy_settings'
    TEST_RESULT = 'test_result'


@enum.unique
class Status(enum.Enum):
    OK = 'ok'
    FAIL = 'fail'


@attr.s
class Response:
    type = attr.ib()
    status = attr.ib()
    message = attr.ib(default='')
    payload = attr.ib(default=attr.Factory(dict))

    @type.validator
    def type_validate(self, attribute, value: Type):
        pass

    @status.validator
    def status_validate(self, attribute, value: Status):
        pass

    def to_dict(self):
        return {
            'type': self.type.value,
            'status': self.status.value,
            'message': self.message,
            'payload': self.payload,
        }


class AliceNextRequest(Response):
    def __init__(self, request):
        super(AliceNextRequest, self).__init__(
            type=Type.NEXT_REQUEST,
            status=Status.OK,
            payload=request,
        )


class AliceEndSession(Response):
    pass


class AliceEndSessionOk(AliceEndSession):
    def __init__(self):
        super(AliceEndSessionOk, self).__init__(
            type=Type.END,
            status=Status.OK,
        )


class AliceEndSessionFailed(AliceEndSession):
    def __init__(self, message=''):
        super(AliceEndSessionFailed, self).__init__(
            type=Type.END,
            status=Status.FAIL,
            message=message,
        )


class AliceNextRequestException(Response):
    def __init__(self, message):
        super(AliceNextRequestException, self).__init__(
            type=Type.REQUEST_EXCEPTION,
            status=Status.FAIL,
            message=message,
        )


class AliceUniproxySettings(Response):
    def __init__(self, uniproxy_settings):
        super(AliceUniproxySettings, self).__init__(
            type=Type.UNIPROXY_SETTINGS,
            status=Status.OK,
            payload=uniproxy_settings,
        )


class AliceEvoTestsStarted(Response):
    def __init__(self):
        super(AliceEvoTestsStarted, self).__init__(
            type=Type.EVO_TESTS_STARTED,
            status=Status.OK,
        )


class AliceEvoTestResult(Response):
    def __init__(self, payload):
        super(AliceEvoTestResult, self).__init__(
            type=Type.TEST_RESULT,
            status=Status.OK,
            payload=payload,
        )
