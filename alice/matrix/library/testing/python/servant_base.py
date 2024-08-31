import json
import os
import requests
import signal
import time

from alice.matrix.library.testing.python.helpers import fix_timeout_for_sanitizer

from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient

import yatest.common
import yatest.common.network


class ServantBase:
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, env={}):
        self._env = env

        self._exec = None

        self._port_manager = yatest.common.network.PortManager()
        self._http_port = self._port_manager.get_port()
        self._grpc_port = self._port_manager.get_port()

        self._http_session = requests.Session()
        self._http_session.mount("http://", requests.adapters.HTTPAdapter(max_retries=10))

        self._apphost_grpc_client = AppHostGrpcClient(self.grpc_endpoint)

    def _get_default_ydb_client_config(self):
        return {
            "OperationTimeout": "10s",
            "ClientTimeout": "10s",
            "CancelAfter": "10s",
            "MaxRetries": 5,
        }

    def _get_default_config(self):
        return {
            "Server": {
                "HttpPort": self._http_port,
                "GrpcPort": self._grpc_port,
            },
            "Log": {
                "FilePath": self._eventlog_file_path,
            },
            "RtLog": {
                "FilePath": self._rtlog_file_path,
                "Service": self.name,
            },
            "YDBClientCommon": {
                "Address": os.getenv("YDB_ENDPOINT"),
                "DBName": os.getenv("YDB_DATABASE"),
            },
            "LockAllMemory": False,
        }

    def _get_config(self):
        return self._get_default_config()

    def _get_argv(self):
        return [
            self._binary_file_path,
            "run",
            "-c",
            self._config_file_name,
        ]

    def _get_exec_kwargs(self):
        return {
            "wait": False,
            "env": self._env,
            "stdout": self._stdout_file_path,
            "stderr": self._stderr_file_path,
        }

    def _execute_bin(self):
        with open(self._config_file_name, "w") as f:
            f.write(json.dumps(self._get_config()))

        return yatest.common.execute(
            self._get_argv(),
            **self._get_exec_kwargs(),
        )

    def _before_start(self):
        pass

    def _after_start(self):
        pass

    def start(self):
        self._before_start()

        self._exec = self._execute_bin()
        self.wait_ready()

        self._after_start()

    def _stop_action(self):
        self.perform_get_request(self._shutdown_path)

    def stop(self):
        assert self._exec.process.poll() is None, "Process died before shutdown"

        try:
            self._stop_action()
            self._exec.wait(check_exit_code=False, timeout=fix_timeout_for_sanitizer(10))
            assert self._exec.exit_code == self._expected_exit_code
        except:
            try:
                self._exec.process.send_signal(signal.SIGKILL)
                self._exec.wait(check_exit_code=False, timeout=fix_timeout_for_sanitizer(10))
            finally:
                raise RuntimeError("Graceful shutdown failed")
        finally:
            self._port_manager.release_port(self._http_port)
            self._port_manager.release_port(self._grpc_port)

    @property
    def name(self):
        raise NotImplementedError()

    @property
    def _binary_file_path(self):
        raise NotImplementedError()

    @property
    def _config_file_name(self):
        return "config.json"

    @property
    def _stdout_file_path(self):
        return yatest.common.test_output_path(f"{self.name}.out")

    @property
    def _stderr_file_path(self):
        return yatest.common.test_output_path(f"{self.name}.err")

    @property
    def _eventlog_file_path(self):
        return yatest.common.test_output_path(f"{self.name}.evlog")

    @property
    def _rtlog_file_path(self):
        return yatest.common.test_output_path(f"{self.name}.rtlog")

    @property
    def _expected_exit_code(self):
        return 0

    @property
    def _wait_ready_timeout(self):
        return 60

    @property
    def host(self):
        return "localhost"

    def _endpoint(self, port):
        return f"{self.host}:{port}"

    def _path_url(self, path):
        if path.startswith("/"):
            return f"http://{self.http_endpoint}{path}"
        else:
            return f"http://{self.http_endpoint}/{path}"

    def _admin_action_path(self, action):
        return f"/admin?action={action}"

    @property
    def http_endpoint(self):
        return self._endpoint(self._http_port)

    @property
    def grpc_endpoint(self):
        return self._endpoint(self._grpc_port)

    @property
    def _ping_path(self):
        return self._admin_action_path("ping")

    @property
    def _shutdown_path(self):
        return self._admin_action_path("shutdown")

    def wait_ready(self, timeout=None):
        if timeout is None:
            timeout = self._wait_ready_timeout

        deadline = time.monotonic() + fix_timeout_for_sanitizer(timeout)
        while True:
            try:
                self.perform_get_request(self._ping_path)
                break
            except:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.2)

    def _perform_http_request(self, path, method, raise_for_status=True, **kwargs):
        response = method(self._path_url(path), **kwargs)
        if raise_for_status:
            response.raise_for_status()
        return response

    def perform_get_request(self, path, **kwargs):
        return self._perform_http_request(path, self._http_session.get, **kwargs)

    def perform_post_request(self, path, **kwargs):
        return self._perform_http_request(path, self._http_session.post, **kwargs)

    def perform_delete_request(self, path, **kwargs):
        return self._perform_http_request(path, self._http_session.delete, **kwargs)

    def create_apphost_grpc_stream(self, path, **kwargs):
        return self._apphost_grpc_client.create_stream(path, **kwargs)

    async def perform_grpc_request(self, items, path, timeout=None, **kwargs):
        async with self.create_apphost_grpc_stream(path=path, timeout=timeout, **kwargs) as stream:
            stream.write_items(items=items, last=True)
            response = await stream.read(timeout=timeout)

            return response
