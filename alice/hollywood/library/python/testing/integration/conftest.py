import logging
import pytest
import os
from typing import List

from alice.apphost.fixture import AppHost
from alice.bass.fixture import Bass
from alice.hollywood.fixture import Hollywood
from alice.hollywood.library.python.testing.scenario_requester import ScenarioRequester
from alice.hollywood.library.python.testing.integration.test_functions import request_and_prepare_canondata_result
from alice.hollywood.library.python.testing.stubber.stubber_server import Stubber, StubberMode, StubberEndpoint
from alice.hollywood.library.python.testing.stubber.static_stubber import StaticStubber
from alice.library.python.testing.auth import auth
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest
from library.python.vault_client.instances import Production as VaultClient
from google.protobuf import text_format
from yatest import common as yc
from yatest.common.network import PortManager


logger = logging.getLogger(__name__)


@pytest.fixture(scope='session')
def port_manager():
    with PortManager() as pm:
        yield pm


@pytest.fixture(scope='session')
def apphost(port_manager):
    is_it2_generator = 'IT2_GENERATOR' in yc.context.flags
    port = port_manager.get_port_range(None, AppHost.port_count())
    with AppHost(port, not is_it2_generator) as service:
        yield service


def create_oauth_token_fixture(oauth_token_yav_secret):
    @pytest.fixture(scope='module')
    def oauth_token():
        # Unauthorized user, i.e. user has no oauth token
        if oauth_token_yav_secret is None:
            return None

        token = 'TEST-OAUTH-TOKEN'
        try:
            # You may get a real token only in local environment, not CI.
            token = VaultClient().get_version(oauth_token_yav_secret)['value']['secret']
        except Exception:
            logger.info(
                f'Failed to get token from Vault secret. '
                f'The default token "{token}" will be used instead.\n'
                f'It is OK if you are in CI, otherwise check whether you have access to secret:\n'
                f'> ya vault get secret "{oauth_token_yav_secret}"',
                exc_info=True
            )
            pass
        return token
    return oauth_token

oauth_token = create_oauth_token_fixture(auth.YaPlusNoMusicLikes)


@pytest.fixture(scope='function')
def test_path(request):
    return request.param


def read_run_request_prototext(filename):
    result = TScenarioRunRequest()
    with open(filename, 'r') as f:
        text_format.Merge(f.read(), result)
    return result


def read_run_request_binary(filename):
    result = TScenarioRunRequest()
    with open(filename, 'rb') as f:
        result.ParseFromString(f.read())
    return result


def read_scenario_run_requests(run_requests_path):
    basenames = sorted(set(
        filename.split('.')[0]
        for filename in os.listdir(run_requests_path)
        if filename.startswith('run_request')
    ))
    logger.info(f'Found run requests: {basenames}')

    run_requests = []
    for basename in basenames:
        filename_binary = os.path.join(run_requests_path, f'{basename}.pb')
        if os.path.exists(filename_binary):
            logger.info(f'Reading input binary run request file {filename_binary}')
            run_request = read_run_request_binary(filename_binary)
            run_requests.append(run_request)
            continue

        filename_prototext = os.path.join(run_requests_path, f'{basename}.pb.txt')
        logger.info(f'Reading input prototext run request file {filename_prototext}')
        run_request = read_run_request_prototext(filename_prototext)
        run_requests.append(run_request)
    return run_requests


def create_test_function(tests_data_path, test_params, scenario_handle, usefixtures=None, use_oauth_token_fixture=None,
                         tests_data=None, scenario_name=None):
    oauth_token_fixture = use_oauth_token_fixture if use_oauth_token_fixture is not None else 'oauth_token'
    usefixtures = list(usefixtures or [])
    usefixtures.append(oauth_token_fixture)

    @pytest.mark.usefixtures(*usefixtures)
    @pytest.mark.parametrize(
        argnames='app_preset, test_name, parametrize_index, test_path', indirect=['test_path'],
        **test_params)
    def test_run(app_preset, test_name, parametrize_index, test_path, request, apphost, hollywood):
        apphost.wait_port()
        run_requests_path = yc.source_path(os.path.join(tests_data_path, test_path))
        scenario_run_requests = read_scenario_run_requests(run_requests_path)

        srcrwr_params = request.getfixturevalue('srcrwr_params') if 'srcrwr_params' in usefixtures else {}
        srcrwr_params['HOLLYWOOD_ALL'] = f'localhost:{hollywood.grpc_port}'

        oauth_token = request.getfixturevalue(oauth_token_fixture)
        scenario_requester = ScenarioRequester(apphost, oauth_token)
        test_data = tests_data[test_name] if tests_data is not None else None

        canondata_results = TestRunner(app_preset, test_name, parametrize_index, test_data, scenario_run_requests,
                                       srcrwr_params, scenario_requester, scenario_handle, scenario_name).run()

        if len(canondata_results) == 1:
            return canondata_results[0]

        result = ''
        for index, canondata_result in enumerate(canondata_results):
            result += '##################\n'
            result += f'# Dialog phrase {index}\n'
            result += canondata_result
            result += '\n'
        return result
    return test_run


