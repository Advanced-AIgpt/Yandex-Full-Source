import logging
import pytest

import tornado
import tornado.ioloop
import tornado.concurrent
import tornado.httpserver
import tornado.gen
import struct

from rtlog import null_logger

import alice.uniproxy.library.testing
from alice.uniproxy.library.events import Event
from cityhash import hash64 as CityHash64

from alice.uniproxy.library.processors import create_event_processor
from alice.uniproxy.library.testing.checks import match

import alice.uniproxy.library.messenger as messenger
from alice.uniproxy.library.messenger.msgsettings import UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS
from alice.uniproxy.library.processors.messenger import TInMessage, TResponse
from alice.uniproxy.library.processors.messenger import THistoryRequest, THistoryResponse
from alice.uniproxy.library.processors.messenger import TEditHistoryRequest, TEditHistoryResponse
from alice.uniproxy.library.processors.messenger import TSubscriptionRequest, TSubscriptionResponse
from alice.uniproxy.library.processors.messenger import TWhoamiRequest, TWhoamiResponse
from alice.uniproxy.library.processors.messenger import TMessageInfoRequest, TMessageInfoResponse


from alice.uniproxy.library.utils.proto_to_json import MessageToDict2
from alice.uniproxy.library.utils.json_to_proto import DictToMessage2

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.async_http_client.http_request import HTTPError

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')
GlobalCounter.init()


_g_fanout = None


# ====================================================================================================================
class FanoutError(Exception):
    def __init__(self, code: int, body: bytes):
        super().__init__()
        self.code = code
        self.body = body


# ====================================================================================================================
class FanoutResponseError(FanoutError):
    def __init__(self, code: int, response: dict, message_type):
        data = DictToMessage2(
            message_type,
            response,
            json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS
        ).SerializeToString()
        header = struct.pack('<IQ', 2, CityHash64(data))
        super().__init__(
            code,
            header + data
        )


# ====================================================================================================================
class FanoutTextResponseError(FanoutError):
    def __init__(self, code: int, response: str):
        super().__init__(code, response.encode('utf-8'))


# ====================================================================================================================
class FanoutBaseHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        self._request_type = kwargs.pop('request_type')
        self._response_type = kwargs.pop('response_type')
        self._logger = logging.getLogger(kwargs.pop('logger_name', 'fanout.base'))
        super().__init__(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    def parse_request(self, body):
        self._logger.debug('parse_request')
        header, data = body[:12], body[12:]
        if len(header) < 12:
            self._logger.error('parse_request header < 12')
            raise FanoutError(400, b'400 Bad Request')

        version, checksum = struct.unpack('<IQ', header)
        if version != 2:
            self._logger.error('parse_request version != 2')
            raise FanoutError(400, b'400 Bad Request')

        if checksum != CityHash64(data):
            self._logger.error('parse_request invalid checksum')
            raise FanoutError(400, b'400 Bad Request')

        self._logger.debug('parse_request parsing proto message')
        message = self._request_type()
        message.ParseFromString(data)
        self._logger.debug('parse_request message: %s', message)

        m = MessageToDict2(message, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)
        self._logger.debug('parse_request dict: %s', m)
        return version, checksum, m

    # ----------------------------------------------------------------------------------------------------------------
    def prepare_response(self, response: dict) -> bytes:
        message = DictToMessage2(self._response_type, response, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)
        data = message.SerializeToString()
        header = struct.pack('<IQ', 2, CityHash64(data))
        return header + data

    # ----------------------------------------------------------------------------------------------------------------
    def _check_headers(self):
        if 'X-Ya-Service-Ticket' not in self.request.headers:
            raise FanoutTextResponseError(403, 'Access rules violated')

    # ----------------------------------------------------------------------------------------------------------------
    def post(self):
        self._logger.info('post')
        response_code = 200
        response_body = b''

        try:
            self._check_headers()
            self._logger.info('post parse_request')
            version, checksum, request = self.parse_request(self.request.body)
            self._logger.info('post handle, request=%s', request)
            response_code, response_dict = self.handle(request)
            self._logger.info('post prepare_response')
            response_body = self.prepare_response(response_dict)
        except FanoutError as ex:
            self._logger.exception(ex)
            response_code = ex.code
            response_body = ex.body
        except Exception as ex:
            self._logger.exception(ex)
            response_code = 500
            response_body = str(ex).encode('utf-8')
        finally:
            self.set_header('Content-Type', 'application/octet-stream')
            self.set_status(response_code)
            self.finish(response_body)

    # ----------------------------------------------------------------------------------------------------------------
    def throw(self, code: int, response: dict):
        raise FanoutResponseError(code, response, self._response_type)

    # ----------------------------------------------------------------------------------------------------------------
    def throw_text(self, code: int, response: str):
        raise FanoutTextResponseError(code, response)

    # ----------------------------------------------------------------------------------------------------------------
    def handle(self, request: dict) -> (int, dict):
        pass


# ====================================================================================================================
class FanoutPushHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TInMessage,
            response_type=TResponse,
            logger_name='fanout.push',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 13})

        client_message = request.get('ClientMessage', {})
        plain_message = client_message.get('Plain', {})
        chat_id = plain_message.get('ChatId', '')
        self._logger.debug('chat_id: %s', chat_id)
        if 'chat-not-found' in chat_id:
            self.throw(404, {'Status': 4})
        elif 'internal-server-error-12' in chat_id:
            self.throw_text(500, 'Internal ser')
        elif 'internal-server-error' in chat_id:
            self.throw_text(500, 'Internal server error')
        elif 'empty-response' in chat_id:
            self.throw_text(200, '')
        return (200, {'Status': 1})


