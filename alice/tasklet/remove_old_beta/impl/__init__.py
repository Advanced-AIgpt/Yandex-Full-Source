import logging
import time
from typing import Dict

from search.priemka.yappy.proto.structures.api_pb2 import ApiBetaFilter
from search.priemka.yappy.proto.structures.beta_pb2 import BetaFilter
from search.priemka.yappy.proto.structures.status_pb2 import Status
from search.priemka.yappy.services.yappy.services import Lineage2Client, Model
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.remove_old_beta.proto import remove_old_beta_tasklet
from sandbox import common

DEFAULT_MAX_RETRIES = 5
DEFAULT_SLEEP_TIME = 60
DEFAULT_BACKOFF = 1.5
DEFAULT_START_TIME = 30

DEFAULT_YAPPY_TOKEN_KEY = 'yappy.token'

DEFAULT_YAPPY_URL = 'yappy.z.yandex-team.ru'

ACCEPTED_STATUSES = (
    Status.STOPPED,
    Status.MISSING,
    Status.OUTDATED
)


class RemoveOldBetaImpl(remove_old_beta_tasklet.RemoveOldBetaBase):
    def run(self):
        self.output.state.success = False

        yappy_token_key = DEFAULT_YAPPY_TOKEN_KEY

        if self.input.config.yappy_token != '':
            yappy_token_key = self.input.config.yappy_token

        token_id = self.input.config.token
        yappy_token = self._get_token_from_yav(token_id, yappy_token_key)

        retries_config = self._get_retries_config()

        version_to_delete = int(self.input.config.current_version) - self.input.config.age_to_delete
        beta_name = f'{self.input.config.template_name}-{version_to_delete}'
        yappy_client = Lineage2Client.from_address(DEFAULT_YAPPY_URL, credentials=yappy_token)
        yappy_model = Model.ModelClient.from_address(DEFAULT_YAPPY_URL, credentials=yappy_token)

        try:
            self._remove_beta(yappy_client, yappy_model, beta_name, retries_config)
        except common.errors.TaskError as e:
            raise common.errors.TaskFailure('Task failed %s' % e)

        self.output.state.success = True

    def _remove_beta(self, yappy_client, yappy_model, beta_name, retries_config):
        if not _check_beta_exists(yappy_model, beta_name) or _check_beta_status(yappy_model, beta_name):
            self.output.state.success = True
            return
        logging.info('Beta is present. Requesting removal.')
        removal_request = ApiBetaFilter(name=beta_name)
        yappy_client.deallocate_beta(removal_request)
        logging.info('Removal request sent. Waiting for completion.')
        self._wait_for_beta(beta_name, retries_config, yappy_model)

    @staticmethod
    def _wait_for_beta(beta_name: str, retries_config: Dict, model: Model) -> bool:
        logging.info('Waiting for %s to be destroyed', beta_name)
        retry_counter = 0

        sleep_for = retries_config['sleep_time']
        while retry_counter < retries_config['retries']:
            if _check_beta_status(model, beta_name):
                return True
            logging.info('Beta is not down yet. Sleeping for %d seconds (retry %d)', sleep_for, retry_counter)
            time.sleep(sleep_for)
            sleep_for = sleep_for * retries_config['backoff']
            retry_counter += 1
        raise common.errors.TaskError('Beta removal took too long')

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


def _check_beta_exists(model, beta_name):
    logging.info(f'Checking existence of {beta_name}')
    return model.beta_exists(BetaFilter(name=beta_name)).exists


def _check_beta_status(model, beta_name):
    logging.info(f'Checking status for {beta_name}')
    status = model.get_beta_status(BetaFilter(name=beta_name))
    logging.info(f'Status for {beta_name} is {status.state.status}')
    return status.state.status in ACCEPTED_STATUSES
