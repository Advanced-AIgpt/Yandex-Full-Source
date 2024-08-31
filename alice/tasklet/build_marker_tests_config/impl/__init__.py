import logging

from alice.tasklet.build_marker_tests_config.proto import build_marker_tests_config_tasklet
from sandbox.projects.common.arcadia import sdk
from sandbox import sdk2
from sandbox.projects.voicetech import resource_types

import os

CONFIG_PATH = 'alice/acceptance/cli/marker_tests/configs/config_production.yaml'
VINS_HAMSTER_URL = 'http://vins.hamster.alice.yandex.net/speechkit/app/pa/'

logging.basicConfig(level=logging.INFO)


class BuildMarkerTestsConfigImpl(build_marker_tests_config_tasklet.BuildMarkerTestsConfigBase):
    def run(self):
        logging.info(f'Checking out from {self.input.config.checkout_arcadia_from_url}')
        with sdk.mount_arc_path(self.input.config.checkout_arcadia_from_url,
                                use_arc_instead_of_aapi=True) as arcadia_path:
            prod_config = self._read_prod_config_from_arcadia(arcadia_path)

        config_text = self._generate_test_config(prod_config)
        result_resource_id = self._strore_in_resource(config_text)
        self.output.data.config = config_text
        self.output.data.resource_id = result_resource_id
        self.output.state.success = True

    def _generate_test_config(self, prod_config):
        config_text = ''
        for line in prod_config.split('\n'):
            if 'hitman_guid:' in line:
                continue
            uniproxy_url_pos = line.find('uniproxy_url:')
            if uniproxy_url_pos >= 0:
                line = line[0:uniproxy_url_pos] + 'uniproxy_url: ' + self.input.config.uniproxy_websocket_url
            if self.input.config.use_vins_hamster:
                vins_url_pos = line.find('vins_url:')
                if vins_url_pos >= 0:
                    line = line[0:vins_url_pos] + 'vins_url: ' + self.VINS_HAMSTER_URL
            config_text += line + '\n'
        return config_text

    def _strore_in_resource(self, config_text):
        result_filename = 'marker_tests_config.yaml'
        result_resource = resource_types.ALICE_MARKER_TESTS_CONFIG(
            sdk2.Task.current,
            "Alice uniproxy marker_tests config for url={}".format(self.input.config.checkout_arcadia_from_url),
            result_filename,
            ttl=40,
        )
        result_data = sdk2.ResourceData(result_resource)
        result_path = str(result_data.path)
        with open(result_path, 'w') as f:
            f.write(config_text)
        return result_resource.id

    @staticmethod
    def _read_prod_config_from_arcadia(arcadia_path):
        logging.info(f'Reading config from {arcadia_path}')
        with open(os.path.join(arcadia_path, CONFIG_PATH)) as file_handler:
            prod_config = file_handler.read()
        return prod_config
