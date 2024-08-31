import base64
import collections
import grpc
import hashlib
import http
import json
import logging
import os
import re
import requests
import socket
import threading
import time
import urllib.parse
from concurrent import futures
from google.protobuf import json_format
from typing import List
from wsgiref import simple_server

import falcon

from enum import Enum
from slugify import slugify

from alice.hollywood.library.python.testing.stubber.stubber_config_pb2 import TStubberConfig
from apphost.lib.grpc.protos.service_pb2 import TPingRequest, TPingResponse, TServiceRequest, TServiceResponse
from apphost.lib.grpc.protos.service_pb2_grpc import TServantStub, TServantServicer, add_TServantServicer_to_server
from infra.yp_service_discovery.api import api_pb2
from infra.yp_service_discovery.python.resolver.resolver import Resolver
import library.python.codecs as codecs
import yatest.common as yc

logger = logging.getLogger(__name__)


def compress_item_data(data, codec_id=0):
    if codec_id == 0:
        return b"\0" + data
    if codec_id == 2:
        return codecs.dumps("lz4", data)
    raise RuntimeError(f"Unable to compress data with codec_id={codec_id}")


def decompress_item_data(data):
    codec_id = data[0]
    if codec_id == 0:
        return data[1:]
    if codec_id == 2:
        return codecs.loads("lz4", data[1:])
    raise RuntimeError(f"Unable to unpack data with codec_id={codec_id}")


def pack_protobuf(item, codec_id=0):
    return compress_item_data(b"p_" + item.SerializeToString(), codec_id=codec_id)


def extract_protobuf(data, protobuf_type):
    data = decompress_item_data(data)
    if not data.startswith(b"p_"):
        raise RuntimeError(f"There is no protobuf in data starts with {data[:10]}...")
    item = protobuf_type()
    item.ParseFromString(data[2:])
    return item


def convert_proto_to_json(proto_obj, type_to_proto):
    json_obj = json_format.MessageToDict(proto_obj)
    answers = json_obj.get('Answers', [])
    for i, answer in enumerate(answers):
        proto_type = type_to_proto.get(answer['Type'])
        if proto_type is None:
            continue
        sub_proto_obj = extract_protobuf(proto_obj.Answers[i].Data, proto_type)
        proto_json_obj = json_format.MessageToDict(sub_proto_obj)
        answer['Data'] = proto_json_obj
    return json_obj


def convert_json_to_proto(json_obj, proto_cls, type_to_proto):
    proto_obj = proto_cls()

    answers = json_obj.get('Answers', [])
    for i, answer in enumerate(answers):
        proto_type = type_to_proto.get(answer['Type'])
        if proto_type is None:
            continue
        proto_subobj = proto_type()
        json_format.ParseDict(answer['Data'], proto_subobj)
        answer['Data'] = pack_protobuf(proto_subobj)
        if isinstance(answer['Data'], bytes):
            answer['Data'] = base64.b64encode(answer['Data']).decode()
    json_format.ParseDict(json_obj, proto_obj)

    return proto_obj


def headers_to_str(headers):
    result_headers = {
        key: '[censored]' if key.lower() == 'authorization' else value
        for key, value in headers.items()}
    return str(result_headers)


def ensure_filter_regex_is_valid(filter_regex):
    for i in range(len(filter_regex)):
        curr = filter_regex[i]
        prev = filter_regex[i - 1] if i > 0 else None
        if curr.isupper() and prev != '\\':
            raise Exception(f'Uppercase character not allowed in `{filter_regex}` at pos={i}')


def filter_headers(headers, keys_black_list):
    keys_black_set = frozenset([key for key in keys_black_list])

    for key_re in keys_black_set:
        ensure_filter_regex_is_valid(key_re)

    result = {
        key.lower(): value for key, value in headers.items()
        if not any([re.match(key_re, key.lower()) for key_re in keys_black_set])
    }

    req_id_header = result.pop('x-request-id', None)
    if req_id_header is not None:
        idx = req_id_header.find('_')
        if idx != -1:
            req_id_header = req_id_header[:idx]
        result['x-request-id'] = req_id_header

    return result


def filter_cgi_query(query, params_black_list):
    logger.info(f'query before filter: {query}, cgi_filter_regexps: {params_black_list}')
    params = urllib.parse.parse_qs(query)
    filtered = {
        name: value for name, value in params.items()
        if not any([re.match(name_re, name) for name_re in params_black_list])
    }
    result = urllib.parse.urlencode(filtered, doseq=True)
    logger.info(f'query after filter: {result}')
    return result


def get_header_value(headers, header_name):
    key = next(filter(lambda name: name.lower() == header_name.lower(), headers.keys()), None)
    if key is not None:
        return headers[key].lower()
    else:
        return None


STUBBER_SERVER_MODE_ENV_VARIABLE = 'STUBBER_SERVER_MODE'

DEFAULT_REQUEST_HEADER_FILTERS = [
    'x-rtlog-token',
    'x-apphost-.*',
    'x-yandex-balancer-retry',
    'authorization', 'x-oauth-token',  # They are fake in CI environment
    'x-ya-user-ticket', 'x-ya-service-ticket',  # They are fake (or absent?) in CI environment
    'host',  # Because host contains random port, taken from PortManager
]


