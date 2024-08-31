import asyncio
import json

from traitlets import Unicode
from traitlets.config import SingletonConfigurable

from .http import AsyncHTTPClientMixin


class JupyterhubClient(SingletonConfigurable, AsyncHTTPClientMixin):
    oauth_token = Unicode(config=True)
    url = Unicode()

    def get_headers(self):
        headers = super().get_headers()
        headers['Authorization'] = f'token {self.oauth_token}'
        return headers

    async def request(self, uri, **kwargs):
        url = self.url + '/' + uri.lstrip('/')

        return await self._raw_request(url=url, **kwargs)

    async def get_users(self):
        response = await self.request(
            uri='users',
            method='GET',
        )

        return json.loads(response.body)

    async def new_user(self, login):
        await self.request(
            uri=f'users/{login}',
            method='POST',
            allow_nonstandard_methods=True,
        )

    async def spawn(self, login, options, timeout=60 * 60):
        await self.request(
            uri=f'users/{login}/server',
            method='POST',
            data=options,
        )

        HEADER_LEN = len(b'data: ')

        message_queue = asyncio.Queue()

        def read_chunk(chunk):
            for line in chunk.splitlines():
                if not line:
                    continue

                substr = line[HEADER_LEN:]

                if not substr:
                    continue

                data = json.loads(substr)
                message_queue.put_nowait(data)

        progress = self.request(
            uri=f'users/{login}/server/progress',
            method='GET',
            streaming_callback=read_chunk,
            request_timeout=timeout,
        )
        progress_task = asyncio.create_task(progress)

        while True:
            get = asyncio.create_task(message_queue.get())
            done, _ = await asyncio.wait(
                [get, progress_task],
                return_when=asyncio.FIRST_COMPLETED,
            )
            if get in done:
                yield get.result()
                while not message_queue.empty():
                    yield message_queue.get_nowait()

            if progress_task in done:
                return
