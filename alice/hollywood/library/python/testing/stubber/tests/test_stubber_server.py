# -*- coding: utf-8 -*-
import json
import logging
import os
import pytest
import requests
import shutil
import stat

import requests_mock

from yatest import common as yc

from alice.hollywood.library.python.testing.stubber.stubber_server import StubberContextManager, StubberMode, \
    HttpResponseStub
from alice.hollywood.library.python.testing.stubber.stubber_config_pb2 import TStubberConfig

logger = logging.getLogger(__name__)

TESTS_HOME = 'alice/hollywood/library/python/testing/stubber/tests/'


def copy_with_write_permissions(src, dst):
    shutil.copytree(src, dst)
    for root, dirs, files in os.walk(dst):
        os.chmod(root, os.stat(root).st_mode | stat.S_IWRITE)
        for file_name in files:
            file_path = f'{root}/{file_name}'
            os.chmod(file_path, os.stat(file_path).st_mode | stat.S_IWRITE)


@pytest.fixture(scope="function")
def data_get_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_get'), f'{tmp_path}/data_get')
    return f'{tmp_path}/data_get'


@pytest.fixture(scope="function")
def data_post_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_post'), f'{tmp_path}/data_post')
    return f'{tmp_path}/data_post'


@pytest.fixture(scope="function")
def data_put_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_put'), f'{tmp_path}/data_put')
    return f'{tmp_path}/data_put'


@pytest.fixture(scope="function")
def data_empty_path(tmp_path):
    return str(tmp_path)


@pytest.fixture(scope="function")
def data_duplicates_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_duplicates'), f'{tmp_path}/data_duplicates')
    return f'{tmp_path}/data_duplicates'


@pytest.fixture(scope="function")
def data_pattern_hash_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_pattern_hash'), f'{tmp_path}/data_pattern_hash')
    return f'{tmp_path}/data_pattern_hash'


@pytest.fixture(scope="function")
def data_request_hash_path(tmp_path):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_request_hash'), f'{tmp_path}/data_request_hash')
    return f'{tmp_path}/data_request_hash'


@pytest.fixture(scope="function")
def data_unused_stubs_path(tmp_path):
    data_dir = 'data_unused_stubs'
    source_path = yc.source_path(os.path.join(TESTS_HOME, data_dir))
    target_path = os.path.join(tmp_path, data_dir)
    copy_with_write_permissions(source_path, target_path)
    return target_path


