import asyncio
import functools
import io
from concurrent.futures import Executor, ThreadPoolExecutor

import arc.api.public.repo_pb2
import arc.api.public.repo_pb2_grpc
import arc.api.public.shared_pb2
import grpc
from traitlets import Instance, Integer, Unicode, default
from traitlets.config import SingletonConfigurable

from .arcadia import BaseVCS, FileIsTooBig, VCSError, WrongNodeType


ARCADIA_TRUNK_PREFIX = 'trunk/arcadia/'


class Arc(SingletonConfigurable, BaseVCS):
    oauth_token = Unicode(config=True)
    arc_api = Unicode('api.arc-vcs.yandex-team.ru:6734', config=True)
    max_receive_message_length = Integer(1024 ** 3, config=True)

    executor = Instance(Executor)
    loop = Instance(asyncio.AbstractEventLoop)
    credentials = Instance(grpc.ChannelCredentials)

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('credentials')
    def _credentials_default(self):
        channel_credentials = grpc.ssl_channel_credentials()
        call_credentials = grpc.access_token_call_credentials(self.oauth_token)
        return grpc.composite_channel_credentials(
            channel_credentials,
            call_credentials,
        )

    @property
    def log_context(self):
        return None

    def _canonize_path(self, path):
        path = str(path)

        if path.startswith(ARCADIA_TRUNK_PREFIX):
            path = path[len(ARCADIA_TRUNK_PREFIX):]

        return path

    async def async_call(self, func, *args, **kwargs):
        @functools.wraps(func)
        def wrapper():
            try:
                return func(*args, **kwargs)
            except grpc.RpcError as e:
                raise VCSError(repr(e)) from e

        return await self.loop.run_in_executor(
            self.executor,
            wrapper,
        )

    def create_channel(self):
        return grpc.secure_channel(
            self.arc_api,
            self.credentials,
            options=[
                ('grpc.max_receive_message_length', self.max_receive_message_length),
            ],
        )

    def _get_node_stat_sync(self, remote_path, rev):
        remote_path = self._canonize_path(remote_path)

        with self.create_channel() as channel:
            file_service = arc.api.public.repo_pb2_grpc.FileServiceStub(channel)

            req = arc.api.public.repo_pb2.StatRequest()
            req.Revision = rev
            req.Path = str(remote_path)

            try:
                resp = file_service.Stat(req)
            except grpc.RpcError as e:
                if e.code() == grpc.StatusCode.NOT_FOUND:
                    return None

                raise

            return {
                'size': resp.Size,
                'type': {
                    arc.api.public.shared_pb2.TreeEntryType.TreeEntryFile: 'file',
                    arc.api.public.shared_pb2.TreeEntryType.TreeEntryDir: 'directory',
                }[resp.Type],
            }

    async def get_node_stat(self, remote_path, rev=None):
        rev = rev or 'trunk'

        return await self.async_call(self._get_node_stat_sync, remote_path, rev)

    async def check_path_exists(self, remote_path, rev=None):
        return await self.get_node_type(remote_path, rev) is not None

    def _read_path_sync(self, remote_path, rev):
        remote_path = self._canonize_path(remote_path)

        buffer = io.BytesIO()
        with self.create_channel() as channel:
            file_service = arc.api.public.repo_pb2_grpc.FileServiceStub(channel)

            req = arc.api.public.repo_pb2.ReadFileRequest()
            req.Revision = rev
            req.Path = str(remote_path)

            resp = file_service.ReadFile(req)
            for chunk in resp:
                buffer.write(chunk.Data)

        return buffer.getvalue()

    async def read_path(self, remote_path, rev=None, *, max_size):
        rev = rev or 'trunk'
        spec = f'{rev}:{remote_path}'

        node_stat = await self.get_node_stat(remote_path, rev)

        if not node_stat:
            raise WrongNodeType(f'trying to read non-existing node {spec}')
        if node_stat['type'] != 'file':
            raise WrongNodeType(f'trying to read non-file node {spec}')
        if node_stat['size'] > max_size:
            raise FileIsTooBig(f'file `{spec}` is too big: {node_stat["size"]} > {max_size}')

        return await self.async_call(self._read_path_sync, remote_path, rev)