class StubberMode(Enum):
    """StubberMode configure stubber behavior through env variable (use --test-env STUBBER_SERVER_MODE=_STUBBER_MODE_ in tests)"""

    UseStubsUpdateNone = 'USE_STUBS_UPDATE_NONE'
    UseUpstreamUpdateAll = 'USE_UPSTREAM_UPDATE_ALL'
    UseStubsUpdatePartial = 'USE_STUBS_UPDATE_PARTIAL'


class RequestHashingMode(Enum):
    """For REST-like request like /object/{id}/action previously path pattern was used when hashing request,
    which is in many cases wrong. Set mode to USE_REQUEST_PATH to use real request path for hashing."""

    UsePatternPath = 'USE_PATTERN_PATH'
    UseRequestPath = 'USE_REQUEST_PATH'


class RequestHandler:
    def __init__(self, stubs_data_path, methods, path, upstream_host, upstream_port, upstream_scheme,
                 stubber_mode, stubs_usage_counts, idempotent, hash_request_path, content_procs=None,
                 header_filter_regexps=None, cgi_filter_regexps=None, type_to_proto={}, request_content_hasher=None):
        self._stubs_data_path = stubs_data_path
        self._methods = methods
        self._path = path
        self._upstream_host = upstream_host
        self._upstream_port = upstream_port
        self._upstream_scheme = upstream_scheme
        self._stubber_mode = stubber_mode
        self._stubs_usage_counts = stubs_usage_counts
        self._idempotent = idempotent
        self._req_hash_mode = RequestHashingMode.UseRequestPath if hash_request_path else RequestHashingMode.UsePatternPath
        self._content_procs = list(content_procs or [])
        self._header_filter_regexps = list(header_filter_regexps or [])
        self._cgi_filter_regexps = list(cgi_filter_regexps or [])
        self._type_to_proto = type_to_proto
        self._pseudo_grpc = any(k in type_to_proto for k in ['pseudo_grpc_request', 'pseudo_grpc_response'])
        logger.info(f'self._req_hash_mode={self._req_hash_mode}')
        self._response_stubs = None
        self._requests_handled_count = 0
        self._request_content_hasher = request_content_hasher

    def on_post(self, req, resp, **kwargs):
        logger.info(f'on_post req={req}')
        self._on_request(req, resp, 'POST')

    def on_get(self, req, resp, **kwargs):
        logger.info(f'on_get req={req}')
        self._on_request(req, resp, 'GET')

    def on_put(self, req, resp, **kwargs):
        logger.info(f'on_put req={req}')
        self._on_request(req, resp, 'PUT')

    def on_delete(self, req, resp, **kwargs):
        logger.info(f'on_delete req={req}')
        self._on_request(req, resp, 'DELETE')

    def freeze_stubs(self, response_stubs):
        self._response_stubs = response_stubs

    def _on_request(self, req, resp, method):
        if self._methods and method not in self._methods:
            resp.status = falcon.HTTP_405
            resp.text = f'Bad method {method}, expected {self._methods}'
            logger.error(resp.text)
        else:
            self._handle_request(req, resp, method)

    def _handle_request(self, req, resp, method):
        try:
            self._handle_request_unsafe(req, resp, method)
        except Exception as exc:
            logger.exception(exc)
            raise

    def _handle_request_unsafe(self, req, resp, method):
        content = req.stream.read(req.content_length or 0)
        logger.info(f'Received request: \
                    PATH={req.path} \
                    QUERY_STRING={req.query_string} \
                    HEADERS={headers_to_str(req.headers)} \
                    DATA={content}. \
                    Processing in {STUBBER_SERVER_MODE_ENV_VARIABLE}={self._stubber_mode}')

        if req.headers.get('X-YANDEX-BALANCER-RETRY') in ['Retry', 'Hedged']:
            logger.info('Processing stopped because this is a hedged/retry request')
            self._update_response(resp, 504, {}, "Gateway Timeout imitation for hedged/retry requests")
            return

        self._requests_handled_count += 1

        if self._response_stubs:
            next_response_stub_index = (self._requests_handled_count - 1) % len(self._response_stubs)
            response_stub = self._response_stubs[next_response_stub_index]
            if response_stub.content_filename:
                logger.info(f'This handle has a freezed stub ({response_stub.content_filename}), returning it without '
                            f'going to the upstream. Status code={response_stub.status_code}, '
                            f'headers={response_stub.headers}, content={response_stub.content}')
            else:
                logger.info(f'This handle has a freezed stub, returning it without going to the upstream. '
                            f'Status code={response_stub.status_code}, headers={response_stub.headers}, '
                            f'content={response_stub.content}')
            self._update_response(resp, response_stub.status_code, response_stub.headers, response_stub.content)
            return

        stub_filename, stub_filename_building_blocks = self._make_stub_filename(req.headers, method, req.path,
                                                                                req.query_string, content)

        read_stub = True  # whether we should read the old stub and not create a new stub

        if self._stubber_mode == StubberMode.UseStubsUpdateNone:
            if not os.path.exists(stub_filename):
                raise Exception(f'Stub file {stub_filename} not found, cannot proceed because mode is '
                                f'{self._stubber_mode}. Maybe you should (re)run it2/generator?')

            # Uncomment to patch .info files with 'stub_filename_building_blocks' data...
            # self._inject_stub_filename_building_blocks_to_stub_info(stub_filename_building_blocks, stub_filename)
        elif self._stubber_mode == StubberMode.UseUpstreamUpdateAll:
            read_stub = False
        elif self._stubber_mode == StubberMode.UseStubsUpdatePartial:
            read_stub = os.path.exists(stub_filename)
        else:
            raise Exception(f'Unknown stubber behavior: {STUBBER_SERVER_MODE_ENV_VARIABLE}={self._stubber_mode}')

        if read_stub:
            stub_content, stub_info = self._read_stub(stub_filename)
        else:
            resp_status_code, resp_headers, stub_content = self._request_upstream(req.query_string, req.path,
                                                                                    req.headers, content, method)
            stub_info = self._make_stub_info(resp_status_code, resp_headers, req.path, req.query_string, content,
                                                stub_filename_building_blocks)
            stub_content = self._preprocess_response_content(stub_content, resp_headers)
            self._write_stub(stub_filename, stub_content, stub_info)
            stub_content, stub_info = self._read_stub(stub_filename)

        logger.debug(f"Increase usage count for stub {stub_filename}")
        self._stubs_usage_counts[stub_filename] += 1
        self._update_response(resp, stub_info['status_code'], stub_info.get('headers', {}), stub_content)

    def _filter_request_headers(self, headers):
        header_filters = self._header_filter_regexps + DEFAULT_REQUEST_HEADER_FILTERS
        headers_filtered = filter_headers(headers, header_filters)

        # This is just an imitation of old behaviour. Then apphost use NEH lib, it send empty content-length header
        # TODO del if and make new stub hash
        if 'content-length' in headers_filtered and headers_filtered['content-length'] == '0':
            headers_filtered['content-length'] = ''
        headers_sorted = list(headers_filtered.items())
        headers_sorted.sort(key=lambda item: item[0])
        headers_sorted = None if not headers_sorted else headers_sorted  # This is just an imitation of old behaviour
        # (using None instead of []), because we do not want all the existing stub filenames to diverge at once...
        return headers_sorted

    def _make_stub_filename(self, headers, method, req_path, query='', content=b''):
        stub_filename_building_blocks = {}
        # Remove all the variadic info from the hash, we need reproducible stub filenames
        headers_sorted = self._filter_request_headers(headers)
        logger.info(f'Request headers after filter: {headers_sorted}')

        if len(self._cgi_filter_regexps) != 0:
            query = filter_cgi_query(query, self._cgi_filter_regexps)

        if self._req_hash_mode == RequestHashingMode.UseRequestPath:
            path = req_path
        else:
            path = self._path

        part1 = f'{method}{path}{headers_sorted}{query}'.encode('utf-8')
        stub_filename_building_blocks['method'] = method
        stub_filename_building_blocks['path'] = path
        # NOTE: it's feeded to hash as str(headers_sorted)
        # NOTE2: headers_sorted is a list of tuples, but *.info is JSON, so it's a list of lists there
        stub_filename_building_blocks['headers_sorted'] = headers_sorted
        stub_filename_building_blocks['query'] = query

        h = hashlib.sha1(part1)
        if not self._pseudo_grpc:
            # pseudo-gRPC requests are hard to remove variadic elements
            preprocessed_request_content = self._preprocess_request_content(headers, content)
            if type(preprocessed_request_content) != bytes:
                raise Exception('request_content_hasher must return bytes data type!')
            h.update(preprocessed_request_content)
            stub_filename_building_blocks['preprocessed_request_content_in_base64'] = \
                base64.b64encode(preprocessed_request_content).decode()

        filename = f'{slugify(method)}_{slugify(path)}_{h.hexdigest()}'

        if not self._idempotent:
            usage_count = sum(
                stub_usage_count > 0
                for stub_path, stub_usage_count in self._stubs_usage_counts.items()
                if os.path.basename(stub_path).startswith(filename)
            )
            filename += f'_{usage_count:02}'
        return os.path.join(self._stubs_data_path, filename), stub_filename_building_blocks

    def _preprocess_request_content(self, headers, content):
        if self._request_content_hasher:
            return self._request_content_hasher(headers, content)
        if content and get_header_value(headers, 'content-type') == 'application/json':
            sorted_json = self._sort_json_content_str(content.decode('utf-8'))
            return sorted_json.encode('utf-8')
        return content

    def _sort_json_content_str(self, content, json_dump_indent=None):
        content_obj = json.loads(content)
        result = json.dumps(content_obj, ensure_ascii=False, sort_keys=True, indent=json_dump_indent)
        logger.info(f'Sorted content is {result}')
        return result

    def _make_stub_info_filename(self, stub_filename):
        return stub_filename + '.info'

    def _request_upstream(self, query, path, original_headers, content, method):
        if not path.startswith('/'):
            path = '/' + path
        url = f'{self._upstream_scheme}://{self._upstream_host}:{self._upstream_port}{path}'
        if query:
            url += '?' + query

        headers = filter_headers(original_headers, ['host'])
        if headers.get('content-length') == '':  # falcon sends us Content-Type: '' for GET requests
            del headers['content-length']

        logger.info(f'Requesting the upstream server url={url}, method={method}, headers={headers_to_str(headers)}')
        if method == 'POST':
            resp = requests.post(url, data=content, headers=headers)
        elif method == 'GET':
            resp = requests.get(url, headers=headers)
        elif method == 'PUT':
            resp = requests.put(url, data=content, headers=headers)
        elif method == 'DELETE':
            resp = requests.delete(url, headers=headers)
        else:
            raise Exception(f'Bad upstream method value, {method}')

        logger.info(f'Upstream response Code: {resp.status_code}, Content: {resp.text}')
        return resp.status_code, resp.headers, resp.content

    def _content_to_str(self, content):
        if self._pseudo_grpc:
            req = self._type_to_proto['pseudo_grpc_request']()
            req.MergeFromString(content)
            json_obj = convert_proto_to_json(req, self._type_to_proto)
            return json.dumps(json_obj)

        try:
            # content is human-readable
            return content.decode()
        except:
            # content is not human-readable
            return base64.b64encode(content).decode()

    def _make_stub_info(self, status_code, headers, req_path, req_query_string, req_content,
                        stub_filename_building_blocks):
        stub_info = {}
        stub_info['status_code'] = status_code
        stub_info['headers'] = {}
        for key in headers:
            stub_info['headers'][key] = headers.get(key)

        stub_info['request'] = self._make_stub_info_request(req_path, req_query_string, req_content)
        stub_info['stub_filename_building_blocks'] = stub_filename_building_blocks
        return stub_info

    def _make_stub_info_request(self, req_path, req_query_string, req_content):
        request_info = {}
        request_info['path'] = req_path
        request_info['query_string'] = req_query_string
        if req_content:
            request_info['content'] = self._content_to_str(req_content)
            try:
                content_json = json.loads(request_info['content'])
                request_info['content'] = content_json
            except:
                pass
        return request_info

    def _read_stub(self, stub_filename):
        logger.info(f'Reading stub {stub_filename} from filesystem...')
        with open(stub_filename, 'rb') as f:
            stub_content = f.read()
        if self._pseudo_grpc:
            stub_json = json.loads(stub_content)
            stub_obj = convert_json_to_proto(stub_json, self._type_to_proto['pseudo_grpc_response'], self._type_to_proto)
            stub_content = stub_obj.SerializeToString()

        stub_info = self._read_stub_info(stub_filename)

        return stub_content, stub_info

    def _read_stub_info(self, stub_filename):
        stub_info_filename = self._make_stub_info_filename(stub_filename)
        with open(stub_info_filename) as f:
            stub_info = json.loads(f.read())
        return stub_info

    def _inject_stub_filename_building_blocks_to_stub_info(self, stub_filename_building_blocks, stub_filename):
        logger.info(f'Injecting stub_filename_building_blocks into {stub_filename}.info...')
        stub_info = self._read_stub_info(stub_filename)
        stub_info['stub_filename_building_blocks'] = stub_filename_building_blocks
        self._write_stub_info(stub_filename, stub_info)

    def _preprocess_response_content(self, content, headers):
        assert isinstance(content, bytes)

        preprocessed_content = content
        for content_proc in self._content_procs:
            preprocessed_content = content_proc(headers, preprocessed_content)
            assert isinstance(preprocessed_content, bytes)

        contentType = get_header_value(headers, 'content-type')
        if contentType is not None and contentType.startswith('application/json'):
            sorted_json_content = self._sort_json_content_str(preprocessed_content.decode('utf-8'), json_dump_indent=4)
            preprocessed_content = sorted_json_content.encode('utf-8')

        assert isinstance(preprocessed_content, bytes)
        return preprocessed_content

    def _write_stub(self, stub_filename, content, stub_info):
        logger.info(f'Writing stub file {stub_filename} to filesystem...')
        if self._pseudo_grpc:
            req = self._type_to_proto['pseudo_grpc_response']()
            req.MergeFromString(content)
            json_obj = convert_proto_to_json(req, self._type_to_proto)
            with open(stub_filename, 'w') as f:
                json.dump(json_obj, f, indent=4, ensure_ascii=False)
        else:
            with open(stub_filename, 'wb') as f:
                f.write(content)

        self._write_stub_info(stub_filename, stub_info)

    def _write_stub_info(self, stub_filename, stub_info):
        stub_info_filename = self._make_stub_info_filename(stub_filename)
        with open(stub_info_filename, 'w') as f:
            json.dump(stub_info, fp=f, ensure_ascii=False, indent=4)

    def _update_response(self, resp, status_code, headers, content):
        resp.status = f'{status_code} {http.client.responses[status_code]}'

        hop_by_hop_headers_rfc2616 = ['connection',
                                      'keep-alive',
                                      'proxy-authenticate',
                                      'proxy-authorization',
                                      'te',
                                      'trailers',
                                      'transfer-encoding',
                                      'upgrade']
        resp_headers = filter_headers(
            headers,
            ["content-encoding"] +  # We remove this header to send back raw data without any compression
            hop_by_hop_headers_rfc2616)  # Hop-by-hop headers are not allowed in final response
        set_cookie = resp_headers.pop('set-cookie', None)
        resp.set_headers(resp_headers)
        if set_cookie:
            resp.append_header('set-cookie', set_cookie)

        resp.text = content


