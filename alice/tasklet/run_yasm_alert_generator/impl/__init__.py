import logging
import os
import subprocess

from alice.tasklet.run_yasm_alert_generator.proto import run_yasm_alert_generator_tasklet
from sandbox import sdk2, common

BLACKLISTED_DIRS = ['panels']
TESTING_FLAGS = ' -d -V -o'


class RunYasmAlertGeneratorImpl(run_yasm_alert_generator_tasklet.RunYasmAlertGeneratorBase):
    def run(self):
        cmd = ''
        binary_file = self._get_resource_data(self.input.config.binary_resource_id)
        data = self._get_resource_data(self.input.config.package_resource_id)

        logging.info('Running %s', binary_file)
        cmd += f'{binary_file} -A'
        if self.input.config.test_run:
            logging.info('Test run. Alerts won\'t apply')
            cmd += TESTING_FLAGS

        subdirs = self._get_all_config_dirs(data)
        logging.info("Data dir is: %s", data)
        logging.info("Subdirs are: %s", subdirs)
        logging.info("Cmd is: %s", cmd)
        for config_dir in subdirs:
            rc = self._spawn_test(cmd, config_dir)
            logging.info('Done with %s', rc)
            if not rc == 0:
                raise common.errors.TaskError(f'Subprocess failed at dir {config_dir}')

        self.output.state.success = True
        self.output.state.result = 'Ok'

    @staticmethod
    def _spawn_test(cmd, config_dir):
        logging.info('Running %s', config_dir)
        logfilename = config_dir.split('/')[-1]
        process = subprocess.Popen(
            cmd + f' -c "{config_dir}" -l log1/alerts.{logfilename}.log',
            env=os.environ.copy(),
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            close_fds=True
        )
        output, err = process.communicate()
        logging.info('Stdout:\n%s', output)
        logging.error('Stderr:\b%s', err)
        return process.returncode

    @staticmethod
    def _get_resource_data(resource_id):
        binary_resource = sdk2.Resource[resource_id]
        binary_file = sdk2.ResourceData(binary_resource)
        return binary_file.path.resolve()

    @staticmethod
    def _get_all_config_dirs(data):
        sub_dirs = []
        data = os.path.join(data, 'data')
        for filename in os.listdir(data):
            if filename in BLACKLISTED_DIRS:
                continue
            full_path = os.path.join(data, filename)
            if os.path.isdir(full_path):
                sub_dirs.append(full_path)
        return sub_dirs
