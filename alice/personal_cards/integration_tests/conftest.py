import contextlib
import json
import os
import pytest
import random
import signal
import string
import subprocess

import yatest.common
from yatest.common.network import PortManager

from alice.personal_cards.integration_tests.lib.util import wait_ready

PERSONAL_CARDS_BINARY_PATH = 'alice/personal_cards/bin/personal_cards'
CONFIG_FILENAME = 'config.json'


def gen_table_path():
    length = 16
    return "/test_" + ''.join(random.choice(string.ascii_lowercase) for i in range(length))


CONFIG_JSON = {}


@contextlib.contextmanager
def bring_up_personal_cards():
    with PortManager() as pm:
        host = '127.0.0.1'
        port = pm.get_port()

        personal_cards_backend = yatest.common.build_path(PERSONAL_CARDS_BINARY_PATH)

        with open("tvmapi.port", "r") as f:
            tvmapi_port = int(f.read())

        config_json = {
            "YDBClient": {
                "Address": os.getenv("YDB_ENDPOINT"),
                "DBName": os.getenv("YDB_DATABASE"),
                "Path": gen_table_path(),
                "ReadReplicasSettings": "PER_AZ:2",
                "OperationTimeout": "10s",
                "ClientTimeout": "10s",
                "CancelAfter": "10s",
                "MaxRetries": 5,
            },
            "HttpServer": {
                "Port": port,
                "Threads": 10,
            },
            "Tvm": {
                "TrustedServicesTvmIds": [
                    2005865,
                ],
                "Api": {
                    "BlackboxEnv": "Test",
                    "SelfTvmId": 2026486,
                },
                "Host": "localhost",
                "Port": tvmapi_port
            },
            "LockAllMemory": False
        }

        global CONFIG_JSON
        CONFIG_JSON = config_json.copy()

        with open(CONFIG_FILENAME, 'w') as f:
            f.write(json.dumps(config_json))

        argv = [
            personal_cards_backend,
            "--config",
            CONFIG_FILENAME
        ]

        process = subprocess.Popen(argv)
        url = "http://{host}:{port}".format(host=host, port=port)
        try:
            wait_ready("{}/ping".format(url), timeout=60)
            yield url
        finally:
            process.send_signal(signal.SIGINT)
            assert process.wait() == 0


@pytest.fixture(scope='function')
def personal_cards_url():
    with bring_up_personal_cards() as url:
        yield url


@pytest.fixture(scope='function')
def cfg():
    yield CONFIG_JSON