class PingHandler:
    PATH = '/ping'

    def on_get(self, req, resp):
        logger.info(f'on_get {req.path}')
        resp.status = falcon.HTTP_OK
        resp.text = 'pong'


class StubberServerBase:
    def __init__(self, stubs_data_path, config, all_content_procs=None, header_filter_regexps=None, type_to_proto={},
                 source_name_filter=set(), request_content_hashers=None):
        logger.debug(f'Initializing StubberServer... stubs_data_path={stubs_data_path}, all_content_procs={all_content_procs}, request_content_hashers={request_content_hashers}')
        self._mkdirs(stubs_data_path)
        self._stubs_usage_counts = self._init_stubs_usage_counts(stubs_data_path)
        self._stubs_data_path = stubs_data_path
        self._config = config
        self._port = config.Port
        self._has_errors = False
        self._all_content_procs = dict(all_content_procs or {})
        self._header_filter_regexps = header_filter_regexps
        self._type_to_proto = type_to_proto
        self._source_name_filter = source_name_filter
        self._request_content_hashers = dict(request_content_hashers or {})

        if STUBBER_SERVER_MODE_ENV_VARIABLE in os.environ:
            self._stubber_mode = StubberMode(os.environ.get(STUBBER_SERVER_MODE_ENV_VARIABLE))
        elif config.StubberMode:
            self._stubber_mode = StubberMode(config.StubberMode)
        else:
            self._stubber_mode = StubberMode.UseStubsUpdateNone

    @property
    def has_errors(self):
        return self._has_errors

    @property
    def unused_stubs(self):
        return [key for key, val in self._stubs_usage_counts.items() if val == 0]

    @property
    def used_stubs(self):
        return [key for key, val in self._stubs_usage_counts.items() if val > 0]

    @property
    def stubber_mode(self):
        return self._stubber_mode

    def start(self):
        raise NotImplementedError()

    def stop(self):
        raise NotImplementedError()

    def ping(self):
        raise NotImplementedError()

    def _mkdirs(self, stubs_data_path):
        # TODO(sparkle): resolve circular dependency of "is_generator_mode"
        if not os.path.exists(stubs_data_path) and 'IT2_GENERATOR' in yc.context.flags:
            os.makedirs(stubs_data_path, exist_ok=True)

    def _init_stubs_usage_counts(self, stubs_data_path):
        result = collections.defaultdict(int)
        file_names = os.listdir(stubs_data_path) if os.path.exists(stubs_data_path) else []
        for file_name in file_names:
            file_name_abs = os.path.join(stubs_data_path, file_name)
            if os.path.isdir(file_name_abs):
                continue
            if file_name.startswith('run_request'):
                continue
            prefix, extention = os.path.splitext(file_name_abs)
            if extention == '.info':
                file_name_abs = prefix
            result[file_name_abs] = 0
        logger.info(f'Found {len(result)} stubs in {stubs_data_path}')
        return result


