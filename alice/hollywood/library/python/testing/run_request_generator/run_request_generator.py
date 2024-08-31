import copy
import logging
import json
import os
import pathlib
import pytest
from enum import Enum
from yatest import common as yc
from google.protobuf import text_format

from alice.hollywood.library.python.testing.megamind_requester import request_megamind, find_session, \
    sk_req_to_str_for_logging, make_request_id_base, make_request_id
from alice.library.python.testing.megamind_request.input_dialog import AbstractInput
from alice.acceptance.modules.request_generator.lib import app_presets as app_presets_module
from alice.acceptance.modules.request_generator.lib import vins
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest
from alice.hollywood.library.python.testing.dump_collector import DumpCollector


logger = logging.getLogger(__name__)

SERVER_TIME_MS_20_JAN_2020_054431 = 1579499071620  # This is just an arbitrary point in time that is fixed in the tests
CLIENT_TIME_STR = '20200120T054430'  # This is just an arbitrary point in time that is fixed in the tests


class RunRequestFormat(Enum):
    PROTO_TEXT = 'proto_text'
    PROTO_BINARY = 'proto_binary'


# TODO(vitvlkv): remove useless scenario_handle param
def create_run_request_generator_fun(scenario_name, scenario_handle, test_params, tests_data_path, tests_data,
                                     default_experiments=None, default_device_state=None,
                                     run_request_format=RunRequestFormat.PROTO_TEXT,
                                     use_oauth_token_fixture=None, usefixtures=None,
                                     mm_force_scenario=True, lang='ru-RU',
                                     default_supported_features=None, default_additional_options=None):
    default_experiments = list(default_experiments or [])

    oauth_token_fixture = use_oauth_token_fixture if use_oauth_token_fixture is not None else 'oauth_token'

    usefixtures = list(usefixtures or [])
    usefixtures.append(oauth_token_fixture)

    @pytest.mark.usefixtures(*usefixtures)
    @pytest.mark.parametrize(
        argnames='app_preset, test_name, parametrize_index, test_path', indirect=['test_path'],
        argvalues=test_params)
    def test_run_request_generator(app_preset, test_name, parametrize_index, test_path, servers, request):
        srcrwr_params = request.getfixturevalue('srcrwr_params') if 'srcrwr_params' in usefixtures else {}

        RunRequestGenerator(tests_data_path, tests_data[test_name], app_preset, test_name, parametrize_index,
                            servers,
                            oauth_token=request.getfixturevalue(oauth_token_fixture),
                            run_request_format=run_request_format,
                            default_scenario_name=scenario_name,
                            default_scenario_handle=scenario_handle,
                            default_experiments=default_experiments,
                            default_device_state=default_device_state,
                            mm_force_scenario=mm_force_scenario,
                            srcrwr_params=srcrwr_params,
                            lang=lang,
                            default_supported_features=default_supported_features,
                            default_additional_options=default_additional_options).run()

    return test_run_request_generator


