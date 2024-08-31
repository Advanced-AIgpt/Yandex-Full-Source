import logging
import time
from typing import Dict, List, Tuple

import nanny_rpc_client
from infra.nanny.yp_lite_api.proto import endpoint_sets_api_pb2
from infra.nanny.yp_lite_api.py_stubs import endpoint_sets_api_stub
from search.priemka.yappy.proto.structures.api_pb2 import CreateBetaFromTemplate as CreateBetaFromTemplateAPI
from search.priemka.yappy.proto.structures.beta_pb2 import BetaFilter
from search.priemka.yappy.proto.structures.patch_pb2 import Patch as YappyPatch
from search.priemka.yappy.proto.structures.payload_pb2 import Lineage2Response
from search.priemka.yappy.proto.structures.status_pb2 import Status
from search.priemka.yappy.services.yappy.services import Lineage2Client, Model
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.create_yappy_from_template.proto import create_yappy_from_template_tasklet
from sandbox import common

DEFAULT_MAX_RETRIES = 5
DEFAULT_SLEEP_TIME = 60
DEFAULT_BACKOFF = 1.5
DEFAULT_START_TIME = 30

DEFAULT_YAPPY_TOKEN_KEY = 'yappy.token'
DEFAULT_NANNY_TOKEN_KEY = 'nanny.token'

DEFAULT_YAPPY_URL = 'yappy.z.yandex-team.ru'

YPLITE_API_URL = 'https://yp-lite-ui.nanny.yandex-team.ru/api/yplite/endpoint-sets/'

REQUIRED_FIELDS = ('template_name',
                   'suffix',
                   'patches',
                   'token_id',
                   'component_id',
                   'parent_external_id')

DCS = ('SAS', 'MAN', 'VLA')