# ====================================================================================================================
class FanoutHistoryHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=THistoryRequest,
            response_type=THistoryResponse,
            logger_name='fanout.history',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 4})

        chat_id = request.get('ChatId', '')
        limit = request.get('Limit', 100)
        request_id = request.get('RequestId')

        if request_id is None:
            self.throw_text(400, {'Status': 1})

        self._logger.debug('chat_id: %s', chat_id)
        if 'chat-not-found' in chat_id:
            self.throw(404, {'Status': 4, 'RequestId': request_id})
        elif 'internal-server-error' in chat_id:
            self.throw_text(500, 'Internal server error')
        elif 'empty-response' in chat_id:
            return (200, {'Status': 0})
        elif limit > 100:
            self.throw(500, {'Status': 1, 'RequestId': 'foo'})

        return (200, {'Status': 0, 'RequestId': 'foo'})


# ====================================================================================================================
class FanoutEditHistoryHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TEditHistoryRequest,
            response_type=TEditHistoryResponse,
            logger_name='fanout.history',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 4})

        chat_id = request.get('ChatId', '')
        limit = request.get('Limit', 100)
        request_id = request.get('RequestId')

        if request_id is None:
            self.throw_text(400, {'Status': 1})

        self._logger.debug('chat_id: %s', chat_id)
        if 'chat-not-found' in chat_id:
            self.throw(404, {'Status': 4, 'RequestId': request_id})
        elif 'internal-server-error' in chat_id:
            self.throw_text(500, 'Internal server error')
        elif 'empty-response' in chat_id:
            return (200, {'Status': 0})
        elif limit > 100:
            self.throw(500, {'Status': 1, 'RequestId': 'foo'})

        return (200, {'Status': 0, 'RequestId': 'foo'})


# ====================================================================================================================
class FanoutSubscribeHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TSubscriptionRequest,
            response_type=TSubscriptionResponse,
            logger_name='fanout.subscribe',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 4})

        chat_id = request.get('ChatId', '')
        request_id = request.get('RequestId')

        if request_id is None:
            self.throw_text(400, {'Status': 1})

        self._logger.debug('chat_id: %s', chat_id)
        if 'chat-not-found' in chat_id:
            self.throw(404, {'Status': 4, 'RequestId': request_id})
        elif 'internal-server-error' in chat_id:
            self.throw_text(500, 'Internal server error')
        elif 'empty-response' in chat_id:
            return (200, {'Status': 0})

        return (200, {'Status': 0, 'RequestId': 'foo'})