class HttpStubberServer(StubberServerBase):
    def __init__(self, stubs_data_path, config, all_content_procs=None, header_filter_regexps=None, type_to_proto={},
                 source_name_filter=set(), request_content_hashers=None):
        super(HttpStubberServer, self).__init__(stubs_data_path, config, all_content_procs, header_filter_regexps,
                                                type_to_proto, source_name_filter, request_content_hashers)
        self._handlers = {}

    def start(self):
        app = falcon.API()

        pingHandler = PingHandler()
        app.add_route(PingHandler.PATH, pingHandler)

        def default_error_handler(ex, req, resp, params):
            self._has_errors = True
            logger.error(f'Exception in stubber {ex!r}, request is {req}')
            raise falcon.HTTPInternalServerError('Exception in stubber', f'{ex!r}')

        app.add_error_handler(Exception, default_error_handler)

        for path, upstream_config in self._config.Upstreams.items():
            assert path != PingHandler.PATH
            handler = RequestHandler(self._stubs_data_path, upstream_config.Methods, path,
                                     upstream_config.Host, upstream_config.Port, upstream_config.Scheme,
                                     self._stubber_mode, self._stubs_usage_counts,
                                     not upstream_config.NonIdempotent,
                                     not upstream_config.HashPatternPath,
                                     self._all_content_procs.get(path),
                                     self._header_filter_regexps,
                                     upstream_config.CgiFilterRegexps,
                                     self._type_to_proto,
                                     self._request_content_hashers.get(path))
            app.add_route(path, handler)
            self._handlers[path] = handler
            logger.debug(f"handler added: {upstream_config.Methods} {upstream_config.Scheme}://{upstream_config.Host}:{upstream_config.Port}{path}")

        logger.debug("HttpStubberServer listening on port %s", self._port)
        self._server = simple_server.make_server('', self._port, app)
        self._server.serve_forever()
        logger.debug("HttpStubberServer finished.")

    def stop(self):
        logger.debug("Stopping HttpStubberServer...")
        self._server.shutdown()
        logger.debug('Stopped HttpStubberServer')

    def ping(self):
        try:
            resp = requests.get(f'http://localhost:{self._port}{PingHandler.PATH}')
            if resp.status_code == 200 and resp.text == 'pong':
                return True
        except requests.exceptions.ConnectionError:
            pass
        return False

    def freeze_stubs(self, path, response_stubs):
        assert path in self._handlers
        self._handlers[path].freeze_stubs(response_stubs)


