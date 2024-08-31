import json
import os
import signal

import yatest.common

from .constants import ServiceHandlers
from .ydb import init_matrix_ydb

from alice.matrix.library.testing.python.servant_base import ServantBase


class PythonNotificator(ServantBase):
    def __init__(
        self,

        tvm_api_port=None,
        tvm_client_id=None,
        tvm_client_secret=None,

        iot_port=None,
        iot_tvm_client_id=None,

        subway_port=None,

        pushes_and_notifications_mock_mode=False,
        user_white_list=None,

        env={},
        *args,
        **kwargs,
    ):
        env.update({
            "UNIPROXY_CUSTOM_ENVIRONMENT_TYPE": "notificator",

            "TVM_APP_SECRET": tvm_client_secret,

            # This values don't affect anything
            "CUSTOM_SETTINGS_DIR": "PWD",
            "YDB_TOKEN_DEVICE": "token",
        })
        super(PythonNotificator, self).__init__(env=env, *args, **kwargs)

        self._tvm_client_id = tvm_client_id
        self._tvm_api_port = tvm_api_port

        self._iot_port = iot_port
        self._iot_tvm_client_id = iot_tvm_client_id

        self._subway_port = subway_port

        self._pushes_and_notifications_mock_mode = pushes_and_notifications_mock_mode
        self._user_white_list = user_white_list

    def _get_config(self):
        # Legacy python service :(

        config = dict()
        with open(yatest.common.source_path("alice/uniproxy/library/settings/notificator.json"), "r") as f:
            config = json.load(f)

        config["debug_logging"] = True
        config["rtlog"]["file_name"] = self._rtlog_file_path

        config["notificator"]["ydb"]["endpoint"] = os.getenv("YDB_ENDPOINT")
        config["notificator"]["ydb"]["database"] = os.getenv("YDB_DATABASE")

        config["client_id"] = self._tvm_client_id or 0
        config["tvm"] = {
            "url": f"http://localhost:{self._tvm_api_port or 0}"
        }

        config["smart_home"]["url"] = f"http://localhost:{self._iot_port or 0}/{ServiceHandlers.HTTP_IOT_USER_INFO}"
        config["smart_home"]["request_timeout"] = 10
        config["smart_home"]["service_id"] = self._iot_tvm_client_id or 0

        config["subway"]["port_override"] = self._subway_port or 0
        config["subway"]["host_override"] = "localhost"

        if self._user_white_list is not None:
            config["notificator"]["white_list"] = {
                "enabled": True,
                "puids": self._user_white_list,
            }

        return config

    def _get_argv(self):
        # Legacy python service :(

        argv = [
            self._binary_file_path,
            "-p",
            str(self._http_port),
            "-n",
            "1",
            "-v",
            "--no-remove-missing",
        ]
        if self._pushes_and_notifications_mock_mode:
            argv.append("--mock-delivery")

        return argv

    def _before_start(self):
        init_matrix_ydb()

    def _stop_action(self):
        # Legacy python service :(
        self.perform_get_request(self._ping_path)
        self.perform_get_request(self._shutdown_path)

        ping_response = self.perform_get_request(self._ping_path, raise_for_status=False)
        assert ping_response.status_code == 503

        self._exec.process.send_signal(signal.SIGTERM)

    @property
    def name(self):
        return "python_notificator"

    @property
    def _binary_file_path(self):
        return yatest.common.build_path("alice/uniproxy/bin/notificator/notificator")

    @property
    def _config_file_name(self):
        return "notificator.json"

    @property
    def _expected_exit_code(self):
        # Legacy python service :(
        return -signal.SIGTERM

    @property
    def _ping_path(self):
        # Legacy python service :(
        return "/ping"

    @property
    def _shutdown_path(self):
        # Legacy python service :(
        return "/stop_hook"
