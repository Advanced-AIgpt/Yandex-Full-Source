import logging
import os
import pytest
import yatest.common as yc

import alice.tests.library.auth as auth
import alice.hollywood.library.python.testing.it2.marks as marks
from alice.hollywood.library.python.testing.it2.marks import Mark
from alice.hollywood.library.python.testing.it2.alice_tests_generator import AliceTestsGenerator
from alice.hollywood.library.python.testing.it2.alice_tests_runner import AliceTestsRunner
from alice.tests.library.translit import translit
from alice.tests.library.service import Bass, Hollywood, Megamind


logger = logging.getLogger(__name__)


class Servers:
    def __init__(self, megamind, bass_server, scenario_runtime, apphost):
        self._megamind = megamind
        self._bass_server = bass_server
        self._scenario_runtime = scenario_runtime
        self._apphost = apphost

    def wait_ports(self):
        for s in [self._megamind, self._bass_server,
                  self._scenario_runtime, self._apphost]:
            s.wait_port()

    @property
    def megamind(self):
        return self._megamind

    @property
    def bass_server(self):
        return self._bass_server

    @property
    def scenario_runtime(self):
        return self._scenario_runtime

    @property
    def apphost(self):
        return self._apphost


# Extracts value from `--test-param key=value` cmd line arg
def get_test_param(name, default=None):
    # TODO(vitvlkv): Maybe replace --test-param with os.environ?.. Otherwise we have weird problems with extraction
    # duplicate fixtures from different test modules to single conftest.py...
    return yc.get_param(name, default)


# Otherwise it is runner mode
def is_generator_mode():
    return 'IT2_GENERATOR' in yc.context.flags


def choose_shard():
    # similar build variable is not propagated into  yc.context.flags so use env var
    if os.environ.get('HOLLYWOOD_SHARD'):
        return os.environ.get('HOLLYWOOD_SHARD')
    return 'all'


def pytest_collection_modifyitems(items):
    if is_generator_mode():
        return
    for item in items:
        marker = marks.get_closest_marker(item, [Mark.no_oauth, Mark.oauth])
        if marker and marker.name == Mark.oauth:
            item.add_marker(pytest.mark.oauth(auth.FAKE_FOR_CI_TEST), append=False)


@pytest.fixture(scope='module')
def megamind(port_manager):
    dummy_mode = not is_generator_mode()
    port = port_manager.get_port_range(None, Megamind.port_count())
    with Megamind(port, dummy=dummy_mode, wait_port=False) as server:
        yield server


@pytest.fixture(scope='module')
def scenario_runtime(port_manager, enabled_scenarios):
    logger.info(f'enabled_scenarios={enabled_scenarios}')

    shard = choose_shard()
    port = port_manager.get_port_range(None, Hollywood.port_count())
    logger.info(f'shard={shard}')
    with Hollywood(port,
                   scenarios=enabled_scenarios,
                   shard=shard,
                   wait_port=False) as service:
        yield service


@pytest.fixture(scope='module')
def bass_server(port_manager):
    dummy_mode = not is_generator_mode() or not os.environ.get('IT2_BASS_INCLUDED')
    with Bass(port_manager.get_port(), dummy=dummy_mode, wait_port=False) as server:
        yield server


@pytest.fixture(scope='module')
def servers(megamind, bass_server, scenario_runtime, apphost):
    s = Servers(megamind, bass_server, scenario_runtime, apphost)
    s.wait_ports()
    return s


@pytest.fixture(scope='function')
def test_path(request):
    maybe_test_class_prefix = f'{request.cls.__name__}.' if request.cls else ''
    result = f'{maybe_test_class_prefix}{translit(request.node.name)}'
    logger.debug(f'test_path={result}')
    return result


@pytest.fixture(scope='function')
def tests_data_path(request, servers):
    module_path = os.path.abspath(request.module.__file__)
    logger.info(f'module_path={module_path}')
    # e.g. /home/vitvlkv/.ya/build/build_root/x5h4/0001bf/alice/hollywood/library/scenarios/music/it2/test-results/py3test/alice/hollywood/library/scenarios/music/it2/tests.py
    start = module_path.rfind(f'alice/{servers.scenario_runtime.name.lower()}')
    end = module_path.find('.py')
    # result must be e.g. alice/hollywood/library/scenarios/music/it2/tests
    result = module_path[start:end]
    logger.info(f'tests_data_path={result}')
    return result


@pytest.fixture(scope='function')  # This fixture is usually overriden in scenario's tests code
def srcrwr_params():
    return {}


@pytest.fixture(scope='function')
def generator_params():
    return []


@pytest.fixture(scope='function')
def experiments(request):
    return marks.get_experiments(request)


def pytest_configure(config):
    for mark_name in Mark.names:
        config.addinivalue_line('markers', f'{mark_name}: ...')


@pytest.fixture(scope='function')
def alice(request, surface, servers, srcrwr_params, experiments,
          tests_data_path, test_path, generator_params):
    srcrwr_params['HOLLYWOOD_ALL'] = f'localhost:{servers.scenario_runtime.grpc_port}'
    scenario_name, scenario_handle = marks.get_scenario(request)
    logger.info(f'@pytest.mark.scenario name={scenario_name}, handle={scenario_handle}')

    freeze_stubs = marks.get_freeze_stubs(request)
    logger.info(f'@pytest.mark.freeze_stubs data is {freeze_stubs}')
    for stubber_name, path_to_response_stubs in freeze_stubs.items():
        stubber_obj = request.getfixturevalue(stubber_name)
        assert stubber_obj, f'Stubber {stubber_name} not found, please check your it2 tests conftest and/or test @pytest.mark.freeze_stubs mark.'
        for path, response_stubs in path_to_response_stubs.items():
            if not isinstance(response_stubs, list):
                response_stubs = [response_stubs]
            stubber_obj.freeze_stubs(path, response_stubs)

    oauth_token = marks.get_oauth_token(request)
    is_voice = marks.is_voice_input(request)
    evo_like = request.node.get_closest_marker(Mark.evo)
    if is_generator_mode():
        logger.info(f'Experiments are {experiments}')

        device_state = marks.get_device_state(request)
        logger.info(f'@pytest.mark.device_state is {device_state}')

        supported_features = marks.get_supported_features(request)
        logger.info(f'@pytest.mark.supported_features are {supported_features}')

        # BEWARE:  Some features can be switched off with 'unsupported_features', but some others - can not!
        # See https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/client/client_features.cpp?rev=r7755585#L7
        unsupported_features = marks.get_unsupported_features(request)
        logger.info(f'@pytest.mark.unsupported_features are {unsupported_features}')

        additional_options = marks.get_additional_options(request)
        logger.info(f'@pytest.mark.additional_options are {additional_options}')

        application = marks.get_application(request)
        iot_user_info = marks.get_iot_user_info(request)
        memento = marks.get_memento(request)
        contacts = marks.get_contacts(request)
        environment_state = marks.get_environment_state(request)
        notification_state = marks.get_notification_state(request)
        region = marks.get_region(request)
        scenario_state = marks.get_scenario_state(request)

        return AliceTestsGenerator(servers, srcrwr_params, scenario_name, scenario_handle,
                                   tests_data_path, test_path, oauth_token, surface,
                                   application, experiments, device_state, supported_features, unsupported_features,
                                   additional_options, iot_user_info, memento, contacts, environment_state,
                                   notification_state, region, generator_params, evo_like, is_voice, scenario_state)
    else:
        return AliceTestsRunner(servers, scenario_handle, srcrwr_params, scenario_name,
                                tests_data_path, test_path, oauth_token, evo_like, is_voice)
