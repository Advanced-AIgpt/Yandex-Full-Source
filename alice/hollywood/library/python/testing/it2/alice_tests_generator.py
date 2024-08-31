import datetime
import inspect
import json
import logging
import pytz
import time
from base64 import b64encode, b64decode
from pathlib import Path

import alice.tests.library.region as evo_region
import alice.tests.library.url as url
import yatest.common as yc
from alice.acceptance.modules.request_generator.lib import vins
from alice.memento.proto.api_pb2 import TRespGetAllObjects
from alice.tests.library.uniclient import AliceSettings
from alice.tests.library.uniclient.serialization import deserialize
from alice.megamind.library.python.testing.session_builder import SessionBuilder
from alice.hollywood.library.python.testing.it2.scenario_responses import ScenarioResponses
from alice.hollywood.library.python.testing.dump_collector import DumpCollector
from alice.library.python.testing.megamind_request.input_dialog import AbstractInput, Text, Voice
from alice.hollywood.library.python.testing.megamind_requester import request_megamind, find_session, \
    sk_req_to_str_for_logging, make_request_id_base, make_request_id
from alice.tests.library.vins_response import HollywoodResponse
from google.protobuf import text_format


logger = logging.getLogger(__name__)

# This is just an arbitrary point in time that is fixed in the tests
SERVER_DATETIME = datetime.datetime(year=2020, month=1, day=20, hour=5, minute=44, second=31)


def _memento_merge(memento, memento_input):
    if not memento_input:
        return memento

    memento_proto = TRespGetAllObjects()
    memento_proto.ParseFromString(b64decode(memento))

    memento_input_proto = TRespGetAllObjects()
    memento_input_proto.ParseFromString(b64decode(memento_input))

    memento_proto.MergeFrom(memento_input_proto)

    return b64encode(memento_proto.SerializeToString()).decode()