# ====================================================================================================================
class FanoutWhoamiHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TWhoamiRequest,
            response_type=TWhoamiResponse,
            logger_name='fanout.whoami',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 13})

        return (200, {
            'UserInfo': {
                'AvatarId': 'avatar',
                'Guid': Guid,
                'Version': 42,
            }
        })


# ====================================================================================================================
class FanoutMessageInfoHandler(FanoutBaseHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TMessageInfoRequest,
            response_type=TMessageInfoResponse,
            logger_name='fanout.message_info',
            **kwargs
        )

    def handle(self, request: dict) -> (int, dict):
        Guid = request.get('Guid')
        if not Guid:
            self.throw(400, {'Status': 3})

        return (200, {
            'Status': 1,
        })


# ====================================================================================================================
class FanoutServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/push', FanoutPushHandler),
            (r'/history', FanoutHistoryHandler),
            (r'/edit_history', FanoutEditHistoryHandler),
            (r'/subscribe', FanoutSubscribeHandler),
            (r'/whoami', FanoutWhoamiHandler),
            (r'/message_info', FanoutMessageInfoHandler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


_g_fanout_mock = None
_g_fanout_initialized = False


@tornado.gen.coroutine
def wait_for_fanout_mock():
    global _g_fanout_mock, _g_fanout_initialized

    if _g_fanout_initialized:
        return True

    config.set_by_path('messenger.host', 'localhost')
    config.set_by_path('messenger.port', 9823)
    config.set_by_path('messenger.secure', False)
    config.set_by_path('messenger.pool.size', 1)
    config.set_by_path('messenger.disable_tvm', True)

    _g_fanout_mock = FanoutServerMock(config['messenger']['host'], config['messenger']['port'])
    _g_fanout_mock.start()

    yield tornado.gen.sleep(0.2)

    messenger.init_mssngr()

    yield tornado.gen.sleep(0.2)

    _g_fanout_initialized = True

    return True


# ====================================================================================================================
class MessengerClientMock(object):
    def __init__(self):
        super().__init__()
        self.push_calls = 0
        self.last_request_type = None
        self.last_response_type = None
        self.last_payload = {}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def fetch(self, url, reqtype, rsptype, payload, accept_gzip=False):
        if 'push' in url:
            return (yield self.push(reqtype, rsptype, payload))
        elif 'history' in url:
            return self.history(reqtype, rsptype, payload)
        elif 'edit_history' in url:
            return self.edit_history(reqtype, rsptype, payload)
        elif 'subscribe' in url:
            return self.subscribe(reqtype, rsptype, payload)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def push(self, reqtype, rsptype, payload):
        self.push_calls += 1
        self.last_request_type = reqtype
        self.last_response_type = rsptype
        self.last_payload = payload
        return True, {'Status': 1}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def history(self, reqtype, rsptype, payload):
        self.push_calls += 1
        self.last_request_type = reqtype
        self.last_response_type = rsptype
        self.last_payload = payload
        return True, {}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def edit_history(self, reqtype, rsptype, payload):
        self.push_calls += 1
        self.last_request_type = reqtype
        self.last_response_type = rsptype
        self.last_payload = payload
        return True, {}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def subscribe(self, reqtype, rsptype, payload):
        self.push_calls += 1
        self.last_request_type = reqtype
        self.last_response_type = rsptype
        self.last_payload = payload
        return True, {}


# ====================================================================================================================
class UnisystemMock(object):
    def __init__(self):
        super().__init__()
        self.session_id = '1d2c0237-8c2c-4d6e-9d27-e82558da2bab'
        self.client_ip = '127.0.0.1'
        self.guid = '6f164a1b-1f6a-4d5e-9a1e-c59f62d86e51'
        self._last_directive = None
        self._last_message = None
        self.user_agent = None
        self.icookie = 'icookie-value'
        self.mssngr_version = 0

    @tornado.gen.coroutine
    def get_bb_user_ticket(self):
        return None

    def uuid(self):
        return '326fa655-9190-4206-9d86-ccd0ea438a12'

    def session_data(self):
        return {
            'uuid': self.uuid(),
        }

    def write_directive(self, directive):
        self._last_directive = directive.create_message(self)

    def write_message(self, message):
        self._last_message = message

    def on_close_event_processor(self, *args, **kwargs):
        pass

    def next_message_id(self):
        return 42


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_heartbeat_version_eq_4():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    unisystem.mssngr_version = 4
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Heartbeat': {
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert 'Heartbeat' in client.last_payload['ClientMessage']
    assert unisystem._last_directive is not None


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_no_ack_for_heartbeat_version_lt_4():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    unisystem.mssngr_version = 3
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Heartbeat': {
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert 'Heartbeat' in client.last_payload['ClientMessage']
    assert unisystem._last_directive is None


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_with_payload():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert client.last_payload['ClientMessage']['Plain']['ChatId'] == event.payload['ClientMessage']['Plain']['ChatId']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'Ack' in unisystem._last_directive['directive']['payload']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_with_obsolete_payload():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'PayloadId': 'payload-id'
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert 'Ack' in unisystem._last_directive['directive']['payload']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_without_payload():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert client.last_payload['ClientMessage']['Plain']['ChatId'] == event.payload['ClientMessage']['Plain']['ChatId']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Ack' not in unisystem._last_directive['directive']['payload']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_bot_request():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'BotRequest': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert client.last_payload['ClientMessage']['BotRequest']['ChatId'] == event.payload['ClientMessage']['BotRequest']['ChatId']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_calling_message():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'CallingMessage': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                },
            },
            'PayloadId': 'payload-id',
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'Ack' in unisystem._last_directive['directive']['payload']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']
    assert unisystem._last_directive['directive']['payload']['Ack'] == event.payload['PayloadId']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_report():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Report': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_seen_marker():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'SeenMarker': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_seen_marker_with_payload_id():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'SeenMarker': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                },
            },
            'PayloadId': 'payload-id',
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'Ack' in unisystem._last_directive['directive']['payload']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']
    assert unisystem._last_directive['directive']['payload']['Ack'] == event.payload['PayloadId']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_authentication_error():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = {
        'scope': 'yamb',
        'code': '404',
        'text': 'user_not_found',
    }
    unisystem.guid = None

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.5)

    print(str(unisystem._last_directive))
    a = unisystem._last_directive
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert a['directive']['payload']['details']['scope'] == 'yamb'
    assert a['directive']['payload']['details']['code'] == '404'