class RunRequestGenerator:
    """
    Generates run_requests and sources' stubs for hollywood tests.
    """
    def __init__(self, tests_data_path, test_data, app_preset, test_name, parametrize_index,
                 servers, oauth_token,
                 run_request_format, default_scenario_name, default_scenario_handle, default_experiments, default_device_state,
                 mm_force_scenario, srcrwr_params, lang, default_supported_features=None,
                 default_additional_options=None, generator_params=[]):
        self._tests_data_path = yc.source_path(tests_data_path)
        self._test_data = test_data
        self._app_preset = app_preset
        self._test_name = test_name
        self._parametrize_index = parametrize_index  # Could be None
        self._servers = servers
        self._oauth_token = oauth_token
        self._run_request_format = run_request_format
        self._scenario_name = self._test_data.get('scenario_name', default_scenario_name)
        self._scenario_handle = self._test_data.get('scenario_handle', default_scenario_handle)
        self._default_experiments = default_experiments
        self._default_device_state = default_device_state
        self._mm_force_scenario = mm_force_scenario
        self._srcrwr_params = srcrwr_params
        self._lang = lang
        self._default_supported_features = default_supported_features or []
        self._default_additional_options = default_additional_options or {}
        self._dump_collector = DumpCollector(servers.apphost)
        self._generator_params = generator_params

    def run(self):
        run_request_generator_options = self._test_data.get('run_request_generator', {})
        if run_request_generator_options.get('skip', False):
            logger.info(f'SKIPPED run request generation for test ({self._app_preset}, {self._test_name})')
            return

        logger.info(f'STARTED run request generation for test ({self._app_preset}, {self._test_name})')
        self._request_id_base = make_request_id_base(self._app_preset, self._test_name)

        self._run_from_plain_data()
        logger.info(f'DONE with ({self._app_preset}, {self._test_name})')

    def _run_from_plain_data(self):
        session = None
        input_objs = self._test_data.get('input_dialog', [])
        for index, input_obj in enumerate(input_objs):
            request_id = make_request_id(self._request_id_base, index)
            run_request_file_basename = _make_run_request_file_basename(index, len(input_objs), single_run_request_without_suffix=True)

            if not isinstance(input_obj, AbstractInput):
                raise Exception(f'bad element in input_dialog of type={type(input_obj)}, expected an AbstractInput child instance')
            event = input_obj.make_event()
            personal_data = input_obj.make_personal_data()
            scenario_name = input_obj.scenario.name if input_obj.scenario else self._scenario_name

            sk_resp_str = self.request_megamind_and_dump_run_request(request_id, None, run_request_file_basename,
                                                                     session, scenario_name,
                                                                     event, input_obj.device_state,
                                                                     personal_data, input_obj.request_patcher, input_obj.memento)
            sk_resp = json.loads(sk_resp_str)
            session = find_session(sk_resp)

    def request_megamind_and_dump_run_request(self, request_id, input_text,
                                              run_request_file_basename, session,
                                              scenario_name,
                                              event=None, device_state=None, personal_data=None, request_patcher=None, memento=None):
        if input_text is None and event is None:
            raise Exception('input_text and event are None')
        uuid = 'deadbeef-dead-beef-1234-deadbeef1234'

        app_info = _get_app_info(self._app_preset)
        experiments = self._make_experiments(scenario_name)
        additional_options = self._make_additional_options()

        device_state = device_state if device_state is not None else self._default_device_state

        sk_req = vins.make_vins_request(
            request_id,
            text=input_text,
            event=event,
            lang=self._lang,
            app=app_info,
            experiments=experiments,
            oauth_token=self._oauth_token,
            uuid=uuid,
            additional_options=additional_options,
            client_time=CLIENT_TIME_STR,
            session=session,
            device_state=device_state,
            personal_data=personal_data,
            memento=memento,
        )
        logger.info(f'Generated speech kit request is: {sk_req_to_str_for_logging(sk_req)}')

        sk_resp_strs = request_megamind(self._servers, sk_req, self._srcrwr_params, self._generator_params)
        dumped_requests = self._dump_collector.extract_requests_from_eventlog(scenario_name, request_id)
        assert 'run' in dumped_requests.keys(), 'Maybe DumpCollector could not find any data?.. ' \
            f'Keys are {dumped_requests.keys()}'

        run_request = text_format.MessageToString(dumped_requests['run'], as_utf8=True)

        if request_patcher is not None:
            logger.info(f'Generated run request before the request patcher: {run_request}')
            run_request = request_patcher(run_request)
        logger.info(f'Generated run request is: {run_request}')

        test_name = _make_full_test_name(self._test_name, self._parametrize_index)
        if self._run_request_format == RunRequestFormat.PROTO_TEXT:
            _write_output_protobuf_text(self._tests_data_path, os.path.join(self._app_preset, test_name), run_request, run_request_file_basename)
        elif self._run_request_format == RunRequestFormat.PROTO_BINARY:
            _write_output_protobuf_binary(self._tests_data_path, self._app_preset, test_name, run_request, run_request_file_basename)
        return sk_resp_strs[-1]

    def _make_experiments(self, scenario_name):
        experiments = {
            f'mm_enable_protocol_scenario={scenario_name}': '1',
        }

        if 'test_case_provider' in self._test_data:
            provider = self._test_data['test_case_provider']
            for exp in provider.experiments:
                experiments[exp] = '1'

        if self._mm_force_scenario:
            experiments[f'mm_scenario={scenario_name}'] = '1'
        for exp in self._default_experiments:
            experiments[exp] = '1'
        for exp in self._test_data.get('experiments', []):
            experiments[exp] = '1'
        for exp in self._test_data.get('ignore_experiments', []):
            del experiments[exp]
        return experiments

    def _make_supported_features(self):
        sf = copy.copy(self._default_supported_features)
        if (self._app_preset in ['quasar', 'aliced']):
            sf.append('music_player_allow_shots')

        for feature in self._test_data.get('supported_features', []):
            sf.append(feature)

        preset_features = app_presets_module.get_preset_attr(self._app_preset, 'supported_features')
        if preset_features:
            sf.extend(preset_features)

        return sf

    def _make_additional_options(self):
        additional_options = {'supported_features': self._make_supported_features()}
        additional_options['server_time_ms'] = SERVER_TIME_MS_20_JAN_2020_054431

        additional_options.update(self._default_additional_options)
        if 'additional_options' in self._test_data:
            additional_options.update(self._test_data['additional_options'])
        return additional_options