class GrpcStubberServer(StubberServerBase):
    def __init__(self, stubs_data_path, config, all_content_procs=None, header_filter_regexps=None, type_to_proto={},
                 source_name_filter=set(), request_content_hashers=None):
        super(GrpcStubberServer, self).__init__(stubs_data_path, config, all_content_procs, header_filter_regexps,
                                                type_to_proto, source_name_filter, request_content_hashers)

    def start(self):
        self._server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        add_TServantServicer_to_server(self.Servicer(self), self._server)
        self._server.add_insecure_port(f'localhost:{self._port}')
        self._stub = None

        logger.debug('GrpcStubberServer listening on port %s', self._port)
        self._server.start()
        self._server.wait_for_termination()
        logger.debug('GrpcStubberServer finished.')

    def stop(self):
        logger.debug('Stopping GrpcStubberServer...')
        self._server.stop(grace=True)
        logger.debug('Stopped GrpcStubberServer')

    def ping(self):
        channel = grpc.insecure_channel(f'localhost:{self._port}')
        ping_res = None
        try:
            stub = TServantStub(channel)
            ping_res = stub.Ping(TPingRequest())
        except:
            pass
        return ping_res == TPingResponse()

    def invoke(self, request_iterator, context):
        logger.debug('GrpcStubberServer Invoke.')

        reqs = [r for r in request_iterator]
        assert len(reqs) == 1  # TODO(sparkle): redo if not true
        req = reqs[0]

        stub_filename = self._make_stub_filename(req)

        if self._stubber_mode == StubberMode.UseStubsUpdateNone:
            if not os.path.exists(stub_filename):
                raise Exception(f'Stub file {stub_filename} not found, cannot proceed because mode is {self._stubber_mode}. Maybe you should (re)run it2/generator?')
            invoke_res = self._read_stub(stub_filename)
        else:
            # TODO(sparkle): implement other modes
            if self._stub is None:
                channel = grpc.insecure_channel(f'{self._config.ServiceHost}:{self._config.ServicePort}')
                self._stub = TServantStub(channel)

            invoke_res = self._stub.Invoke((r for r in reqs))
            self._save_stub(invoke_res, stub_filename)
            self._save_stub_info(req, self._make_stub_info_filename(stub_filename))

        logger.debug(f"Increase usage count for stub {stub_filename}")
        self._stubs_usage_counts[stub_filename] += 1

        return invoke_res

    def freeze_stubs(self, path, response_stubs):
        raise NotImplementedError()

    def _read_stub(self, stub_filename):
        with open(stub_filename, 'r') as f:
            json_obj = json.loads(f.read())
        return convert_json_to_proto(json_obj, TServiceResponse, self._type_to_proto)

    def _save_stub(self, stub, stub_filename):
        json_obj = convert_proto_to_json(stub, self._type_to_proto)
        with open(stub_filename, 'w') as f:
            f.write(json.dumps(json_obj, indent=4, sort_keys=True).encode().decode('unicode-escape'))

    def _save_stub_info(self, request, stub_info_filename):
        json_obj = convert_proto_to_json(request, self._type_to_proto)
        with open(stub_info_filename, 'w') as f:
            f.write(json.dumps(json_obj, indent=4, sort_keys=True).encode().decode('unicode-escape'))

    def _make_stub_filename(self, request):
        # Remove all the variadic info from the hash, we need reproducible stub filenames
        h = hashlib.sha1(request.Path.encode())
        for answer in request.Answers:
            if answer.SourceName in self._source_name_filter | {'APP_HOST_PARAMS', '_DEBUG_INFO'}:
                continue
            h.update(answer.SourceName.encode())
            h.update(answer.Type.encode())
            h.update(answer.Data)
        filename = f'grpc_{slugify(request.Path)}_{h.hexdigest()}'

        # TODO(sparkle): if endpoints are idempotent - redo this
        usage_count = sum(
            stub_usage_count > 0
            for stub_path, stub_usage_count in self._stubs_usage_counts.items()
            if os.path.basename(stub_path).startswith(filename)
        )
        filename += f'_{usage_count:02}'
        return os.path.join(self._stubs_data_path, filename)

    def _make_stub_info_filename(self, stub_filename):
        return stub_filename + '.info'

    # corresponding API: https://a.yandex-team.ru/arc/trunk/arcadia/apphost/lib/grpc/protos/service.proto?rev=r8320342#L12-18
    class Servicer(TServantServicer):
        def __init__(self, stubber):
            self._stubber = stubber

        def Invoke(self, request_iterator, context):
            return self._stubber.invoke(request_iterator, context)

        def Ping(self, request, context):
            return TPingResponse()


