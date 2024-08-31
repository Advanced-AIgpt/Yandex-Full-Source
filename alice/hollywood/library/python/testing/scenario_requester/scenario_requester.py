import json
import os
import requests
import logging

from library.langs import langs
from apphost.python.client.client import Client, ItemDescriptor
from alice.megamind.protos.scenarios.request_pb2 import TDataSource
from alice.megamind.protos.scenarios.request_meta_pb2 import TRequestMeta
from alice.megamind.protos.common.data_source_type_pb2 import EDataSourceType
from alice.protos.data.language.language_pb2 import ELang

logger = logging.getLogger(__name__)

SCENARIO_STAGE_TO_INPUT_NODE = {
    'run': 'WALKER_RUN_STAGE0',
    'continue': 'WALKER_POST_CLASSIFY',
    'apply': 'WALKER_APPLY_PREPARE',
    'commit': 'WALKER_APPLY_PREPARE',
}


class ScenarioRequestError(Exception):
    pass


class ScenarioRequester:

    _filtered_errors = [
        '(Connection refused) balancer/kernel/connection_manager_helpers/connect.cpp:79: ECONNREFUSED connect to HOST:PORT',
        '(NAppHost::NGrpc::NClient::TGrpcError) 14 UNAVAILABLE: failed to connect to HOST:PORT',
        'no output deps fetched',
    ]

    def __init__(self, apphost, oauth_token):
        self._apphost = apphost
        self._apphost_client = Client(host=apphost.host, port=apphost.grpc_port)
        self._oauth_token = oauth_token

    def request(self, graph_name, request_proto, scenario_stage, response_cls, srcrwr_params={}):
        logger.info(f'Preparing request graph_name: {graph_name}, scenario_stage:{scenario_stage}, '
                    f'response_cls: {response_cls}, srcrwr_params: {srcrwr_params}, request_proto: {request_proto}')
        item_descriptors = []

        data_sources = self._extract_data_sources(request_proto)
        for data_source_name, data_source_value in data_sources.items():
            item_descriptors.append(ItemDescriptor(f'datasource_{data_source_name}', 'DATASOURCES',
                                                   [data_source_value]))

        request_proto.DataSources.clear()
        item_descriptors.append(ItemDescriptor('mm_scenario_request',
                                               SCENARIO_STAGE_TO_INPUT_NODE[scenario_stage], [request_proto]))

        meta = self._make_request_meta(request_proto)
        item_descriptors.append(ItemDescriptor('mm_scenario_request_meta',
                                               SCENARIO_STAGE_TO_INPUT_NODE[scenario_stage], [meta]))

        item_descriptors.append(ItemDescriptor('app_host_params', 'APP_HOST_PARAMS',
                                               self._make_apphost_params(request_proto, srcrwr_params)))

        apphost_client_response = self._apphost_client.request_graph_with_multiple_input_nodes(
            graph_name,
            item_descriptors=item_descriptors
        )

        logger.info(f'Request to scenario graph {graph_name} returned: {apphost_client_response.get_all_type_names()}')
        try:
            response = apphost_client_response.get_only_type_item('mm_scenario_response', response_cls)
            response.Version = 'trunk@******'  # Rewriting version because we want reproducible results
        except Exception as exc:
            raise ScenarioRequestError(f'Request to scenario graph {graph_name} was failed') from exc

        if os.environ.get('SCENARIO_WITH_EXCEPTIONS') == 'true':
            logger.info('Skip looking into error booster log for errors')
            return response

        self._raise_if_error_booster_log_has_errors(request_proto.BaseRequest.RequestId)

        return response

    def _make_apphost_params(self, request_proto, srcrwr_params):
        return [
            {'reqid': request_proto.BaseRequest.RequestId},
            {'srcrwr': srcrwr_params},
            {'timeout': 100000},
            {'waitall': 'da'},
            {'dump_source_requests': 1},
            {'dump_source_responses': 1},
        ]

    def _extract_data_sources(self, request_proto):
        result = {}
        for data_source_type, data_source in request_proto.DataSources.items():
            data_source_name = EDataSourceType.Name(data_source_type)
            pb_type = data_source.WhichOneof('Type')
            logger.info(f'Found data_source={data_source_name}, PbType={pb_type}, data_source={data_source.SerializeToString()}')
            data_source_copy = TDataSource()
            data_source_copy.CopyFrom(data_source)  # We copy because a little bit later we do DataSources.clear()
            result[data_source_name] = data_source_copy
        return result

    def _make_request_meta(self, request_proto):
        meta = TRequestMeta()
        meta.ContentType = 'application/protobuf'
        if self._oauth_token:
            meta.OAuthToken = self._oauth_token
        meta.UserTicket = 'dummy_it2_tvm_user_ticket'
        base_request = request_proto.BaseRequest
        meta.RequestId = base_request.RequestId
        meta.RandomSeed = base_request.RandomSeed
        meta.Lang = base_request.ClientInfo.Lang
        if base_request.HasField('Options'):
            meta.ClientIP = base_request.Options.ClientIP
        if base_request.UserLanguage != ELang.L_UNK:
            user_lang_iso = langs.IsoNameByLanguage(base_request.UserLanguage)
            logger.info(f'base_request.UserLanguage = {base_request.UserLanguage}, user_lang_iso = {user_lang_iso}, type={type(user_lang_iso)}')
            meta.UserLang = str(user_lang_iso)

        return meta

    def _raise_if_error_booster_log_has_errors(self, request_id):
        logger.info('Looking into error booster log for errors...')

        flush_url = f'http://localhost:{self._apphost.port}/admin?action=flush'
        apphost_flush_response = requests.get(flush_url)
        logger.info(f'{flush_url} returned {apphost_flush_response}')

        alice_path = f'{self._apphost.local_app_host_dir}/ALICE'
        if not os.path.exists(alice_path):
            raise Exception(f'{alice_path} doesn\'t exist')

        error_booster_path = f'{alice_path}/error_booster_log-{self._apphost.port}'
        if not os.path.exists(error_booster_path):
            raise Exception(f'Error booster log not found: {error_booster_path}')

        if os.stat(error_booster_path).st_size > 0:
            with open(error_booster_path, 'r') as error_booster:
                for line in error_booster.readlines():
                    error_booster_json = json.loads(line)
                    if error_booster_json['reqid'] == request_id:
                        assert error_booster_json['level'] == 'error'
                        error_message = error_booster_json['message']
                        if error_message not in self._filtered_errors:
                            raise Exception(f'An error was found in {error_booster_path}: \n'
                                            f'    {error_message}\n'
                                            'For more information see <TESTING_OUT_STUFF>/hollywood.err '
                                            f'and/or other related logs...')
        else:
            logger.info(f'Log file {error_booster_path} is empty')

        logger.info('No errors found in error booster log')