@pytest.fixture(scope="function")
def config_get(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search'
    config.Upstreams[path].Methods.append('GET')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    return config


@pytest.fixture(scope="function")
def config_post(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search'
    config.Upstreams[path].Methods.append('POST')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    return config


@pytest.fixture(scope="function")
def config_put(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search'
    config.Upstreams[path].Methods.append('PUT')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    return config


@pytest.fixture(scope="function")
def config_two_routes(port):
    config = TStubberConfig()
    config.Port = port
    pathOne = '/route_one'
    config.Upstreams[pathOne].Methods.append('GET')
    config.Upstreams[pathOne].Host = 'example.yandex.net'
    config.Upstreams[pathOne].Port = 80
    config.Upstreams[pathOne].Scheme = 'http'
    pathTwo = '/route_two'
    config.Upstreams[pathTwo].Methods.append('GET')
    config.Upstreams[pathTwo].Host = 'example.yandex.net'
    config.Upstreams[pathTwo].Port = 80
    config.Upstreams[pathTwo].Scheme = 'http'
    return config


@pytest.fixture(scope="function")
def config_one_route_two_methods(port):
    config = TStubberConfig()
    config.Port = port
    pathOne = '/search'
    config.Upstreams[pathOne].Methods.extend(['GET', 'POST'])
    config.Upstreams[pathOne].Host = 'example.yandex.net'
    config.Upstreams[pathOne].Port = 80
    config.Upstreams[pathOne].Scheme = 'http'
    return config


@pytest.fixture(scope="function")
def config_get_nonidempotent(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search'
    config.Upstreams[path].Methods.append('GET')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    config.Upstreams[path].NonIdempotent = True
    return config


@pytest.fixture(scope="function")
def config_get_pattern_hash(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search/{id}'
    config.Upstreams[path].Methods.append('GET')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    config.Upstreams[path].HashPatternPath = True
    return config


@pytest.fixture(scope="function")
def config_get_request_hash(port):
    config = TStubberConfig()
    config.Port = port
    path = '/search/{id}'
    config.Upstreams[path].Methods.append('GET')
    config.Upstreams[path].Host = 'example.yandex.net'
    config.Upstreams[path].Port = 80
    config.Upstreams[path].Scheme = 'http'
    config.Upstreams[path].HashPatternPath = False
    return config


@pytest.fixture(scope="function")
def stubber_with_data_get(data_get_path, config_get):
    with StubberContextManager(data_get_path, config_get) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_get_use_upstream_update_all(data_get_path, config_get):
    config_get.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_get_path, config_get) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_post(data_post_path, config_post):
    with StubberContextManager(data_post_path, config_post) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_post_json(tmp_path, config_post):
    data_dir = 'data_post_json'
    tmp_data_dir = os.path.join(tmp_path, data_dir)
    copy_with_write_permissions(yc.source_path(os.path.join(TESTS_HOME, data_dir)), tmp_data_dir)
    with StubberContextManager(tmp_data_dir, config_post) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_post_json_not_sorted(tmp_path, config_post):
    data_dir = 'data_post_json_not_sorted'
    tmp_data_dir = os.path.join(tmp_path, data_dir)
    copy_with_write_permissions(yc.source_path(os.path.join(TESTS_HOME, data_dir)), tmp_data_dir)
    with StubberContextManager(tmp_data_dir, config_post) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_put(data_put_path, config_put):
    with StubberContextManager(data_put_path, config_put) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_get_expect_errors(data_empty_path, config_get):
    with StubberContextManager(data_empty_path, config_get, expect_errors=True) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_get_update_all(data_empty_path, config_get):
    config_get.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_get) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_post_update_all(data_empty_path, config_post):
    config_post.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_post) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_put_update_all(data_empty_path, config_put):
    config_put.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_put) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_two_routes_update_all(data_empty_path, config_two_routes):
    config_two_routes.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_two_routes) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_two_methods_update_all(data_empty_path, config_one_route_two_methods):
    config_one_route_two_methods.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_one_route_two_methods) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_nonidempotent(data_empty_path, config_get_nonidempotent):
    config_get_nonidempotent.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_get_nonidempotent) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_nonidempotent(data_duplicates_path, config_get_nonidempotent):
    config_get_nonidempotent.StubberMode = StubberMode.UseStubsUpdateNone.value
    with StubberContextManager(data_duplicates_path, config_get_nonidempotent) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_pattern_hash(data_empty_path, config_get_pattern_hash):
    config_get_pattern_hash.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_get_pattern_hash) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_pattern_hash(data_pattern_hash_path, config_get_pattern_hash):
    config_get_pattern_hash.StubberMode = StubberMode.UseStubsUpdateNone.value
    with StubberContextManager(data_pattern_hash_path, config_get_pattern_hash) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_without_data_request_hash(data_empty_path, config_get_request_hash):
    config_get_request_hash.StubberMode = StubberMode.UseUpstreamUpdateAll.value
    with StubberContextManager(data_empty_path, config_get_request_hash) as thread:
        yield thread


@pytest.fixture(scope="function")
def stubber_with_data_request_hash(data_request_hash_path, config_get_request_hash):
    config_get_request_hash.StubberMode = StubberMode.UseStubsUpdateNone.value
    with StubberContextManager(data_request_hash_path, config_get_request_hash) as thread:
        yield thread


def read_stub_content(stub_folder):
    stubs = []
    for file_name in os.listdir(stub_folder):
        if not file_name.endswith('info'):
            with open(os.path.join(stub_folder, file_name), 'rb') as f:
                stubs.append((file_name, f.read()))
    stubs.sort(key=lambda stub: stub[0])
    return stubs