class TestRunner:
    def __init__(self, app_preset, test_name, parametrize_index, test_data, scenario_run_requests, srcrwr_params,
                 scenario_requester, scenario_handle, scenario_name):
        self._app_preset = app_preset
        self._test_name = test_name
        self._parametrize_index = parametrize_index
        self._test_data = test_data
        self._scenario_run_requests = scenario_run_requests
        self._srcrwr_params = srcrwr_params
        self._scenario_requester = scenario_requester
        self._scenario_handle = scenario_handle
        self._scenario_name = scenario_name

    def run(self):
        logger.info(f'STARTED test run ({self._app_preset}, {self._test_name}, {self._parametrize_index})')
        canondata_results = self._run_from_plain_data()
        logger.info(f'DONE with ({self._app_preset}, {self._test_name}, {self._parametrize_index})')
        return canondata_results

    def _run_from_plain_data(self):
        canondata_results = []
        for index, scenario_run_request in enumerate(self._scenario_run_requests):
            scenario_handle = self._scenario_handle
            if self._test_data is not None:
                input_obj = self._test_data['input_dialog'][index]
                scenario_handle = input_obj.scenario.handle if input_obj.scenario else scenario_handle

            canondata_result = request_and_prepare_canondata_result(
                self._scenario_requester, scenario_run_request, scenario_handle, self._srcrwr_params)
            canondata_results.append(canondata_result)
        return canondata_results


# TODO(vitvlkv): Better use header_filter_regexps=[], but this will requre generation of all stub files
def create_stubber_fixture(tests_data_path, host, port, endpoints: List[StubberEndpoint], scheme='http',
                           stubs_subdir='', hash_pattern=False, header_filter_regexps=[r'.*'],
                           stubber_mode=StubberMode.UseStubsUpdateNone):
    @pytest.fixture(scope='function')
    def stubber_fixture(test_path, port_manager):
        stubs_data_path = yc.source_path(os.path.join(tests_data_path, test_path, stubs_subdir))
        stub_port = port_manager.get_port()
        with Stubber(stubs_data_path, stub_port, host, port, endpoints, scheme, stubber_mode,
                     hash_pattern, header_filter_regexps=header_filter_regexps) as stubber:
            yield stubber
    return stubber_fixture


@pytest.fixture(scope='module')
def bass_server(port_manager):
    with Bass(port_manager.get_port()) as server:
        yield server


# TODO(vitvlkv): Better use header_filter_regexps=[], but this will requre generation of all stub files
def create_localhost_bass_stubber_fixture(tests_data_path, stubs_subdir='', header_filter_regexps=[r'.*'], custom_request_content_hasher=None):
    @pytest.fixture(scope='function')
    def stubber_fixture(test_path, port_manager, bass_server):
        host = os.getenv('TEST_HOLLYWOOD_BASS_HOST', 'localhost')
        port = int(os.getenv('TEST_HOLLYWOOD_BASS_PORT', str(bass_server.port)))
        endpoints = [
            StubberEndpoint('/vins', ['POST']),
            StubberEndpoint('/megamind/prepare', ['POST']),
            StubberEndpoint('/megamind/apply', ['POST']),
            StubberEndpoint('/megamind/protocol/video_general/run', ['POST'], request_content_hasher=custom_request_content_hasher),
            StubberEndpoint('/megamind/protocol/video_general/apply', ['POST'], request_content_hasher=custom_request_content_hasher),
        ]
        stubs_data_path = yc.source_path(os.path.join(tests_data_path, test_path, stubs_subdir))
        stubber_port = port_manager.get_port()
        with Stubber(stubs_data_path, stubber_port, host, port, endpoints, 'http', StubberMode.UseStubsUpdateNone,
                     header_filter_regexps=header_filter_regexps) as stubber:
            yield stubber
    return stubber_fixture


def create_bass_stubber_fixture(tests_data_path, stubs_subdir='', stubber_mode=StubberMode.UseStubsUpdateNone, custom_request_content_hasher=None):
    return create_stubber_fixture(
        tests_data_path,
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
        stubber_mode=stubber_mode,
    )


def create_static_stubber_fixture(path, static_data_filepath):
    @pytest.fixture(scope='function')
    def stubber_fixture(port_manager):
        static_data_filepath_abs = yc.source_path(static_data_filepath)
        stub_port = port_manager.get_port()
        with StaticStubber(stub_port, path, static_data_filepath_abs) as stubber:
            yield stubber
    return stubber_fixture


def create_hollywood_fixture(scenarios):
    @pytest.fixture(scope='module')
    def hollywood(port_manager):
        port = port_manager.get_port_range(None, Hollywood.port_count())
        with Hollywood(port, scenarios) as service:
            yield service
    return hollywood
