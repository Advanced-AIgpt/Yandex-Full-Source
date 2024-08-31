import abc
import logging
import sys
import os
import requests

from jinja2 import Template

from library.python import resource

import yatest.common
from library.recipes.common import start_daemon, stop_daemon
from library.python.testing.recipe import declare_recipe
from library.python.testing.recipe.ports import get_port_range, release_port_range

from jupytercloud.backend.tests.mock.spec import JupyterHubSpec


logger = logging.getLogger(__name__)


def setup_logging(level=logging.DEBUG):
    global logger
    logger.setLevel(level)

    log_format = '%(asctime)s - %(levelname)s - %(name)s - %(funcName)s: %(message)s'
    formatter = logging.Formatter(log_format)

    handler = logging.StreamHandler(sys.stderr)
    handler.setLevel(logging.DEBUG)
    handler.setFormatter(formatter)

    logger.addHandler(handler)


class Daemon(metaclass=abc.ABCMeta):
    pid_file = None
    ports_file = None
    daemon_name = None
    arcadia_path = None
    config_path = None

    start_timeout = 10

    def start(self):
        environment = os.environ.copy()
        new_path = yatest.common.work_path()
        if old_path := environment.get('PATH'):
            new_path = f'{new_path}:{old_path}'
        environment['PATH'] = new_path

        start_daemon(
            command=self.argv,
            environment=environment,
            is_alive_check=self.check_alive,
            pid_file_name=self.pid_file,
            timeout=self.start_timeout,
            daemon_name=self.daemon_name
        )

    def dump_config(self, key, path, config_params):
        template_str = resource.resfs_read(key)
        assert template_str
        template_str = template_str.decode('utf-8')

        template = Template(template_str)
        config = template.render(**config_params)

        with open(path, 'w') as f_:
            f_.write(config)

    def stop(self):
        with open(self.pid_file) as f:
            pid = f.read()

        if not stop_daemon(pid):
            logger.error("pid for %r is dead: %s", self.daemon_name, self.pid_file)

        if self.ports_file:
            release_port_range(self.ports_file)

    @property
    def argv(self):
        return [
            yatest.common.build_path(self.arcadia_path),
        ]

    @abc.abstractmethod
    def check_alive(self):
        pass


class Backend(Daemon):
    pid_file = 'backend.pid'
    ports_file = 'bakend.ports'
    daemon_name = 'backend'
    arcadia_path = 'jupytercloud/backend/tests/mock/app/mock_launcher'

    spec = None

    def start(self):
        self.hub_port = base_port = get_port_range(3, self.ports_file)

        self.spec = spec = JupyterHubSpec(
            hub_port=base_port,
            hub_ip='localhost',
            hub_prefix='/hub/',
            public_port=base_port + 1,
            traefik_port=base_port + 2,
            stdout_log=yatest.common.output_path('{}.out.log').format(self.daemon_name),
            stderr_log=yatest.common.output_path('{}.err.log').format(self.daemon_name),
        )

        spec.dump()

        self.dump_config(
            'jupytercloud/backend/tests/mock/recipe/config_template.tpl.py',
            'backend_config.py',
            config_params=spec.asdict()
        )

        super().start()

    @property
    def argv(self):
        return super().argv + ['backend', '--config', 'backend_config.py']

    def check_alive(self):
        try:
            response = self.spec.request('GET', '/api/solomon')
        except requests.ConnectionError:
            return False

        return response.status_code == 200


def start(argv):
    Backend().start()


def stop(argv):
    Backend().stop()


def main():
    setup_logging()
    declare_recipe(start, stop)


if __name__ == '__main__':
    main()