def test_no_stubs_method_get(stubber_without_data_get_expect_errors):
    port = stubber_without_data_get_expect_errors.port
    data_empty_path = stubber_without_data_get_expect_errors.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.get(stubber_uri, real_http=True)
        resp = requests.get(stubber_uri)

    assert resp.status_code == 500
    assert len(os.listdir(data_empty_path)) == 0


def test_update_missing_stubs_method_get(stubber_without_data_get_update_all):
    port = stubber_without_data_get_update_all.port
    data_empty_path = stubber_without_data_get_update_all.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.get(stubber_uri, real_http=True)
        resp = requests.get(stubber_uri)

    assert resp.status_code == 200
    assert len(resp.content) > 0
    assert len(os.listdir(data_empty_path)) == 2


def test_update_missing_stubs_method_post(stubber_without_data_post_update_all):
    port = stubber_without_data_post_update_all.port
    data_empty_path = stubber_without_data_post_update_all.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.post('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.post(stubber_uri, real_http=True)
        resp = requests.post(stubber_uri, data='some data')

    assert resp.status_code == 200
    assert len(resp.content) > 0
    files = os.listdir(data_empty_path)
    assert len(files) == 2

    # check auxiliary info fields
    info_file = next(f for f in files if f.endswith('.info'))
    info_content = json.loads(open(data_empty_path + '/' + info_file, 'r').read())
    req_info = info_content['request']
    assert info_content['status_code'] == 200
    assert req_info['path'] == '/search'
    assert req_info['query_string'] == 'text=foobar'
    assert req_info['content'] == 'some data'


def test_update_missing_stubs_method_put(stubber_without_data_put_update_all):
    assert stubber_without_data_put_update_all._stubber_server_thread._stubber_server._config.StubberMode == StubberMode.UseUpstreamUpdateAll.value
    port = stubber_without_data_put_update_all.port
    data_empty_path = stubber_without_data_put_update_all.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.put('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.put(stubber_uri, real_http=True)
        resp = requests.put(stubber_uri, data='some data')

    assert resp.status_code == 200
    assert len(resp.content) > 0
    assert len(os.listdir(data_empty_path)) == 2


@pytest.mark.parametrize('stubber_mode', [
    StubberMode.UseUpstreamUpdateAll,
    StubberMode.UseStubsUpdateNone,
])
def test_stubber_with_freeze_stubs(data_empty_path, config_get, stubber_mode):
    config_get.StubberMode = stubber_mode.value
    with StubberContextManager(data_empty_path, config_get) as stubber:
        port = stubber.port
        stubber.freeze_stubs('/search', [
            HttpResponseStub(202, content='FREEZED STUB CONTENT', headers={'x-freezed-header': '42'}),
        ])
        resp = requests.get(f'http://localhost:{port}/search?text=foobar')
        assert resp.status_code == 202
        assert resp.content == b'FREEZED STUB CONTENT'
        assert resp.headers['x-freezed-header'] == '42'


@pytest.mark.parametrize('stubber_mode', [
    StubberMode.UseUpstreamUpdateAll,
    StubberMode.UseStubsUpdateNone,
])
def test_stubber_with_freeze_stubs_two_responses(data_empty_path, config_get, stubber_mode):
    config_get.StubberMode = stubber_mode.value
    with StubberContextManager(data_empty_path, config_get) as stubber:
        port = stubber.port
        stubber.freeze_stubs('/search', [
            HttpResponseStub(202, content='FREEZED STUB CONTENT 1', headers={'x-freezed-header-1': '1'}),
            HttpResponseStub(203, content='FREEZED STUB CONTENT 2', headers={'x-freezed-header-2': '2'}),
        ])
        resp = requests.get(f'http://localhost:{port}/search?text=foobar')
        assert resp.status_code == 202
        assert resp.content == b'FREEZED STUB CONTENT 1'
        assert resp.headers['x-freezed-header-1'] == '1'

        resp = requests.get(f'http://localhost:{port}/search?text=foobar')
        assert resp.status_code == 203
        assert resp.content == b'FREEZED STUB CONTENT 2'
        assert resp.headers['x-freezed-header-2'] == '2'

        # Two responses are returned in a cycle
        resp = requests.get(f'http://localhost:{port}/search?text=foobar')
        assert resp.status_code == 202
        assert resp.content == b'FREEZED STUB CONTENT 1'
        assert resp.headers['x-freezed-header-1'] == '1'


@pytest.mark.parametrize('stubber_mode', [
    StubberMode.UseUpstreamUpdateAll,
    StubberMode.UseStubsUpdateNone,
])
def test_stubber_with_freeze_stubs_content_from_file(data_empty_path, config_get, stubber_mode):
    config_get.StubberMode = stubber_mode.value
    with StubberContextManager(data_empty_path, config_get) as stubber:
        port = stubber.port
        content_filename = yc.source_path(f'{TESTS_HOME}/freeze_stub/freeze_stub_content')
        stubber.freeze_stubs('/search', [
            HttpResponseStub(202, content_filename=content_filename, headers={'x-freezed-header': '42'}),
        ])
        resp = requests.get(f'http://localhost:{port}/search?text=foobar')
        assert resp.status_code == 202
        assert resp.content == b'FREEZE STUB CONTENT FROM FILE'
        assert resp.headers['x-freezed-header'] == '42'


def test_read_stubs_method_get(stubber_with_data_get):
    port = stubber_with_data_get.port
    data_get_path = stubber_with_data_get.stubs_data_path
    assert os.path.exists(data_get_path)
    assert len(os.listdir(data_get_path)) == 2
    resp = requests.get(f'http://localhost:{port}/search?text=foobar')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'baz'
    assert len(os.listdir(data_get_path)) == 2


def test_use_upstream_update_all_method_get(stubber_with_data_get_use_upstream_update_all):
    port = stubber_with_data_get_use_upstream_update_all.port
    data_get_path = stubber_with_data_get_use_upstream_update_all.stubs_data_path
    assert os.path.exists(data_get_path)
    assert len(os.listdir(data_get_path)) == 2
    stubs = read_stub_content(data_get_path)
    assert len(stubs) == 1
    assert stubs[0][1] == b'TEST STUB FILE CONTENT'

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')
        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.get(stubber_uri, real_http=True)
        resp = requests.get(stubber_uri)

    assert resp.status_code == 200
    assert resp.content == b'DATA FROM UPSTREAM SERVER'
    assert len(os.listdir(data_get_path)) == 2
    stubs = read_stub_content(data_get_path)
    assert len(stubs) == 1
    assert stubs[0][1] == b'DATA FROM UPSTREAM SERVER'


def test_read_stubs_method_post(stubber_with_data_post):
    port = stubber_with_data_post.port
    data_post_path = stubber_with_data_post.stubs_data_path
    assert os.path.exists(data_post_path)
    assert len(os.listdir(data_post_path)) == 2
    resp = requests.post(f'http://localhost:{port}/search?text=foobar', data='some data')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'baz'
    assert len(os.listdir(data_post_path)) == 2


def test_read_stubs_method_put(stubber_with_data_put):
    port = stubber_with_data_put.port
    data_put_path = stubber_with_data_put.stubs_data_path
    assert os.path.exists(data_put_path)
    assert len(os.listdir(data_put_path)) == 2
    resp = requests.put(f'http://localhost:{port}/search?text=foobar', data='some data')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'baz'
    assert len(os.listdir(data_put_path)) == 2


def test_stubber_serves_two_routes(stubber_two_routes_update_all):
    port = stubber_two_routes_update_all.port
    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/route_one', text='FOO DATA')
        m.get('http://example.yandex.net:80/route_two', text='BAR DATA')

        stubber_uri1 = f'http://localhost:{port}/route_one'
        m.get(stubber_uri1, real_http=True)
        resp1 = requests.get(stubber_uri1)

        stubber_uri2 = f'http://localhost:{port}/route_two'
        m.get(stubber_uri2, real_http=True)
        resp2 = requests.get(stubber_uri2)
    assert resp1.status_code == 200
    assert resp1.content == b'FOO DATA'
    assert resp2.status_code == 200
    assert resp2.content == b'BAR DATA'


def test_stubber_serves_two_methods(stubber_two_methods_update_all):
    port = stubber_two_methods_update_all.port
    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search', text='FOO DATA')
        m.post('http://example.yandex.net:80/search', text='BAR DATA')

        stubber_uri = f'http://localhost:{port}/search'
        m.get(stubber_uri, real_http=True)
        resp1 = requests.get(stubber_uri)

        m.post(stubber_uri, real_http=True)
        resp2 = requests.post(stubber_uri, data='some data')
    assert resp1.status_code == 200
    assert resp1.content == b'FOO DATA'
    assert resp2.status_code == 200
    assert resp2.content == b'BAR DATA'


def test_json_bodies_of_requests_are_sorted(stubber_with_data_post_json):
    port = stubber_with_data_post_json.port
    url = f'http://localhost:{port}/search'
    headers = {'Content-Type': 'application/json'}
    response_content = b'Response content 1'

    resp1 = requests.post(url, headers=headers, data=b'{"abc":"foo", "xyz":"bar"}')
    assert resp1.status_code == 200
    assert resp1.content == response_content

    resp2 = requests.post(url, headers=headers, data=b'{"xyz":"bar", "abc":"foo"}')
    assert resp2.status_code == 200
    assert resp2.content == response_content


def test_json_bodies_of_requests_are_not_sorted(stubber_with_data_post_json_not_sorted):
    port = stubber_with_data_post_json_not_sorted.port
    url = f'http://localhost:{port}/search'
    headers = {'Content-Type': 'text/plain'}

    resp1 = requests.post(url, headers=headers, data=b'{"abc":"foo", "xyz":"bar"}')
    assert resp1.status_code == 200
    assert resp1.content == b'Response content 2'

    resp2 = requests.post(url, headers=headers, data=b'{"xyz":"bar", "abc":"foo"}')
    assert resp2.status_code == 200
    assert resp2.content == b'Response content 3'


def test_stubber_writes_pretty_printed_and_sorted_json_stubs(stubber_without_data_get_update_all, request_headers={}):
    port = stubber_without_data_get_update_all.port
    data_empty_path = stubber_without_data_get_update_all.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search?text=foobar', text='{"foo":1, "bar":2}', headers={'Content-Type': 'application/json'})

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.get(stubber_uri, real_http=True)
        resp = requests.get(stubber_uri, headers=request_headers)

    assert resp.status_code == 200
    assert len(resp.content) > 0
    assert len(os.listdir(data_empty_path)) == 2

    stubs = read_stub_content(data_empty_path)
    assert len(stubs) == 1
    assert stubs[0][1] == b'{\n    "bar": 2,\n    "foo": 1\n}'


def test_stubber_handles_empty_request_json_content(stubber_without_data_get_update_all):
    test_stubber_writes_pretty_printed_and_sorted_json_stubs(
        stubber_without_data_get_update_all,
        request_headers={'Content-Type': 'application/json'}
    )


def test_unused_stubs_update_none_mode(data_unused_stubs_path, config_get):
    config_get.StubberMode = StubberMode.UseStubsUpdateNone.value
    with pytest.raises(Exception) as e_info:
        with StubberContextManager(data_unused_stubs_path, config_get) as stubber:
            port = stubber.port
            stubs_data_path = stubber.stubs_data_path
            assert len(os.listdir(stubs_data_path)) == 6
            resp = requests.get(f'http://localhost:{port}/search?text=foobar')
            assert resp.status_code == 200
    assert len(os.listdir(stubs_data_path)) == 6, 'No stub files should be deleted in this mode'
    e_info_str = str(e_info)
    assert 'Unused trash files found in' in e_info_str


def test_duplicate_stubs(stubber_without_data_nonidempotent):
    port = stubber_without_data_nonidempotent.port
    data_empty_path = stubber_without_data_nonidempotent.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search?text=foobar', text='DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search?text=foobar'
        m.get(stubber_uri, real_http=True)
        resp1 = requests.get(stubber_uri)
        resp2 = requests.get(stubber_uri)

    assert resp1.status_code == 200
    assert len(resp1.content) > 0
    assert resp2.status_code == 200
    assert len(resp2.content) > 0
    assert set(os.listdir(data_empty_path)) == {
        'get_search_2c533feac51e48ba234991eddd3ef44a522bbb6d_00',
        'get_search_2c533feac51e48ba234991eddd3ef44a522bbb6d_00.info',
        'get_search_2c533feac51e48ba234991eddd3ef44a522bbb6d_01',
        'get_search_2c533feac51e48ba234991eddd3ef44a522bbb6d_01.info',
    }


def test_read_duplicate_stubs(stubber_with_data_nonidempotent):
    port = stubber_with_data_nonidempotent.port
    data_get_path = stubber_with_data_nonidempotent.stubs_data_path
    assert os.path.exists(data_get_path)
    assert len(os.listdir(data_get_path)) == 4

    resp = requests.get(f'http://localhost:{port}/search?text=foobar')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'baz'

    resp = requests.get(f'http://localhost:{port}/search?text=foobar')
    assert resp.status_code == 200
    assert resp.content == b'ANOTHER TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'foo'

    assert len(os.listdir(data_get_path)) == 4


@pytest.mark.parametrize('stubber_mode, check_stub', [
    (StubberMode.UseUpstreamUpdateAll, True),
    # StubberMode.UseStubsUpdateNone is not applicable here, because it doesn't write any stubs
])
def test_process_content_before_writing_stubs(data_empty_path, config_get, stubber_mode, check_stub):
    procs = {
        '/search': [lambda headers, content: content.decode('utf8').replace('UPSTREAM', "FOOBAR").encode('utf-8')]
    }
    config_get.StubberMode = stubber_mode.value
    with StubberContextManager(data_empty_path, config_get, all_content_procs=procs) as stubber:
        port = stubber.port

        with requests_mock.mock() as m:
            m.get('http://example.yandex.net:80/search', text='DATA FROM UPSTREAM SERVER')

            stubber_uri = f'http://localhost:{port}/search'
            m.get(stubber_uri, real_http=True)
            resp = requests.get(stubber_uri)

        assert resp.status_code == 200
        assert resp.content == b'DATA FROM FOOBAR SERVER'

        if check_stub:
            stub_filename = sorted(os.listdir(data_empty_path))[0]
            assert len(os.listdir(data_empty_path)) == 2
            with open(os.path.join(data_empty_path, stub_filename), 'rb') as in_file:
                assert in_file.read() == b'DATA FROM FOOBAR SERVER'


def test_pattern_hash_stubs(stubber_without_data_pattern_hash):
    port = stubber_without_data_pattern_hash.port
    data_empty_path = stubber_without_data_pattern_hash.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search/id1', text='DATA FROM UPSTREAM SERVER')
        m.get('http://example.yandex.net:80/search/id2', text='OTHER DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search/id1'
        m.get(stubber_uri, real_http=True)
        resp1 = requests.get(stubber_uri)
        stubber_uri = f'http://localhost:{port}/search/id2'
        m.get(stubber_uri, real_http=True)
        resp2 = requests.get(stubber_uri)

    assert resp1.status_code == 200
    assert len(resp1.content) > 0
    assert resp2.status_code == 200
    assert len(resp2.content) > 0
    assert set(os.listdir(data_empty_path)) == {
        'get_search-id_92b945a925ac54c3c69ab00943d0142f381aa828',
        'get_search-id_92b945a925ac54c3c69ab00943d0142f381aa828.info',
    }


def test_read_hash_pattern_stubs(stubber_with_data_pattern_hash):
    port = stubber_with_data_pattern_hash.port
    data_get_path = stubber_with_data_pattern_hash.stubs_data_path
    assert os.path.exists(data_get_path)
    assert len(os.listdir(data_get_path)) == 2

    resp = requests.get(f'http://localhost:{port}/search/id1')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    resp_content = resp.content
    assert resp.headers['x-foo-bar'] == 'baz'

    resp = requests.get(f'http://localhost:{port}/search/id2')
    assert resp.status_code == 200
    assert resp.content == resp_content
    assert resp.headers['x-foo-bar'] == 'baz'

    assert len(os.listdir(data_get_path)) == 2


def test_request_hash_stubs(stubber_without_data_request_hash):
    port = stubber_without_data_request_hash.port
    data_empty_path = stubber_without_data_request_hash.stubs_data_path
    assert os.path.exists(data_empty_path)
    assert len(os.listdir(data_empty_path)) == 0

    with requests_mock.mock() as m:
        m.get('http://example.yandex.net:80/search/id1', text='DATA FROM UPSTREAM SERVER')
        m.get('http://example.yandex.net:80/search/id2', text='OTHER DATA FROM UPSTREAM SERVER')

        stubber_uri = f'http://localhost:{port}/search/id1'
        m.get(stubber_uri, real_http=True)
        resp1 = requests.get(stubber_uri)
        stubber_uri = f'http://localhost:{port}/search/id2'
        m.get(stubber_uri, real_http=True)
        resp2 = requests.get(stubber_uri)

    assert resp1.status_code == 200
    assert len(resp1.content) > 0
    assert resp2.status_code == 200
    assert len(resp2.content) > 0
    assert set(os.listdir(data_empty_path)) == {
        'get_search-id1_69c9a1b6cd1cedc2f63b00718c9b806d4cee0717',
        'get_search-id1_69c9a1b6cd1cedc2f63b00718c9b806d4cee0717.info',
        'get_search-id2_55a903defb740569f06259e376a822ac74016b0a',
        'get_search-id2_55a903defb740569f06259e376a822ac74016b0a.info',
    }


def test_read_hash_request_stubs(stubber_with_data_request_hash):
    port = stubber_with_data_request_hash.port
    data_get_path = stubber_with_data_request_hash.stubs_data_path
    assert os.path.exists(data_get_path)
    assert len(os.listdir(data_get_path)) == 4

    resp = requests.get(f'http://localhost:{port}/search/id1')
    assert resp.status_code == 200
    assert resp.content == b'TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'baz'

    resp = requests.get(f'http://localhost:{port}/search/id2')
    assert resp.status_code == 200
    assert resp.content == b'ANOTHER TEST STUB FILE CONTENT'
    assert resp.headers['x-foo-bar'] == 'foo'

    assert len(os.listdir(data_get_path)) == 4


def test_request_content_hasher(tmp_path, config_post):
    copy_with_write_permissions(yc.source_path(f'{TESTS_HOME}/data_post2'), f'{tmp_path}/data_post2')
    data_post_path2 = f'{tmp_path}/data_post2'

    def sort_words(headers, content):
        words = content.decode('utf-8').split()
        words.sort()
        return ' '.join(words).encode('utf-8')

    config_post.StubberMode = StubberMode.UseStubsUpdateNone.value
    with StubberContextManager(data_post_path2, config_post,
                               request_content_hashers={'/search': sort_words}) as stubber:
        stubber_uri = f'http://localhost:{stubber.port}/search'
        resp1 = requests.post(stubber_uri, data='Apple Banana Orange Pear')
        assert resp1.status_code == 200
        assert resp1.content == b'Response content one'

        resp2 = requests.post(stubber_uri, data='Banana Apple Pear Orange')
        assert resp2.status_code == 200
        assert resp2.content == b'Response content one'

        resp3 = requests.post(stubber_uri, data='Apple Banana Orange Potato')
        assert resp3.status_code == 200
        assert resp3.content == b'Response content two'