# TODO: For Search scenario support RunRequestFormat.PROTO_BINARY request file format
class AliceTestsGenerator:
    def __init__(self, servers, srcrwr_params, scenario_name, scenario_handle,
                 tests_data_path, test_path, oauth_token, surface_ctor, application, experiments,
                 initial_device_state, supported_features, unsupported_features, additional_options, iot_user_info,
                 memento, contacts, environment_state, notification_state, region,
                 generator_params, evo_like=False, is_voice=False, scenario_state=None):
        logger.info('Initializing Alice in GENERATOR mode...')
        self._servers = servers
        self._srcrwr_params = srcrwr_params
        self._scenario_name = scenario_name
        self._scenario_handle = scenario_handle
        self._test_path = Path(yc.source_path(tests_data_path), test_path)  # Basically test_path is a combination of test_name and surface

        self._request_id_base = make_request_id_base(app_preset='', test_name=test_path)
        self._alice_commands_count = 0

        if scenario_state:
            session_builder = SessionBuilder()
            session_builder.set_scenario_state(scenario_name, scenario_state)
            self._session = session_builder.build().decode()
        else:
            self._session = None

        self._settings = AliceSettings()
        self._settings.uniproxy_url = url.Uniproxy.hamster
        self._settings.experiments = experiments
        self._settings.application = application
        self._settings.application['timestamp'] = str(int(SERVER_DATETIME.timestamp()))
        self._settings.oauth_token = oauth_token
        self._settings.region = region or evo_region.Moscow
        self._settings.supported_features = supported_features
        self._settings.unsupported_features = unsupported_features
        self._settings.environment_state = environment_state

        if inspect.isfunction(surface_ctor):
            surface_ctor = surface_ctor()
        surface_ctor.keywords['device_state'] = {
            **initial_device_state,
            **surface_ctor.keywords['device_state'],
        }
        self._surface = surface_ctor(self._settings)

        self._additional_options = additional_options
        self._iot_user_info = iot_user_info
        self._memento = memento
        self._contacts = contacts
        self._notification_state = notification_state
        self._generator_params = generator_params

        self._evo_like = evo_like
        self._is_voice = is_voice

        self._dump_collector = DumpCollector(servers.apphost)
        self._remove_old_scenario_request_files()

    def __call__(self, input_obj):
        if not isinstance(input_obj, AbstractInput):
            if self._is_voice:
                input_obj = Voice(input_obj)
            elif self._evo_like:
                input_obj = Text(input_obj)
            else:
                raise Exception(f'input_dialog is type={type(input_obj)}, expected an AbstractInput child instance')

        self._alice_commands_count += 1
        request_id = make_request_id(self._request_id_base, self._alice_commands_count - 1)
        scenario_name = input_obj.scenario.name if input_obj.scenario else self._scenario_name
        sk_req = self._make_sk_req_from(input_obj, request_id, scenario_name)
        sk_resp_strs = request_megamind(
            self._servers,
            sk_req,
            self._srcrwr_params,
            self._generator_params,
        )
        mm_responses_count = len(sk_resp_strs)
        logger.info(f'Request to megamind returned {mm_responses_count} responses')
        sk_resp = json.loads(sk_resp_strs[-1])
        self._session = find_session(sk_resp)
        self._apply_directives_to_surface(sk_resp_strs[-1])

        time_start = time.time()
        dur_treshold_sec = 5
        while True:  # XXX(vitvlkv): This `while True` ugly hack is needed because we wait for APPHOSTSUPPORT-998
            dumped_requests, dumped_responses, sources_dump = \
                self._dump_collector.extract_requests_and_responses_from_eventlog(scenario_name, request_id)
            # In case of run+continue we will have mm_responses_count==1 and 2 requests and 2 responses
            # In case of run+commit we will have mm_responses_count==2 and 2 requests and 2 responses
            if len(dumped_requests) >= mm_responses_count and len(dumped_responses) >= mm_responses_count:
                break
            duration = time.time() - time_start
            if duration > dur_treshold_sec:
                raise Exception(f'Found {len(dumped_requests)} requests and {len(dumped_responses)} responses in the '
                                f'AppHost eventlog. Expected at least {mm_responses_count} responses and requests '
                                'each. Something is wrong. Ask for help the it2 development team')
            time.sleep(0.1)

        scenario_handle = input_obj.scenario.handle if input_obj.scenario else self._scenario_handle
        for scenario_stage, scenario_request in dumped_requests.items():
            alice_command_index = self._alice_commands_count - 1
            out_file_basename = f'{(alice_command_index):02d}_{scenario_handle}_{scenario_stage}'
            self._write_output_protobuf_text(scenario_request, out_file_basename)

        scenario_responses = self._make_scenario_responses(dumped_responses, sources_dump)

        logger.info(f'Hollywood response #{self._alice_commands_count} is {str(scenario_responses)}')
        return HollywoodResponse(scenario_responses, scenario_name) if self._evo_like else scenario_responses

    def skip(self, hours=0, minutes=0, seconds=0):
        self._surface.skip(hours, minutes, seconds)

    def update_current_screen(self, screen_name):
        self._surface.device_state.Video.CurrentScreen = screen_name

    def update_audio_player_current_stream_stream_id(self, stream_id):
        self._surface.device_state.AudioPlayer.CurrentlyPlaying.StreamId = stream_id

    def update_audio_player_duration_ms(self, duration_ms):
        self._surface.device_state.AudioPlayer.DurationMs = duration_ms

    def grant_permissions(self, permission_name):
        if 'permissions' not in self._additional_options:
            self._additional_options['permissions'] = []
        for permission in self._additional_options['permissions']:
            if permission['name'] == permission_name:
                permission['granted'] = True
                return
        self._additional_options['permissions'].append({'name': permission_name, 'granted': True})

    def clear_session(self):
        self._session = None

    @property
    def device_state(self):
        return self._surface.device_state.dict()

    def _make_sk_req_from(self, input_obj, request_id, scenario_name):
        event = input_obj.make_event()
        personal_data = input_obj.make_personal_data()
        assert not input_obj.device_state, 'Inputs with device_state are not allowed in these tests'
        guest_options = input_obj.make_guest_options()
        guest_data = input_obj.make_guest_data()

        sk_req = self._make_sk_req(request_id, input_text=None, session=self._session, scenario_name=scenario_name,
                                   event=event, personal_data=personal_data, guest_options=guest_options, guest_data=guest_data,
                                   memento=input_obj.memento)
        logger.info(f'Generated speech kit request is: {sk_req_to_str_for_logging(sk_req)}')
        return sk_req

    def _make_sk_req(self, request_id, input_text, session, scenario_name,
                     event=None, personal_data=None, guest_options=None, guest_data=None, memento=None):
        if input_text is None and event is None:
            raise Exception('input_text and event are None')
        uuid = 'deadbeef-dead-beef-1234-deadbeef1234'

        experiments = self._make_experiments(scenario_name)
        additional_options = self._make_additional_options()

        client_time = self._surface._alice_time
        client_time_str = client_time.astimezone(pytz.utc).strftime('%Y%m%dT%H%M%S')

        return vins.make_vins_request(
            request_id,
            text=input_text,
            event=event,
            lang=self._surface.application.Lang,
            app=self._surface.preset.application,
            experiments=experiments,
            oauth_token=self._settings.oauth_token,
            uuid=uuid,
            additional_options=additional_options,
            client_time=client_time_str,  # This is local time
            session=session,
            device_state=self.device_state,
            personal_data=personal_data,
            iot_user_info=self._iot_user_info,
            memento=_memento_merge(self._memento, memento),
            contacts=self._contacts,
            environment_state=self._settings.environment_state,
            guest_options=guest_options,
            guest_data=guest_data,
            notification_state=self._notification_state,
            location=self._settings.region.location.dict(),
            client_ip=self._settings.region.client_ip,
        )

    def _apply_directives_to_surface(self, sk_response_str):
        sk_response = deserialize(sk_response_str)
        logger.info(f'Found directives in response: {sk_response.response.directives}')
        self._surface._wrap_response(self._surface._handle_directives(sk_response.response.directives))

    def _make_experiments(self, scenario_name):
        experiments = {
            f'mm_enable_protocol_scenario={scenario_name}': '1',
            f'mm_scenario={scenario_name}': '1',
        }
        return {**experiments, **self._settings.experiments}

    def _make_additional_options(self):
        additional_options = {
            'supported_features': self._settings.supported_features,
            'unsupported_features': self._settings.unsupported_features,
        }
        additional_options['server_time_ms'] = self._surface.alice_time_ms + 2  # Server time is late a little bit
        additional_options.update(self._additional_options)
        return additional_options

    def _make_scenario_responses(self, dump_result, sources_dump):
        result = ScenarioResponses(sources_dump=sources_dump)
        for name, response in dump_result.items():
            if name == 'run':
                result.run_response = response
            elif name == 'continue':
                result.continue_response = response
            elif name == 'apply':
                result.apply_response = response
            elif name == 'commit':
                result.commit_response = response
            else:
                raise Exception(f'Unsupported name in dump_result: {name}')
        return result

    def _remove_old_scenario_request_files(self):
        if not self._test_path.exists():
            return
        for filename in self._test_path.iterdir():
            if filename.is_file() and filename.name.endswith('.pb.txt'):
                filename.unlink(missing_ok=True)

    def _write_output_protobuf_text(self, scenario_request, request_file_basename):
        request_out_file = self._test_path / f'{request_file_basename}.pb.txt'
        logger.info(f'Writing scenario request to a file {request_out_file}')
        scenario_request_proto_text = text_format.MessageToString(scenario_request, as_utf8=True)
        request_out_file.parent.mkdir(parents=True, exist_ok=True)
        with request_out_file.open('w') as f:
            f.write(scenario_request_proto_text)