class StubberServerThread(threading.Thread):
    def __init__(self, stubs_data_path, config, all_content_procs=None, header_filter_regexps=None, type_to_proto={},
                 source_name_filter=set(), request_content_hashers=None):
        super(StubberServerThread, self).__init__()
        cls = GrpcStubberServer if config.Scheme == 'grpc' else HttpStubberServer
        self._stubber_server = cls(stubs_data_path, config, all_content_procs, header_filter_regexps, type_to_proto,
                                   source_name_filter, request_content_hashers)
        self.daemon = True

    @property
    def has_errors(self):
        return self._stubber_server.has_errors

    @property
    def unused_stubs(self):
        return self._stubber_server.unused_stubs

    @property
    def used_stubs(self):
        return self._stubber_server.used_stubs

    @property
    def stubber_mode(self):
        return self._stubber_server.stubber_mode

    def run(self):
        logger.info('Starting thread...')
        try:
            self._stubber_server.start()
        except Exception as exc:
            logger.exception(exc)
        finally:
            logger.info('Thread finished.')

    def start_async(self):
        logger.info('Thread is about to start...')
        self.start()

    def start_sync(self):
        self.start_async()
        while True:
            logger.info('Pinging StubberServer...')
            if self._stubber_server.ping():
                break
            time.sleep(0.1)
        logger.info('StubberServer is ready.')

    def stop_sync(self):
        logger.info('Thread is about to stop...')
        self._stubber_server.stop()
        self.join(300)
        logger.info('Thread stopped.')

    def freeze_stubs(self, path, response_stubs):
        self._stubber_server.freeze_stubs(path, response_stubs)


