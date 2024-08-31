import tornado.gen
import struct

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest

try:
    from cityhash import hash64 as CityHash64
except:
    from clickhouse_cityhash.cityhash import CityHash64

from alice.uniproxy.library.messenger.msgsettings import UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS

from alice.uniproxy.library.utils.json_to_proto import DictToMessage2
from alice.uniproxy.library.utils.proto_to_json import MessageToDict2
from alice.uniproxy.library.settings import UNIPROXY_INSTALLATION

from alice.uniproxy.library.auth.tvm2 import get_fanout_service_token

from alice.uniproxy.library.messenger.exception import MessengerError


# ====================================================================================================================
class MessengerClient(object):
    # ----------------------------------------------------------------------------------------------------------------
    def __init__(self, settings, **kwargs):
        super().__init__()
        self._host = settings.get('host', 'localhost')
        self._disable_tvm = settings.get('disable_tvm', False)
        self._request_timeout = settings.get('timeout', 10.0)
        self._client = QueuedHTTPClient.get_client(
            host=settings.get('host'),
            port=settings.get('port'),
            secure=settings.get('secure'),
            queue_size=1,
            pool_size=settings.get('pool', {}).get('size', 1)
        )

    # ----------------------------------------------------------------------------------------------------------------
    def _validate_settings(self):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def convert_to_request(self, request_type, payload):
        message = DictToMessage2(request_type, payload, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)
        data = message.SerializeToString()
        header = struct.pack('<IQ', 2, CityHash64(data))
        return header + data

    # ----------------------------------------------------------------------------------------------------------------
    def _create_version_error(self, version):
        return {
            'error': {
                'type': 'Error',
                'message': 'invalid version {}, expected 2'.format(version),
            },
            'details': {
                'scope': 'fanout',
                'code': 'invalid_header',
                'text': 'invalid version {}'.format(version)
            }
        }

    # ----------------------------------------------------------------------------------------------------------------
    def _create_checksum_error(self, expected_checksum, checksum):
        return {
            'error': {
                'type': 'Error',
                'message': 'invalid checksum {}, expected {}'.format(checksum, expected_checksum),
            },
            'details': {
                'scope': 'fanout',
                'code': 'invalid_header',
                'text': 'invalid checksum {}'.format(checksum)
            }
        }

    # ----------------------------------------------------------------------------------------------------------------
    def _try_parse_text(self, data: bytes):
        if isinstance(data, str):
            return data
        try:
            return data.decode('utf-8')
        except Exception:
            return None

    # ----------------------------------------------------------------------------------------------------------------
    def _create_empty_response_error(self):
        return {
            'error': {
                'type': 'Error',
                'message': 'fanout error'
            },
            'details': {
                'scope': 'fanout',
                'code': 'generic',
                'text': 'empty response'
            }
        }

    # ----------------------------------------------------------------------------------------------------------------
    def _try_create_text_error(self, code, data, force=False):
        text = self._try_parse_text(data)
        if text is not None:
            return True, {
                'error': {
                    'type': 'Error',
                    'message': text,
                },
                'details': {
                    'scope': 'fanout',
                    'code': str(code),
                    'text': text
                }
            }

        if force:
            return True, {
                'error': {
                    'type': 'Error',
                    'message': str(code),
                },
                'details': {
                    'scope': 'fanout',
                    'code': str(code),
                    'text': 'fanout error'
                }
            }

        return False, None

    # ----------------------------------------------------------------------------------------------------------------
    def convert_to_response(self, code, response_type, data):
        if data is None:
            return False, self._create_empty_response_error()

        if len(data) < 12:
            _, result = self._try_create_text_error(data, True)
            return False, result

        header, body = data[:12], data[12:]

        version = None
        checksum = None
        try:
            version, checksum = struct.unpack('<IQ', header)
        except Exception:
            pass

        if version != 2:
            ok, result = self._try_create_text_error(code, data)
            if ok:
                return False, result
            else:
                return False, self._create_version_error(version)

        body_checksum = CityHash64(body)
        if body_checksum != checksum:
            ok, result = self._try_create_text_error(code, data)
            if ok:
                return False, result
            else:
                return False, self._create_checksum_error(checksum, body_checksum)

        try:
            message = response_type()
            message.ParseFromString(body)

            response = MessageToDict2(message, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)
        except Exception as ex:
            return False, {
                'error': {
                    'type': 'Error',
                    'message': str(ex),
                },
                'details': {
                    'scope': 'uniproxy',
                    'code': 'conversion_failure',
                    'text': str(ex),
                }
            }

        return True, response

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def fetch(self, request_url, request_type, response_type, payload, accept_gzip=False):
        if not self._client.free_workers:
            return False, {
                'error': {
                    'type': 'Error',
                    'message': 'all fanout connections are in use',
                },
                'details': {
                    'scope': 'fanout',
                    'code': 'no_free_connection',
                    'text': 'all fanout connections are in use',
                }
            }

        request_body = self.convert_to_request(request_type, payload)

        if not self._disable_tvm:
            service_ticket = yield get_fanout_service_token()
        else:
            service_ticket = ''

        headers = {
            'Host': self._host,
            'Content-Type': 'application/octet-stream',
            'X-Ya-Service-Ticket': service_ticket,
            'X-Rproxy-Client': UNIPROXY_INSTALLATION,
        }

        if accept_gzip:
            headers.update({
                'Accept-Encoding': 'gzip',
            })

        request = HTTPRequest(
            request_url,
            method='POST',
            headers=headers,
            body=request_body,
            request_timeout=self._request_timeout,
            retries=3
        )

        response = yield self._client.fetch(request, raise_error=False)

        ok, payload = self.convert_to_response(response.code, response_type, response.body)

        if not ok:
            raise MessengerError(response.code, payload, raw=True)

        if response.code != 200:
            raise MessengerError(response.code, payload)

        return ok, payload
