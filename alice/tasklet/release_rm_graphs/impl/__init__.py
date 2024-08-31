import logging
import time

from tasklet.services.yav.proto import yav_pb2 as yav

import sandbox.projects.release_machine.core.const as rm_const
from alice.tasklet.release_rm_graphs.proto import release_rm_graphs_tasklet
from sandbox import common
from sandbox.projects.common.testenv_client import TEClient
from sandbox.projects.release_machine import client
from sandbox.projects.release_machine.components import all as all_rm_components

TE_TOKEN_KEY = 'te.token'


class ReleaseRmGraphsImpl(release_rm_graphs_tasklet.ReleaseRmGraphsBase):
    def run(self):
        testenv_token = self.ctx.yav.get_secret(
            yav.YavSecretSpec(uuid=self.input.context.secret_uid, key=TE_TOKEN_KEY),
            default_key=TE_TOKEN_KEY
        ).secret

        self.retries_config = {
            'sleep_time': self.input.config.sleep_time,
            'retries': self.input.config.retries,
            'backoff': self.input.config.backoff
        }

        self.te_client = TEClient(oauth_token=testenv_token)
        self.rm_client = client.RMClient()

        c_info = all_rm_components.get_component(self.input.config.component_name)
        te_database = c_info.testenv_cfg__db_template.format(testenv_db_num=self.input.config.branch)
        response = self.rm_client.post_release(
            self.input.config.svn_revision,
            te_database,
            self.input.config.component_name,
            self.input.config.release_to,
            '{}__{}'.format(c_info.name, rm_const.ReleaseStatus.stable).upper(),
            'Release from wsproxy CI',
            self.input.config.branch,
            self.input.config.tag_number
        )

        logging.info("Got response for release request: %s", response)

        self._wait_for(self._release_is_ok, te_database)
        self.output.state.success = True

    def _wait_for(self, func, *args):
        logging.info("Waiting for release")
        retry_counter = 0
        sleep_for = self.retries_config['sleep_time']
        while retry_counter < self.retries_config['retries']:
            result = func(*args)
            if result:
                return result
            logging.info('Data is not ready yet. Sleeping for %d seconds (retry %d)', sleep_for, retry_counter)
            time.sleep(sleep_for)
            sleep_for = sleep_for * self.retries_config['backoff']
            retry_counter += 1
        raise common.errors.TaskError("Release didn't happen in time")

    def _release_is_ok(self, te_database):
        aux_runs = self.te_client.get_aux_runs(db=te_database)
        logging.info("Got aux runs: %s", aux_runs)
        if aux_runs:
            aux_check = aux_runs[0]['auxiliary_check']
            logging.info("Check result: %s", aux_check)
            if 'FINISHED' in aux_check:
                logging.info('Check finished')
                return True
            else:
                logging.info('Not ready yet')
                return False
        return False
