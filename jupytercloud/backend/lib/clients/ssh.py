import contextlib
import io
import socket

import asyncssh
import paramiko
from traitlets import Dict, Instance, Integer, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.util.report import ReportMixin


class SSHClient(LoggingConfigurable, ReportMixin):
    id_rsa = Unicode(config=True)

    host = Unicode()
    client = Instance(paramiko.SSHClient, (), allow_none=True)
    connect_timeout = Integer(default_value=120)
    connect_kwargs = Dict()

    @default('connect_kwargs')
    def _connect_kwargs_default(self):
        return dict(
            hostname=self.host,
            username='root',
            look_for_keys=False,
            allow_agent=False,
            pkey=paramiko.RSAKey(file_obj=io.StringIO(self.id_rsa)),
            timeout=self.connect_timeout,
        )

    def __enter__(self):
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        self.log.debug('trying to connect to %s', self.host)

        try:
            self.client.connect(**self.connect_kwargs)
        except OSError as e:
            self.report.error(
                'ssh-execution.connection-problem',
                str(e),
                vm=self.host,
            )
            raise ConnectionError from e
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        try:
            self.client.close()
        finally:
            self.client = None

    def execute(self, command, timeout, command_kwargs=None, log_output=True):
        command_kwargs = command_kwargs or {}
        command = command.format(**command_kwargs)

        self.log.debug('running "%s" on remote server', command)

        try:
            _, stdout, stderr = self.client.exec_command(command, timeout=timeout)

            # TODO: Fix a potential problem with buffer overflow
            out = stdout.read()
            err = stderr.read()
            exit_status = stdout.channel.recv_exit_status()
        except OSError as e:
            self.report.error(
                'ssh-execution.connection-problem',
                str(e),
                vm=self.host,
            )
            raise ExecutionError from e

        if exit_status:
            if log_output:
                self.log.error(
                    'command `%s` returned non-zero exit status %d on host %s\n'
                    'command stdout: %s\n'
                    'command stderr: %s\n',
                    command,
                    exit_status,
                    self.host,
                    out,
                    err,
                )
            else:
                self.log.error(
                    'command `%s` returned non-zero exit status %d on host %s',
                    command,
                    exit_status,
                    self.host,
                )
            self.report.error(
                'ssh-execution.command-failed',
                f'Stdout:\n{stdout}\n\nStderr:\n{stderr}',
                vm=self.host,
            )
        elif err:
            if log_output:
                self.log.warning(
                    'command %s has non-empty stderr on host %s:\n%s', command, self.host, err,
                )

        return ExecutionResult(
            command=command, host=self.host, stdout=out, stderr=err, exit_code=exit_status,
        )

    @contextlib.contextmanager
    def open_sftp(self):
        sftp = self._client.open_sftp()
        try:
            yield sftp
        finally:
            sftp.close()


class AsyncSSHClient(LoggingConfigurable, ReportMixin):
    id_rsa = Unicode(config=True)
    host = Unicode()

    _key = None
    _connection = None

    @property
    def key(self):
        if self._key is None:
            self._key = asyncssh.import_private_key(self.id_rsa)

        return self._key

    async def __aenter__(self):
        self.log.debug('connecting to %s via ssh', self.host)

        self._connection = await asyncssh.connect(
            host=self.host,
            client_keys=[self.key],
            family=socket.AF_INET6,
            known_hosts=None,
            username='root'
        )
        return self._connection

    async def __aexit__(self, exc_type, exc, tb):
        await self._connection.__aexit__(exc_type, exc, tb)


class ExecutionResult:
    def __init__(self, command, host, stdout, stderr, exit_code):
        self.command = command
        self.host = host
        self.stdout = self._canonize_output(stdout)
        self.stderr = self._canonize_output(stderr)

        assert isinstance(exit_code, int)
        self.exit_code = exit_code

    def _canonize_output(self, output):
        output = output or b''
        if isinstance(output, str):
            output = output.encode('utf-8')

        return output.strip()

    def __nonzero__(self):
        return True if self.exit_code == 0 else False

    def raise_for_status(self):
        if self.exit_code:
            raise ExecutionFailure(self)


class SSHError(RuntimeError):
    pass


class ConnectionError(SSHError):
    pass


class ExecutionError(SSHError):
    pass


class ExecutionFailure(SSHError):
    def __init__(self, execute_result):
        self.execute_result = execute_result

    def __str__(self):
        return 'Command `{}` on host {} failed with exit_code {}.\nStdout: {}\nStderr: {}'.format(
            self.execute_result.command,
            self.execute_result.host,
            self.execute_result.exit_code,
            self.execute_result.stdout,
            self.execute_result.stderr,
        )