@alice.uniproxy.library.testing.ioloop_run
def test_post_authentication_generic_error():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = None

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.5)

    print(str(unisystem._last_directive))
    a = unisystem._last_directive
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'details' not in a['directive']['payload']


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock():
    yield wait_for_fanout_mock()


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_2():
    yield wait_for_fanout_mock()


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_push_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'Ack'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in a['directive']['payload']
    assert 'Status' in a['directive']['payload']['Response']
    assert a['directive']['payload']['Response']['Status'] == 1


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_push_200_empty():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/empty-response',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())

    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)

    assert a['directive']['header']['name'] == 'EventException'
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['error']['message'] == 'fanout error'
    assert a['directive']['payload']['details']['text'] == 'empty response'
    assert a['directive']['payload']['details']['scope'] == 'fanout'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_push_404():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/chat-not-found',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })

    x = GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_4XX_SUMM.value()
    y = GlobalCounter.MSSNGR_MESSAGES_IN_FAIL_SUMM.value()

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert 'Response' in a['directive']['payload']
    assert 'Status' in a['directive']['payload']['Response']
    assert a['directive']['payload']['Response']['Status'] == 4
    assert 'code' in a['directive']['payload']['details']
    assert type(a['directive']['payload']['details']['code']) == str
    assert a['directive']['payload']['error']['message'] == '404'
    assert GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_4XX_SUMM.value() - x == 1
    assert GlobalCounter.MSSNGR_MESSAGES_IN_FAIL_SUMM.value() - y == 1


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_push_500():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/internal-server-error',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })

    x = GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_5XX_SUMM.value()

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['error']['message'] == 'Internal server error'
    assert GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_5XX_SUMM.value() - x == 1


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_push_500_12():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Plain': {
                    'ChatId': '0/0/internal-server-error-12',
                    'Text': {
                        'MessageText': 'foo bar baz',
                    },
                    'PayloadId': 'payload-id',
                },
            },
        },
    })

    x = GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_5XX_SUMM.value()

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['error']['message'].startswith('Internal')
    assert GlobalCounter.MSSNGR_MESSAGES_IN_FANOUT_5XX_SUMM.value() - x == 1


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_history_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'HistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/f1f375a9-4493-4657-930a-5bf983b77577',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'History'
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['RequestId'] == 'foo'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_history_200_empty():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'HistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/empty-response',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'History'
    assert 'Response' not in a['directive']['payload']
    assert 'RequestId' not in a['directive']['payload']


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_history_404():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'HistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/chat-not-found',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in a['directive']['payload']
    assert 'Status' in a['directive']['payload']['Response']
    assert a['directive']['payload']['Response']['RequestId'] == 'foo'
    assert a['directive']['payload']['details']['scope'] == 'fanout'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_history_500():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'HistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/internal-server-error',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert a['directive']['header']['refMessageId'] == event.message_id
    assert 'error' in a['directive']['payload']
    assert 'details' in a['directive']['payload']
    assert a['directive']['payload']['details']['scope'] == 'fanout'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_edit_history_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'EditHistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/f1f375a9-4493-4657-930a-5bf983b77577',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EditHistoryResponse'
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['RequestId'] == 'foo'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_edit_history_404():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'EditHistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/7/chat-not-found',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert 'Response' in a['directive']['payload']
    assert a['directive']['payload']['details']['scope'] == 'fanout'
    assert a['directive']['payload']['details']['code'] == '404'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_edit_history_500():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'EditHistoryRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/7/internal-server-error',
            'Limit': 50,
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'EventException'
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['details']['scope'] == 'fanout'
    assert a['directive']['payload']['details']['code'] == '500'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_subscribe_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'SubscriptionRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ChatId': '0/0/f1f375a9-4493-4657-930a-5bf983b77577',
            'RequestId': 'foo',
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'SubscriptionResponse'
    assert 'Response' not in a['directive']['payload']
    assert a['directive']['payload']['RequestId'] == 'foo'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_whoami_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'Whoami',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'WhoamiResponse'
    assert 'UserInfo' in a['directive']['payload']
    assert a['directive']['payload']['UserInfo']['Guid'] == unisystem.guid
    assert a['directive']['payload']['UserInfo']['AvatarId'] == 'avatar'
    assert a['directive']['payload']['UserInfo']['Version'] == 42


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_message_info_200():
    yield wait_for_fanout_mock()

    unisystem = UnisystemMock()
    unisystem.mssngr_auth_error = None
    unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'MessageInfoRequest',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
        },
    })

    print(event.payload)
    processor = create_event_processor(unisystem, event, rt_log=null_logger())
    processor.process_event(event)

    yield tornado.gen.sleep(0.4)

    a = unisystem._last_directive
    print(a)
    assert a['directive']['header']['name'] == 'MessageInfo'
    assert a['directive']['payload']['Status'] == 1


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_delivery_event_standard_message():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'Fanout',
            'namespace': 'Messenger',
        },
        'payload': {
            'data': {
                'ServerMessage': {
                    'ClientMessage': {
                        'Plain': {
                            'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                            'Text': {
                                'MessageText': 'foo bar baz',
                            },
                            'PayloadId': 'payload-id',
                        },
                    },
                },
            }
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger())

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_message))
    assert client.push_calls == 0
    assert unisystem._last_message is not None

    header = unisystem._last_message['directive']['header']
    payload = unisystem._last_message['directive']['payload']
    assert header['name'] == 'Message'
    assert 'ServerMessage' in payload
    assert 'ClientMessage' in payload['ServerMessage']
    assert 'Plain' in payload['ServerMessage']['ClientMessage']
    assert payload['ServerMessage']['ClientMessage']['Plain']['PayloadId'] == 'payload-id'
    assert payload['ServerMessage']['ClientMessage']['Plain']['ChatId'] == '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_delivery_event_event_exception():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'Fanout',
            'namespace': 'Messenger',
        },
        'payload': {
            'data': {
                'ServerMessage': {
                    'ClientMessage': {
                        'EventException': {
                            'Type': 'type',
                            'Message': 'message',
                            'Details': {
                                'Code': 'code',
                                'Scope': 'scope',
                                'Text': 'text',
                            }
                        },
                    },
                },
            }
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger())

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 0
    assert unisystem._last_directive is not None

    header = unisystem._last_directive['directive']['header']
    payload = unisystem._last_directive['directive']['payload']
    assert header['namespace'] == 'System'
    assert header['name'] == 'EventException'
    assert 'error' in payload
    assert 'type' in payload['error']
    assert 'message' in payload['error']
    assert payload['error']['type'] == 'Error'
    assert payload['error']['message'] == 'message'
    assert 'details' in payload
    assert payload['details']['code'] == 'code'
    assert payload['details']['scope'] == 'scope'
    assert payload['details']['text'] == 'text'


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_pin():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Pin': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Timestamp': 42,
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert client.last_payload['ClientMessage']['Pin']['ChatId'] == event.payload['ClientMessage']['Pin']['ChatId']
    assert client.last_payload['ClientMessage']['Pin']['Timestamp'] == event.payload['ClientMessage']['Pin']['Timestamp']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_unpin():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'Unpin': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Timestamp': 42,
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['Guid'] == event.payload['Guid']
    assert client.last_payload['ClientMessage']['Unpin']['ChatId'] == event.payload['ClientMessage']['Unpin']['ChatId']
    assert client.last_payload['ClientMessage']['Unpin']['Timestamp'] == event.payload['ClientMessage']['Unpin']['Timestamp']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_ack_for_read_marker():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid,
            'ClientMessage': {
                'ReadMarker': {
                    'ChatId': '0/0/79a59633-e08e-47d7-977e-4a3c145dadc3',
                    'Timestamps': [1, 2, 3],
                },
            },
        },
    })
    print(event.payload)

    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)

    processor.process_event(event)

    yield tornado.gen.sleep(0.025)

    print(str(unisystem._last_directive))
    assert client.push_calls == 1
    assert client.last_request_type == TInMessage
    assert client.last_response_type == TResponse
    assert client.last_payload['ClientMessage']['ReadMarker']['ChatId'] == event.payload['ClientMessage']['ReadMarker']['ChatId']
    assert unisystem._last_directive['directive']['header']['name'] == 'Ack'
    assert unisystem._last_directive['directive']['header']['refMessageId'] == event.message_id
    assert 'Response' in unisystem._last_directive['directive']['payload']
    assert 'Status' in unisystem._last_directive['directive']['payload']['Response']
    assert 'TsVal' in unisystem._last_directive['directive']['payload']
    assert 'TsEcR' in unisystem._last_directive['directive']['payload']


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_message_with_icookie():
    client = MessengerClientMock()
    unisystem = UnisystemMock()
    event = Event({
        'header': {
            'messageId': 'fffd06fe-0bdd-4f9b-952d-b4c423460d4b',
            'name': 'PostMessage',
            'namespace': 'Messenger',
        },
        'payload': {
            'Guid': unisystem.guid
        },
    })
    processor = create_event_processor(unisystem, event, rt_log=null_logger(), mssngr_client=client)
    processor.process_event(event)

    yield tornado.gen.sleep(0.025)
    assert client.last_request_type == TInMessage
    assert client.last_payload['ClientMessage']['LogData']['ICookie'] == 'icookie-value'


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def _test_messenger_59x_error(processor_name, counter_name, code):
    class MessengerClientMock:
        def __enter__(self, *args, **kwargs):
            self._orig_client = messenger._g_client
            messenger._g_client = self
            return self

        def __exit__(self, *args, **kwargs):
            messenger._g_client = self._orig_client

        def __init__(self, err_code):
            self._orig_client = None
            self._err_code = err_code

        async def fetch(self, *args, **kw):
            raise HTTPError(code=self._err_code, body=b"I am naughty")

    with MessengerClientMock(code):
        unisystem = UnisystemMock()
        unisystem.mssngr_auth_error = None
        unisystem.guid = '0a14c4ef-9992-4daa-b8a6-ce183afff76d'

        counter = GlobalCounter.g_counters[counter_name]
        event = Event({
            "header": {
                "messageId": "fffd06fe-0bdd-4f9b-952d-b4c423460d4b",
                "name": processor_name,
                "namespace": "Messenger",
            },
            "payload": {
                "Guid": unisystem.guid,
            },
        })

        counter_val = counter.value()
        processor = create_event_processor(system=unisystem, event=event, rt_log=null_logger())
        processor.process_event(event)

        yield tornado.gen.sleep(0.3)

        res = unisystem._last_directive
        print(res)
        assert match(res, {"directive": {"payload": {
            "error": {"type": "Error"},
            "details": {
                "text": "I am naughty",
                "code": str(code)
            }
        }}})
        assert counter.value() == counter_val + 1