class StubberContextManager(object):
    def __init__(self, stubs_data_path, config, expect_errors=False, all_content_procs=None,
                 header_filter_regexps=None, type_to_proto={}, source_name_filter=set(),
                 request_content_hashers=None):
        self._stubber_server_thread = StubberServerThread(stubs_data_path, config, all_content_procs,
                                                          header_filter_regexps, type_to_proto,
                                                          source_name_filter, request_content_hashers)
        self._stubs_data_path = stubs_data_path

        freezed_stubs_dir, _ = os.path.split(stubs_data_path)
        self._freezed_stubs_dir, _ = os.path.split(freezed_stubs_dir)

        self._port = config.Port
        self._expect_errors = expect_errors
        self._disable_stubs_usage_checks = config.DisableStubsUsageChecks

    def __enter__(self):
        self._stubber_server_thread.start_sync()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._stubber_server_thread.stop_sync()
        assert self._expect_errors == self._stubber_server_thread.has_errors, f'Errors expected: {self._expect_errors}'

        read_only_modes = [StubberMode.UseStubsUpdateNone]
        write_modes = [StubberMode.UseUpstreamUpdateAll, StubberMode.UseStubsUpdatePartial]

        if not self._disable_stubs_usage_checks:
            logger.info('Checking stub files usage...')
            unused_stubs = self._collect_unused_files_in_stubs_data_path(self._stubber_server_thread.used_stubs)
            logger.info(f'Found unused_stubs={unused_stubs}')
            if self._stubber_server_thread.stubber_mode in read_only_modes and unused_stubs:
                raise Exception(f'Unused trash files found in {self._stubs_data_path}: '
                                f'{json.dumps(unused_stubs, indent=4)}\n'
                                'WHAT TO DO: 1) Remove them manually (preferred) 2) Run it/it2 generator for that test '
                                'case\n'
                                '3) ONLY if 1 and 2 do not help. It is possible that it/runner and it/generator '
                                'calculate stub filenames differently. Check that `stub_filename_building_blocks` data '
                                'in *.info files (which is created by it/generator) matches the corresponding data, '
                                'calculated by runner (search for it in run.log). Then avoid differences somehow, '
                                'maybe use `StubberEndpoint.request_content_hasher`... research and think!')

            elif self._stubber_server_thread.stubber_mode in write_modes:
                for filename in unused_stubs:
                    logger.info(f'Removing unused file... {filename}...')
                    self._remove_if_exists(os.path.join(self._stubs_data_path, filename))

        files_in_stub_dir = os.listdir(self._stubs_data_path) if os.path.exists(self._stubs_data_path) else []
        if self._stubber_server_thread.stubber_mode in write_modes and not files_in_stub_dir and os.path.exists(self._stubs_data_path):
            logger.info(f'Removing {self._stubs_data_path} directory, because there are no stubs')
            os.rmdir(self._stubs_data_path)

    def freeze_stubs(self, path, response_stubs):
        for response_stub in response_stubs:
            if response_stub.content_filename:
                with open(os.path.join(self._freezed_stubs_dir, response_stub.content_filename), 'r') as in_f:
                    response_stub.content = in_f.read()
        self._stubber_server_thread.freeze_stubs(path, response_stubs)

    def _collect_unused_files_in_stubs_data_path(self, used_stubs):
        used_files = {
            f'{os.path.relpath(stub, self._stubs_data_path)}{suffix}'
            for stub in used_stubs
            for suffix in ['', '.info']
        }
        logger.info(f'Considering that used non-trash files are: {used_files}')

        result = []
        if os.path.exists(self._stubs_data_path):
            for filename in os.listdir(self._stubs_data_path):
                if os.path.isdir(os.path.join(self._stubs_data_path, filename)):
                    continue
                if filename in used_files or filename.startswith('run_request'):
                    continue
                result.append(filename)
        return result

    def _remove_if_exists(self, filename):
        try:
            os.remove(filename)
            logger.info(f'File {filename} removed')
        except PermissionError:
            dirname = os.path.dirname(filename)
            logger.info(f'{filename} st_mode is {os.stat(filename).st_mode} st_mode of containing dir is {os.stat(dirname).st_mode}')
            raise
        except FileNotFoundError:
            pass

    @property
    def port(self):
        return self._port

    @property
    def stubs_data_path(self):
        return self._stubs_data_path


