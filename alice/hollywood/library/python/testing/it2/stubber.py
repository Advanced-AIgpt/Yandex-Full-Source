import os
import pytest
from typing import List

import yatest.common as yc

import alice.hollywood.library.python.testing.stubber.stubber_server as stubber_server
from alice.hollywood.library.python.testing.it2.conftest import is_generator_mode
from alice.hollywood.library.python.testing.stubber.stubber_server import Stubber, StubberMode
from alice.hollywood.library.python.testing.stubber.static_stubber import StaticStubber

StubberEndpoint = stubber_server.StubberEndpoint
HttpResponseStub = stubber_server.HttpResponseStub

STUBBER_MODE_FOR_GENERATOR = StubberMode.UseUpstreamUpdateAll


# This function uses FIXTURE tests_data_path, instead of parameter tests_data_path
def create_stubber_fixture(host, port, endpoints: List[StubberEndpoint] = [], scheme='http',
                           stubs_subdir='', hash_pattern=False,
                           header_filter_regexps=None, type_to_proto={},
                           source_name_filter=set(), pseudo_grpc=False):
    @pytest.fixture(scope='function')
    def stubber_fixture(tests_data_path, test_path, port_manager):
        stubs_data_path = yc.source_path(os.path.join(tests_data_path, test_path, stubs_subdir))
        stub_port = port_manager.get_port()
        stubber_mode = STUBBER_MODE_FOR_GENERATOR if is_generator_mode() else StubberMode.UseStubsUpdateNone
        with Stubber(stubs_data_path, stub_port, host, port, endpoints, scheme, stubber_mode,
                     hash_pattern, header_filter_regexps=header_filter_regexps, type_to_proto=type_to_proto,
                     source_name_filter=source_name_filter, pseudo_grpc=pseudo_grpc) as stubber:
            yield stubber
    return stubber_fixture


def create_bass_stubber_fixture(stubs_subdir='bass', custom_request_content_hasher=None):
    return create_stubber_fixture(
        os.getenv('TEST_HOLLYWOOD_BASS_HOST', 'bass.hamster.alice.yandex.net'),
        int(os.getenv('TEST_HOLLYWOOD_BASS_PORT', '80')),
        [
            StubberEndpoint('/vins', ['POST']),
            StubberEndpoint('/megamind/prepare', ['POST']),
            StubberEndpoint('/megamind/apply', ['POST']),
            StubberEndpoint('/megamind/protocol/video_general/run', ['POST'], request_content_hasher=custom_request_content_hasher),
            StubberEndpoint('/megamind/protocol/video_general/apply', ['POST'], request_content_hasher=custom_request_content_hasher),
        ],
        stubs_subdir=stubs_subdir,
    )


def create_localhost_bass_stubber_fixture(stubs_subdir='bass', custom_request_content_hasher=None):
    @pytest.fixture(scope='function')
    def stubber_fixture(tests_data_path, test_path, port_manager, servers):
        host = 'localhost'
        port = servers.bass_server.port
        endpoints = [
            StubberEndpoint('/vins', ['POST'], cgi_filter_regexps=['srcrwr']),
            StubberEndpoint('/megamind/prepare', ['POST'], cgi_filter_regexps=['srcrwr']),
            StubberEndpoint('/megamind/apply', ['POST'], cgi_filter_regexps=['srcrwr']),
            StubberEndpoint('/megamind/protocol/video_general/run', ['POST'], request_content_hasher=custom_request_content_hasher),
            StubberEndpoint('/megamind/protocol/video_general/apply', ['POST'], request_content_hasher=custom_request_content_hasher),
        ]
        stubs_data_path = yc.source_path(os.path.join(tests_data_path, test_path, stubs_subdir))
        stubber_port = port_manager.get_port()
        stubber_mode = STUBBER_MODE_FOR_GENERATOR if is_generator_mode() else StubberMode.UseStubsUpdateNone
        with Stubber(stubs_data_path, stubber_port, host, port, endpoints, 'http', stubber_mode) as stubber:
            yield stubber
    return stubber_fixture


def create_static_stubber_fixture(path, static_data_filepath):
    @pytest.fixture(scope='function')
    def stubber_fixture(port_manager):
        static_data_filepath_abs = yc.source_path(static_data_filepath)
        stub_port = port_manager.get_port()
        with StaticStubber(stub_port, path, static_data_filepath_abs) as stubber:
            yield stubber
    return stubber_fixture