class CreateBetaFromTemplateImpl(create_yappy_from_template_tasklet.CreateBetaFromTemplateBase):
    def run(self):
        template_name = self.input.config.template_name
        suffix = self.input.config.suffix
        patches = self.input.config.patches
        token_id = self.input.config.token
        component_id = self.input.config.component_id
        parent_external_id = self.input.config.parent_external_id
        beta_name = '-'.join((template_name, suffix))

        self.output.state.success = False

        yappy_token_key = DEFAULT_YAPPY_TOKEN_KEY
        nanny_token_key = DEFAULT_NANNY_TOKEN_KEY

        if self.input.config.yappy_token != '':
            yappy_token_key = self.input.config.yappy_token
        if self.input.config.nanny_token != '':
            nanny_token_key = self.input.config.nanny_token

        yappy_token = self._get_token_from_yav(token_id, yappy_token_key)
        nanny_token = self._get_token_from_yav(token_id, nanny_token_key)

        retries_config = self._get_retries_config()

        try:
            self._create_beta(patches, suffix, template_name, yappy_token, component_id, parent_external_id)
            self._wait_for_beta(beta_name, retries_config, yappy_token, self.input.context.job_instance_id.number)
            nanny_service = self._get_nanny_service(beta_name, yappy_token)
            endpoint_set, endpoint_port = self._get_endpointsets(nanny_service, nanny_token)
        except common.errors.TaskError as e:
            raise common.errors.TaskFailure('Task failed %s' % e)

        self.output.state.beta_name = beta_name
        self.output.state.endpoint_set = endpoint_set
        self.output.state.endpoint_port = endpoint_port
        self.output.state.success = True

    def _get_token_from_yav(self, token_id: str, token_key: str) -> str:
        spec = yav.YavSecretSpec(uuid=token_id, key=token_key)
        return self.ctx.yav.get_secret(spec, default_key=token_key).secret

    def _get_retries_config(self) -> Dict:
        retries_config = {'retries': DEFAULT_MAX_RETRIES,
                          'sleep_time': DEFAULT_SLEEP_TIME,
                          'backoff': DEFAULT_BACKOFF,
                          'wait_for_beta': DEFAULT_START_TIME}
        if self.input.config.retries != 0:
            retries_config['retries'] = self.input.config.retries
        if self.input.config.sleep_time != 0:
            retries_config['sleep_time'] = self.input.config.sleep_time
        if self.input.config.backoff != 0:
            retries_config['backoff'] = self.input.config.backoff
        if self.input.config.wait_for_beta != 0:
            retries_config['wait_for_beta'] = self.input.config.wait_for_beta
        return retries_config

    def _create_beta(self,
                     patches: List,
                     suffix: str,
                     template_name: str,
                     yappy_token: str,
                     component_id: str,
                     parent_external_id: str) -> bool:
        logging.info("Creating beta request to Yappy")
        self.yappy = Lineage2Client.from_address(DEFAULT_YAPPY_URL, credentials=yappy_token)
        patch_map = CreateBetaFromTemplateAPI.PatchMap()
        patch_map.component_id = component_id
        patch_map.patch.parent_external_id = parent_external_id
        patch_map.patch.resources.extend([
            YappyPatch.Resource(
                manage_type=YappyPatch.Resource.ManageType.SANDBOX_RESOURCE, local_path=x.name,
                sandbox_resource_id=str(x.resource_id))
            for x in patches])
        request = CreateBetaFromTemplateAPI(
            template_name=template_name,
            patches=(patch_map,),
            suffix=suffix,
            update_if_exist=True,
        )
        yappy_response = self.yappy.create_beta_from_beta_template(request=request)
        if yappy_response.status == Lineage2Response.Status.FAILED:
            raise common.errors.TaskError('Yappy request failed: %s' % yappy_response.error)
        logging.info("Request sent to Yappy")
        return True

    @staticmethod
    def _wait_for_beta(beta_name: str, retries_config: Dict, yappy_token: str, ci_run_number: int) -> bool:
        logging.info('Waiting for %s to start', beta_name)
        retry_counter = 0
        model = Model.ModelClient.from_address(DEFAULT_YAPPY_URL, credentials=yappy_token)

        sleep_for = retries_config['sleep_time']
        if ci_run_number == 1:
            time.sleep(retries_config['wait_for_beta'])
        while retry_counter < retries_config['retries']:
            if _check_beta_status(model, beta_name):
                return True
            logging.info('Beta is not consistent yet. Sleeping for %d seconds (retry %d)', sleep_for, retry_counter)
            time.sleep(sleep_for)
            sleep_for = sleep_for * retries_config['backoff']
            retry_counter += 1
        raise common.errors.TaskError('Beta creation took too long')

    @staticmethod
    def _get_nanny_service(beta_name: str, yappy_token: str) -> str:
        logging.info('Requesting beta slot service from %s', beta_name)
        model = Model.ModelClient.from_address(DEFAULT_YAPPY_URL, credentials=yappy_token)
        nanny_slot = model.retrieve_beta(BetaFilter(name=beta_name)).components[0].slot.id
        return nanny_slot

    @staticmethod
    def _get_endpointsets(nanny_service: str, nanny_token: str) -> Tuple[str, int]:
        logging.info('Resolving endpointsets')
        c = nanny_rpc_client.RetryingRpcClient(rpc_url=YPLITE_API_URL,
                                               oauth_token=nanny_token)
        stub = endpoint_sets_api_stub.YpLiteUIEndpointSetsServiceStub(c)
        req = endpoint_sets_api_pb2.ListEndpointSetsRequest()
        for dc in DCS:
            req.cluster = dc
            req.service_id = nanny_service
            endpointsets_in_dc = stub.list_endpoint_sets(req)
            try:
                return endpointsets_in_dc.endpoint_sets[0].meta.id.upper(), endpointsets_in_dc.endpoint_sets[
                    0].spec.port
            except Exception as e:
                logging.info(f'{dc} skipped ({e})')


def _check_beta_status(model, beta_name):
    status = model.get_beta_status(BetaFilter(name=beta_name))
    return status.state.status == Status.CONSISTENT