class StubberEndpoint:
    '''
    path is the endpoint path, e.g. '/somepath/foo/bar'
    methods is a list of HTTP methods supported by the endpoint, e.g. ['GET', 'POST', 'PUT']
    content_procs is a list of callables with signature: (headers_dict, content_bytes) -> {some code returning modified content}
    '''
    def __init__(self, path, methods, content_procs=None, idempotent=True, cgi_filter_regexps=None,
                 request_content_hasher=None):
        self.path = path
        self.methods = methods
        self.content_procs = list(content_procs or [])
        self.idempotent = idempotent
        self.cgi_filter_regexps = cgi_filter_regexps
        self.request_content_hasher = request_content_hasher


class HttpResponseStub:
    # status_code - the HTTP response status code
    # content_filename - is a relative path to filename with content for the HTTP response, relative to the
    #   root directory of all stubs. For example, if your it2 tests module is
    #   $ARCADIA_ROOT/alice/hollywood/library/scenarios/music/it2/tests_thin_client.py
    #   then the 'root directory of all stubs' is
    #   $ARCADIA_ROOT/alice/hollywood/library/scenarios/music/it2/tests_thin_client
    # content - string (or binary?) which will be used as response content
    # headers - dict of HTTP headers
    def __init__(self, status_code=200, content_filename=None, content=None, headers=None):
        self.status_code = status_code

        assert not (content_filename and content), f'You should set either content_filename or content, not both (have content_filename={content_filename}, content={content})'
        self.content_filename = content_filename
        self.content = content

        self.headers = headers or {}


class Stubber(StubberContextManager):
    def resolve_service_host(self, service_host):
        if isinstance(service_host, str):
            # service host already defined
            return service_host

        assert isinstance(service_host, tuple)
        cluster_name, endpoint_set_id = service_host

        # TODO(sparkle): resolve circular dependency of "is_generator_mode"
        if 'IT2_GENERATOR' not in yc.context.flags:
            # will not resolve host in runner mode
            return endpoint_set_id

        logger.info(f'resolve host from cluster "{cluster_name}", endpoint set id "{endpoint_set_id}"')

        resolver = Resolver(client_name='test:{}'.format(socket.gethostname()), timeout=5)
        request = api_pb2.TReqResolveEndpoints()
        request.cluster_name = cluster_name
        request.endpoint_set_id = endpoint_set_id

        result = resolver.resolve_endpoints(request)
        endpoints = result.endpoint_set.endpoints
        if len(endpoints) > 0:
            logger.info(f'{len(endpoints)} endpoints found, take the first')
            endpoint = endpoints[0]
            return endpoint.fqdn
        else:
            logger.error('no endpoints found, error!')

        return None

    def __init__(self, stubs_data_path, port, service_host, service_port, endpoints: List[StubberEndpoint], scheme,
                 stubber_mode=None, hash_pattern=False, header_filter_regexps=None, type_to_proto={},
                 pseudo_grpc=False, source_name_filter=set()):
        service_host = self.resolve_service_host(service_host)

        config = TStubberConfig()
        config.Port = port
        config.Scheme = scheme
        config.ServiceHost = service_host
        config.ServicePort = service_port
        if stubber_mode is not None:
            config.StubberMode = stubber_mode.value

        all_content_procs = {}
        request_content_hashers = {}
        for endpoint in endpoints:
            path = endpoint.path
            methods = endpoint.methods
            config.Upstreams[path].Methods.extend(methods)
            config.Upstreams[path].Host = service_host
            config.Upstreams[path].Port = service_port
            config.Upstreams[path].Scheme = scheme
            config.Upstreams[path].NonIdempotent = not endpoint.idempotent
            config.Upstreams[path].HashPatternPath = hash_pattern
            config.Upstreams[path].CgiFilterRegexps.extend(list(endpoint.cgi_filter_regexps or []))
            if len(endpoint.content_procs) > 0:
                all_content_procs[path] = endpoint.content_procs
            if endpoint.request_content_hasher:
                request_content_hashers[path] = endpoint.request_content_hasher

        if pseudo_grpc or any(k in type_to_proto for k in {'pseudo_grpc_request', 'pseudo_grpc_response'}):
            if 'pseudo_grpc_request' not in type_to_proto:
                type_to_proto['pseudo_grpc_request'] = TServiceRequest
            if 'pseudo_grpc_response' not in type_to_proto:
                type_to_proto['pseudo_grpc_response'] = TServiceResponse

        super(Stubber, self).__init__(stubs_data_path, config, all_content_procs=all_content_procs,
                                      header_filter_regexps=header_filter_regexps, type_to_proto=type_to_proto,
                                      source_name_filter=source_name_filter,
                                      request_content_hashers=request_content_hashers)
