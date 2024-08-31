import json
import logging
import signal
import subprocess
import time
import urllib.error
import urllib.request

from alice.cuttlefish.library.python.test_utils import fix_timeout_for_sanitizer


class IApphostServantPortManager:
    def get_http_port(self):
        pass

    def get_grpc_port(self):
        pass

    def release_port(self, value):
        pass


class SimpleApphostServantPortManager(IApphostServantPortManager):
    def __init__(self, http_port, grpc_port):
        self.http_port = http_port
        self.grpc_port = grpc_port

    def get_http_port(self):
        return self.http_port

    def get_grpc_port(self):
        return self.grpc_port

    def release_port(self, value):
        pass


class YaTestApphostServantPortManager(IApphostServantPortManager):
    def __init__(self, yatest_port_manager):
        self.yatest_port_manager = yatest_port_manager

    def get_http_port(self):
        return self.yatest_port_manager.get_port()

    def get_grpc_port(self):
        return self.yatest_port_manager.get_port()

    def release_port(self, value):
        if value is not None:
            self.yatest_port_manager.release_port(value)


class IApphostServantProcess:
    def send_signal(self, signal_number):
        pass

    def get_pid(self):
        pass

    def wait(self, timeout):
        pass

    def poll(self):
        pass


class ApphostServantProcess(IApphostServantProcess):
    def __init__(self, cmd, env):
        self.pipe = subprocess.Popen(cmd, env=env)

    def send_signal(self, signal_number):
        self.pipe.send_signal(signal_number)

    def get_pid(self):
        return self.pipe.pid

    def wait(self, timeout):
        self.pipe.wait(timeout=fix_timeout_for_sanitizer(timeout))

    def poll(self):
        return self.pipe.poll()


class YaTestApphostServantProcess(IApphostServantProcess):
    def __init__(self, ya_test_subproc):
        self.ya_test_subproc = ya_test_subproc

    def send_signal(self, signal_number):
        self.ya_test_subproc.process.send_signal(signal_number)

    def get_pid(self):
        return self.ya_test_subproc.process.pid

    def wait(self, timeout):
        self.ya_test_subproc.wait(timeout=fix_timeout_for_sanitizer(timeout), check_exit_code=False)

    def poll(self):
        return self.ya_test_subproc.process.poll()


def wrap_port_manager(port_manager):
    if isinstance(port_manager, IApphostServantPortManager):
        return port_manager
    else:
        return YaTestApphostServantPortManager(port_manager)


def wrap_exec_obj(exec_obj):
    if isinstance(exec_obj, IApphostServantProcess):
        return exec_obj
    else:
        return YaTestApphostServantProcess(exec_obj)


class LogDump:
    def __init__(self, bin_path, log_path, subprocess_run):
        self.bin_path = bin_path
        self.log_path = log_path
        self._next_log_line = 0
        self._subprocess_run = subprocess_run

    def lines(self, from_beginning=False):
        if from_beginning:
            self._next_evlog_line = 0
        proc = self._subprocess_run([self.bin_path, "-j", self.log_path], stdout=subprocess.PIPE)
        lines = proc.stdout.split("\n")[self._next_evlog_line :]
        for l in lines:
            if not l:
                continue
            self._next_evlog_line += 1
            yield json.loads(l)


class ApphostServant:
    LogDump = LogDump

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, port_manager, env={}, wait_ready_timeout=3):
        self.env = env
        self.wait_ready_timeout = wait_ready_timeout
        self._port_manager = wrap_port_manager(port_manager)
        self._init_members()

    def _init_members(self):
        self._exec = None
        self.http_port = None
        self.grpc_port = None
        self._grpc_channel = None
        self._grpc_stub = None

    def _execute_bin(self):
        # Should be implemented in subclass
        raise NotImplementedError()

    def _after_start(self):
        pass

    def start(self):
        name = f"ApphostServant({self.__class__.__name__})"

        self.http_port = self._port_manager.get_http_port()
        self.grpc_port = self._port_manager.get_grpc_port()

        self._exec = wrap_exec_obj(self._execute_bin())
        logging.info(
            f"{name} started: "
            f"PID={self._exec.get_pid()}, "
            f"http_port={self.http_port}, "
            f"grpc_port={self.grpc_port}, "
            f"env={self.env}"
        )

        self.wait_ready()
        logging.info(f"{name} is ready")

        self._after_start()

    def stop(self):
        assert self._exec.poll() is None, "process died before shutdown"

        try:
            resp = urllib.request.urlopen(self._shutdown_url)
            assert resp.status == 200
            self._exec.wait(timeout=fix_timeout_for_sanitizer(10))
        except:
            try:
                self._exec.send_signal(signal.SIGKILL)
                self._exec.wait(timeout=fix_timeout_for_sanitizer(10))
            finally:
                raise RuntimeError("Graceful shutdown failed")
        finally:
            self._port_manager.release_port(self.http_port)
            self._port_manager.release_port(self.grpc_port)
            self._init_members()

    @property
    def host(self):
        return "localhost"

    def _endpoint(self, port):
        if self._exec is None:
            return None
        return f"{self.host}:{port}"

    def _admin_action_url(self, action):
        return f"http://{self.http_endpoint}/admin?action={action}"

    @property
    def grpc_endpoint(self):
        return self._endpoint(self.grpc_port)

    @property
    def http_endpoint(self):
        return self._endpoint(self.http_port)

    @property
    def _ping_url(self):
        return self._admin_action_url("ping")

    @property
    def _shutdown_url(self):
        return self._admin_action_url("shutdown")

    def wait_ready(self, timeout=None):
        if timeout is None:
            timeout = self.wait_ready_timeout
        deadline = time.monotonic() + fix_timeout_for_sanitizer(timeout)
        while True:
            try:
                resp = urllib.request.urlopen(self._ping_url)
                assert resp.status == 200
                break
            except urllib.error.URLError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.2)