# ====================================================================================================================
@pytest.mark.parametrize("processor_name,counter_name", [
    ("MessageInfoRequest", "mssngr_minfo_in_fanout_599_summ"),
    ("Whoami", "mssngr_whoami_in_fanout_599_summ"),
    ("PostMessage", "mssngr_messages_in_fanout_599_summ"),
    ("SubscriptionRequest", "mssngr_subscription_in_fanout_599_summ"),
    ("EditHistoryRequest", "mssngr_edit_history_fanout_599_summ"),
    ("HistoryRequest", "mssngr_history_in_fanout_599_summ")
])
def test_messenger_metrics_for_599(processor_name, counter_name):
    _test_messenger_59x_error(processor_name, counter_name, HTTPError.CODE_REQUEST_TIMEOUT)


# ====================================================================================================================
@pytest.mark.parametrize("processor_name,counter_name", [
    ("MessageInfoRequest", "mssngr_minfo_in_fanout_598_summ"),
    ("Whoami", "mssngr_whoami_in_fanout_598_summ"),
    ("PostMessage", "mssngr_messages_in_fanout_598_summ"),
    ("SubscriptionRequest", "mssngr_subscription_in_fanout_598_summ"),
    ("EditHistoryRequest", "mssngr_edit_history_fanout_598_summ"),
    ("HistoryRequest", "mssngr_history_in_fanout_598_summ")
])
def test_messenger_metrics_for_598(processor_name, counter_name):
    _test_messenger_59x_error(processor_name, counter_name, HTTPError.CODE_CONNECT_TIMEOUT)


# ====================================================================================================================
@pytest.mark.parametrize("processor_name,counter_name", [
    ("MessageInfoRequest", "mssngr_minfo_in_fanout_597_summ"),
    ("Whoami", "mssngr_whoami_in_fanout_597_summ"),
    ("PostMessage", "mssngr_messages_in_fanout_597_summ"),
    ("SubscriptionRequest", "mssngr_subscription_in_fanout_597_summ"),
    ("EditHistoryRequest", "mssngr_edit_history_fanout_597_summ"),
    ("HistoryRequest", "mssngr_history_in_fanout_597_summ")
])
def test_messenger_metrics_for_597(processor_name, counter_name):
    _test_messenger_59x_error(processor_name, counter_name, HTTPError.CODE_CONNECTION_DROPPED)
