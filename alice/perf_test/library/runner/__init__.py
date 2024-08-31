import alice.perf_test.library.joker as joker_module
from alice.perf_test.library.runner.bmv import AliceServer
from alice.perf_test.library.runner.request import SpeechKit

import logging
import os.path
import requests
import time

logger = logging.getLogger(__name__)


def wait_for_christmas(url, timeout_seconds=10, attempts=50):
    for t in range(attempts):
        try:
            req = requests.get(url)
            return req.text
        except:
            pass
        time.sleep(timeout_seconds)
    return None


class JokerConfig(joker_module.Config):
    def __init__(self,  settings):
        super(JokerConfig, self).__init__(
            project='megamind',
            session_id='default',
            workdir=settings.get_workdir('.joker.temp'),
            binary_path=settings.get_binary_path(),
            skip_headers=[
                'oauth',  # User token
                'x-rtlog-token',  # Token is always changing
                'x-ya-service-ticket',  # Blackbox always uses current timestamp in request
                'authorization',  # Blackbox can use it
                'x-yandex-proxy-header-x-yandex-via-proxy',
                'x-yandex-proxy-header-x-yandex-proxy-header-x-yandex-joker',
                'x-basshamsterquota',  # hamster uses it
                'date',  # hamster uses it
                'expires',  # hamster uses it
                'x-yandex-internal-flags',  # hamster uses it
                'x-request-id',  # music uses it
            ],
            skip_cgis=[
                'userip',  # For blackbox source
                'ts',
                'sign',
            ],
            stub_force_update=settings.is_force_update_enabled(),
            stub_fetch_if_not_exists=settings.is_fetch_if_not_exists_enabled(),
        )


class Runner:
    def __init__(
        self,
        joker_port,
        megamind_port,
        joker_settings,
        vins_package_path,
        saved_stubs_directory='saved_stubs',
        log_dir='logs',
        run_all=False,
    ):
        self.run_all = run_all

        # Run joker.
        self.joker_config = JokerConfig(joker_settings)
        self.joker = joker_module.JokerMocker(
            self.joker_config,
            url_requester=wait_for_christmas,
            port=joker_port,
            run_all=run_all,
            save_stubs=saved_stubs_directory,
            log_dir=log_dir
        )

        # Run alice
        self.alice = AliceServer(
            url_requester=wait_for_christmas,
            port=megamind_port,
            vins_package_path=vins_package_path,
            log_dir=log_dir,
            run_all=run_all,
        )
        self.speechkit = SpeechKit(self.alice, self.joker)

    def get_address(self):
        return self.alice.get_address()

    def sync_stubs(self, filename):
        try:
            sync_request = joker_module.JokerSyncRequest(self.joker)
            with open(filename, 'r') as f:
                linenum = 0
                for line in f:
                    linenum += 1
                    version, stub_data = line.rstrip().split('\t', 1)
                    if version != '1':
                        raise joker_module.JokerSyncRequest.Exception("Unsupported version of stub in file '{}:{}'".format(filename, linenum))

                    sync_request.add_stub(stub_data)

            sync_request.check_run()
        except FileNotFoundError as e:
            logger.info('unable to open file (%s) for sync stubs: %s', filename, e)

    def tear_down(self):
        try:
            self.alice.stop()
        except Exception as e:
            logger.error("Unable to stop megamind server %s", e)

        try:
            self.joker.stop()
        except Exception as e:
            logger.error("Unable to stop joker server %s", e)

    def request(self, json):
        return self.speechkit.request(json)

    def info(self):
        self.joker.info()

    def push(self, stubs_filename, rewrite=False):
        self.joker.push(stubs_filename, rewrite)