def _write_output_protobuf_text(tests_data_abs_path, test_path, run_request, run_request_file_basename):
    run_request_out_file = os.path.join(tests_data_abs_path, test_path, f'{run_request_file_basename}.pb.txt')
    logger.info(f'Writing run request to a file {run_request_out_file}')
    pathlib.Path(run_request_out_file).parents[0].mkdir(parents=True, exist_ok=True)
    with open(run_request_out_file, 'w') as f:
        f.write(run_request)


def _write_output_protobuf_binary(tests_data_abs_path, app_preset, test_name, run_request, run_request_file_basename):
    run_request_proto = TScenarioRunRequest()
    text_format.Merge(run_request, run_request_proto)

    run_request_binary = run_request_proto.SerializeToString()
    run_request_binary_out_file = os.path.join(tests_data_abs_path, app_preset, test_name, f'{run_request_file_basename}.pb')
    logger.info(f'Writing run request to a binary file {run_request_binary_out_file}')
    pathlib.Path(run_request_binary_out_file).parents[0].mkdir(parents=True, exist_ok=True)
    with open(run_request_binary_out_file, 'wb') as f:
        f.write(run_request_binary)

    run_request_proto.DataSources.clear()
    run_request_prototext_without_datasources = text_format.MessageToString(run_request_proto, as_utf8=True)
    run_request_prototext_out_file = os.path.join(tests_data_abs_path, app_preset, test_name, f'{run_request_file_basename}.pb.txt')
    logger.info(f'Writing run request to a prototext file (with stripped DataSources) {run_request_prototext_out_file}')
    pathlib.Path(run_request_prototext_out_file).parents[0].mkdir(parents=True, exist_ok=True)
    with open(run_request_prototext_out_file, 'w') as f:
        f.write('# ATTENTION: This file contains a TScenarioRunRequest with stripped DataSources. Tests use another\n')
        f.write('# file in binary format run_request.pb. This file is provided for human convenience only (to ease \n')
        f.write('# the review process).\n')
        f.write(run_request_prototext_without_datasources)


def _make_app_preset_test_case_tuples(tests_data, default_app_presets_list):
    result = []
    for key, val in tests_data.items():
        test_app_presets = val.get('app_presets', {})
        test_app_presets_only = test_app_presets.get('only', [])
        if len(test_app_presets_only) > 0:
            app_presets = test_app_presets_only.copy()
        else:
            app_presets = default_app_presets_list.copy()
        for ap in test_app_presets.get('ignore', []):
            if ap in app_presets:
                app_presets.remove(ap)
        for ap in app_presets:
            result.append((ap, key, None))

    return result


def make_generator_params(tests_data, default_app_presets_list):
    tuple_ = _make_app_preset_test_case_tuples(tests_data, default_app_presets_list)
    return [(app_preset, test_name, parametrize_index, _make_test_path(app_preset, test_name, parametrize_index))
            for app_preset, test_name, parametrize_index in tuple_]


def make_runner_params(tests_data, default_app_presets_list):
    return {
        # argvalues and ids are part of pytest API, see more details https://docs.pytest.org/en/stable/reference.html#pytest-mark-parametrize
        'argvalues': _make_runner_test_argvalues(tests_data, default_app_presets_list),
        'ids': _make_runner_test_ids(tests_data, default_app_presets_list),
    }


def _make_runner_test_argvalues(tests_data, default_app_presets_list):
    tuple_ = _make_app_preset_test_case_tuples(tests_data, default_app_presets_list)
    return [(app_preset, test_name, parametrize_index, _make_test_path(app_preset, test_name, parametrize_index))
            for app_preset, test_name, parametrize_index in tuple_]


def _make_runner_test_ids(tests_data, default_app_presets_list):
    tuple_ = _make_app_preset_test_case_tuples(tests_data, default_app_presets_list)
    ids = []
    for app_preset, test_name, parametrize_index in tuple_:
        parametrize_index_str = f'_{parametrize_index}' if parametrize_index is not None else ''
        ids.append(f'{app_preset}/{test_name}{parametrize_index_str}')
    return ids


def _make_full_test_name(test_name, parametrize_index):
    return test_name if parametrize_index is None else f'{test_name}_{parametrize_index}'


def _make_test_path(app_preset, test_name, parametrize_index):
    return os.path.join(app_preset, _make_full_test_name(test_name, parametrize_index))


def _get_app_info(app_preset_name):
    if not app_preset_name:
        return app_presets_module.DEFAULT_APP.application

    preset = app_presets_module.get_preset_attr(app_preset_name, 'application')
    if preset is None:
        raise ValueError(f'Invalid app preset name: {app_preset_name}')
    return preset


def _make_run_request_file_basename(run_request_index, run_requests_count=None, single_run_request_without_suffix=False):
    if run_requests_count == 1 and single_run_request_without_suffix:
        return 'run_request'
    else:
        index_suffix = f'{run_request_index:02d}'
        return f'run_request_{index_suffix}'
