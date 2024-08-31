import json
import pathlib
from urllib.parse import quote

from traitlets import Unicode
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.util.exc import ClientError
from jupytercloud.backend.lib.util.logging import LoggingContextMixin

from .http import AsyncHTTPClientMixin


class CreateDirectoryError(ClientError):
    pass


class JupyterClient(LoggingConfigurable, AsyncHTTPClientMixin, LoggingContextMixin):
    oauth_token = Unicode(config=True)
    proxy_base_url = Unicode(config=True)
    user = Unicode()

    @property
    def log_context(self):
        return {
            'user': self.user,
            'proxy_base_url': self.proxy_base_url,
        }

    def get_headers(self):
        headers = super().get_headers()
        headers['Accept'] = 'application/json'
        headers['Authorization'] = f'token {self.oauth_token}'

        return headers

    async def request(self, uri, **kwargs):
        url = f'{self.proxy_base_url}/user/{self.user}/{uri.lstrip("/")}'

        return await self._raw_request(url=url, **kwargs)

    async def get_file_info(self, path):
        path = quote(str(path))

        response = await self.request(
            uri=f'/api/contents/{path}?content=0',
            method='GET',
            raise_error=False,
        )

        if response.code == 404:
            return None

        response.rethrow()

        return json.loads(response.body)

    async def get_content(self, path):
        path = quote(str(path))

        response = await self.request(
            uri=f'/api/contents/{path}',
            method='GET',
        )

        return json.loads(response.body)

    async def check_path_exists(self, path):
        return await self.get_file_info(path) is not None

    async def write_file(self, path, *, name, type, format, content):
        path = quote(str(path))

        response = await self.request(
            uri=f'/api/contents/{path}',
            method='PUT',
            data=dict(
                name=name,
                path=path,
                type=type,
                format=format,
                content=content,
            ),
        )

        return json.loads(response.body)

    async def _create_directory(self, path):
        path = pathlib.PurePath(path)

        self.log.debug('creating directory %s', path)

        if not path.parts:
            raise ValueError(f'Cannot create dir {path}')

        parent_uri = '/' + str(path.parent) if path.parent.parts else ''

        response = await self.request(
            uri=f'/api/contents/{quote(parent_uri)}',
            method='POST',
            data=dict(
                type='directory',
            ),
        )
        untitled_dir_info = json.loads(response.body)
        untitled_dir_path = untitled_dir_info['path']

        self.log.debug(
            'intermediate directory %s created, going to rename it to %s',
            untitled_dir_path, path,
        )

        await self.request(
            uri=f'/api/contents/{quote(untitled_dir_path)}',
            method='PATCH',
            data=dict(
                path=str(path),
            ),
        )

        self.log.debug('directory %s successfully created', path)

    async def create_directory(self, path):
        target_path = pathlib.PurePath(path)

        self.log.debug('creating path %s', path)

        prefix = pathlib.PurePath('.')

        for part in target_path.parts:
            prefix = prefix / part

            node_info = await self.get_file_info(prefix)

            if not node_info:
                await self._create_directory(prefix)
            elif node_info['type'] == 'file':
                raise CreateDirectoryError(
                    f'cannot create directory {prefix} because path exists as a file',
                )
            else:
                assert node_info['type'] == 'directory', f'unknown node type {node_info["type"]}'

                self.log.debug('path %s already exists', prefix)
