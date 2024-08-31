import abc
import asyncio
import json
import logging
import typing
from collections.abc import AsyncGenerator
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta
from pathlib import Path
from random import shuffle

import more_itertools
import websockets
from asyncio_pool import AioPool
from tenacity import (
    AsyncRetrying, RetryError, TryAgain, before_sleep_log, retry_if_exception,
    retry_if_exception_type, stop_after_attempt, stop_after_delay, wait_random_exponential,
)
from tornado.httpclient import HTTPError
from traitlets import Dict, HasTraits, Instance, Integer, List, Unicode, default
from traitlets.config import LoggingConfigurable, SingletonConfigurable

from jupytercloud.backend.lib.clients.http import AsyncHTTPClientMixin
from jupytercloud.backend.lib.util.exc import JupyterCloudException
from jupytercloud.backend.lib.util.format import pretty_json
from jupytercloud.backend.lib.util.logging import LoggingContextMixin
from jupytercloud.backend.lib.util.misc import Url
from jupytercloud.backend.lib.util.report import ReportMixin


class BaseSaltConnection(abc.ABC, LoggingContextMixin):
    username = Unicode()
    password = Unicode()
    eauth = Unicode()
    request_timeout = Integer()

    @abc.abstractmethod
    async def request(self, path: str, **kwargs) -> typing.Dict[str, typing.Any]:
        pass

    @abc.abstractmethod
    async def listen_events_websocket(self) -> AsyncGenerator[typing.AnyStr, None]:
        pass


class SaltException(JupyterCloudException):
    pass


class SaltAuthException(SaltException):
    pass


class SaltConnection(BaseSaltConnection, LoggingConfigurable, AsyncHTTPClientMixin):
    exceptions_to_retry = AsyncHTTPClientMixin.exceptions_to_retry + (ConnectionRefusedError, )

    url = Url()
    pool_size = Integer(default_value=8)

    @default('_pool')
    def _pool_default(self):
        return AioPool(self.pool_size)

    _auth_state = Dict()
    _valid_auth = Instance(asyncio.Event, ())  # set when auth data is valid (often)
    _need_reauth = Instance(asyncio.Event, ())  # set to signal reauth (rare)
    _pool = Instance(AioPool)

    @property
    def websocket_url(self):
        return self.url.with_scheme('ws').with_path('/ws')

    @property
    def log_context(self):
        return {'url': self.url.human_repr()}

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self._need_reauth.set()

    async def initialize(self):
        return await self._wait_reauth(0)

    async def check_init(self):
        if self._need_reauth.is_set():
            return await self.initialize()

    async def login(self):
        """You don't need to log in manually, but you may."""
        self.log.info('logging in Salt master API')

        data = dict(
            username=self.username,
            password=self.password,
            eauth=self.eauth,
        )

        # raw request, sic!
        url = self.url.with_path('/login')
        response = await self._raw_request(
            str(url),
            data=data,
            request_timeout=self.request_timeout,
        )
        data = json.loads(response.body)

        self._auth_state = data['return'][0]

        # to be sure that our auth expires sooner than salt-api auth
        auth_seconds = (
            datetime.fromtimestamp(self._auth_state['expire']) - datetime.now() - timedelta(hours=1)
        )
        self._need_reauth.clear()
        self._valid_auth.set()

        asyncio.create_task(self._wait_reauth(auth_seconds.total_seconds()))
        # FIXME: Exceptions are NOT HANDLED here

        self.log.info('salt API login successful, reauth in %s', auth_seconds)

    def reauth_now(self):
        """Blocks all further requests from continuing until a new auth state arrives

        Also kicks `_wait_reauth` so it awakens instantly and goes reauthorizing"""
        self.log.info('explicit salt re-login')
        self._valid_auth.clear()
        self._need_reauth.set()

    def _make_login_retrier(self, condition):
        logger = before_sleep_log(self.log, logging.INFO, exc_info=True)

        def _before_sleep(retry_state):
            self.reauth_now()
            logger(retry_state)

        return AsyncRetrying(
            stop=stop_after_attempt(2),
            before_sleep=_before_sleep,
            retry=condition,
            reraise=True,
        )

    async def request(self, path: str = '', **kwargs) -> typing.Dict[str, typing.Any]:
        pool_slot = await self._pool.spawn(self._request(path, **kwargs))
        return await pool_slot

    async def _request(self, path='', **kwargs):
        await self.check_init()
        url = self.url.with_path(path)

        async for attempt in self._make_login_retrier(
            condition=retry_if_exception_type(HTTPError)
            & retry_if_exception(lambda e: e.code == 401),
        ):
            with attempt:
                headers = {'X-Auth-Token': await self._get_auth_token()}
                response = await self._raw_request(
                    url=str(url),
                    headers=headers,
                    request_timeout=self.request_timeout,
                    **kwargs,
                )

        return json.loads(response.body)

    async def listen_events_websocket(self) -> AsyncGenerator[typing.AnyStr, None]:
        await self.check_init()

        async for attempt in self._make_login_retrier(
            condition=retry_if_exception_type(websockets.InvalidHandshake),
        ):
            with attempt:
                try:
                    headers = {'X-Auth-Token': await self._get_auth_token()}
                    # blocks on bad `_valid_auth`
                    async with websockets.connect(
                        str(self.websocket_url),
                        max_size=None,
                        extra_headers=headers,
                    ) as websocket:
                        await websocket.send('websocket client ready')  # magic request
                        assert await websocket.recv() == 'server received message'  # magic response
                        self.log.info('WebSocket connection to Salt API established')

                        while True:
                            message = await websocket.recv()
                            yield message
                except websockets.ConnectionClosedError:
                    self.log.exception('WebSocket connection to Salt API closed unexpectedly')
                    raise
                except websockets.ConnectionClosedOK:
                    self.log.info('WebSocket connection closed')

    async def _wait_reauth(self, lock_time):
        try:
            await asyncio.wait_for(self._need_reauth.wait(), lock_time)
        except asyncio.TimeoutError:
            self.log.info('re-auth because of time')
        else:
            self.log.info('re-auth because of signal')

        self._valid_auth.clear()
        await self.login()

    def _get_headers(self):
        return super().get_headers() | {'Accept': 'application/json'}

    async def _get_auth_token(self):
        """Blocks until the auth token is valid."""
        await self._valid_auth.wait()
        return self._auth_state['token']


class SaltPoolConnection(BaseSaltConnection, LoggingConfigurable, LoggingContextMixin):
    urls = List(Url())
    pool_sizes = Integer(default_value=8)

    connections = List(Instance(SaltConnection))
    _rr = Integer(default_value=0)  # round-robin

    @default('connections')
    def _connections_default(self):
        return [
            SaltConnection(
                url=url,
                username=self.username,
                password=self.password,
                eauth=self.eauth,
                request_timeout=self.request_timeout,
                pool_size=self.pool_sizes,
                parent=self,
            )
            for url in self.urls
        ]

    @property
    def log_context(self):
        return {'urls': [url.human_repr() for url in self.urls]}

    def _get_connection(self):
        self._rr = rr = (self._rr + 1) % len(self.connections)
        return self.connections[rr]

    async def request(self, path='', **kwargs):
        return await self._get_connection().request(path, **kwargs)

    async def listen_events_websocket(self) -> AsyncGenerator[typing.AnyStr, None]:
        conn = self._get_connection()
        websocket_coro = conn.listen_events_websocket()

        async for message in websocket_coro:
            yield message

    async def request_all(self, path='', **kwargs):
        responses = [conn.request(path, **kwargs) for conn in self.connections]
        return await asyncio.gather(*responses)


class SaltClient(SingletonConfigurable, ReportMixin, HasTraits):
    urls = List(Url(), minlen=1, config=True)
    username = Unicode(config=True)
    password = Unicode(config=True)
    eauth = Unicode(config=True)
    request_timeout = Integer(120, config=True)
    log_dir = Instance(Path, config=True, default_value=None, allow_none=True)

    connection = Instance(BaseSaltConnection)

    @default('connection')
    def _connection_default(self):
        kw = dict(
            username=self.username,
            password=self.password,
            eauth=self.eauth,
            request_timeout=self.request_timeout,
        )

        # in case of 1 url, SaltPool degrades to a simple connection + very little overhead
        return SaltPoolConnection(
            urls=self.urls,
            parent=self,
            log=self.log,
            **kw,
        )

    async def wheel(self, fun, arg=None, kwarg=None, **kwargs):
        return await self._cmd(
            client='wheel',
            fun=fun,
            arg=arg,
            kwarg=kwarg,
            **kwargs,
        )

    async def local(
        self,
        tgt,
        fun,
        arg=None,
        kwarg=None,
        tgt_type='glob',
        timeout=None,
        ret=None,
        request_all=False,
    ):
        return await self._cmd(
            client='local',
            fun=fun,
            tgt=tgt,
            arg=arg,
            kwarg=kwarg,
            tgt_type=tgt_type,
            timeout=timeout,
            ret=ret,
            request_all=request_all,
        )

    async def local_async(
        self,
        tgt,
        fun,
        arg=None,
        kwarg=None,
        tgt_type='glob',
        timeout=None,
        ret=None,
        request_all=False,
    ):
        return await self._cmd(
            client='local_async',
            fun=fun,
            tgt=tgt,
            arg=arg,
            kwarg=kwarg,
            tgt_type=tgt_type,
            timeout=timeout,
            ret=ret,
            request_all=request_all,
        )

    async def runner(self, fun, arg=None, kwarg=None, **kwargs):
        return await self._cmd(
            client='runner',
            fun=fun,
            arg=arg,
            kwarg=kwarg,
            **kwargs,
        )

    async def get_job(self, jid):  # TODO: make additional function for getting result from DB?
        return await self.connection.request(f'jobs/{jid}', method='GET')

    async def listen_events_websocket(self) -> AsyncGenerator[typing.AnyStr, None]:
        async for message in self.connection.listen_events_websocket():
            yield message

    async def ping(self, minions: list[str], tgt_type='list', *, timeout=60) -> dict[str, bool]:
        response = await self.local_async(minions, 'test.ping', tgt_type=tgt_type)
        jid = response['return'][0]['jid']
        result = {}
        async for minion, res in self._wait_for_job(
            jid, lambda x: x['return'][0], minions, timeout=timeout, interval=1,
        ):
            if isinstance(res, TimeoutError):
                self.report.info('salt.ping.timeout', str(res), vm=minion)
                result[minion] = False
            else:
                if res:
                    self.report.success('salt.ping.success', vm=minion)
                else:
                    self.report.error('salt.ping.failure', vm=minion)
                result[minion] = res
        return result

    async def restart_minions(self, *, minions, restart_timeout=120) -> list[bool]:
        if not minions:
            self.log.warning('No minions to restart')
            return []

        self.log.info(
            'going to restart %d salt minions: %s',
            len(minions),
            [m.minion_id for m in minions],
        )

        def restart_one(minion):
            try:
                minion.restart(restart_timeout)
            except Exception:
                self.log.debug(
                    'error while restarting salt minion %s', minion.minion_id, exc_info=True,
                )
                return False
            return True

        with ThreadPoolExecutor() as executor:
            result = executor.map(restart_one, minions)

        return list(result)

    async def apply_state(
        self,
        minions,
        state,
        *,
        interval=15,
        max_poll_interval=60,
        timeout=600,
        concurrency=1,
        batch_size=None,
        batch_callback=None,
        failed_state_callback=None
    ):
        """Batch state-applier.

        concurrency: how many batches apply at the same time
        batch_size: number of minions in a batch (default: all minions)
        batch_callback: function `f(batch)` which is called after every batch return,
          Made to show progress in a long-running apply (default: do nothing)
        """

        # TODO: return meaningful errors instead of strings

        if batch_callback is None:
            batch_callback = lambda batch: None

        shuffle(minions)

        pool = AioPool(size=concurrency)
        if batch_size is not None:
            batches = more_itertools.chunked(minions, batch_size)
        else:
            batches = [minions[:]]

        def _parse_result_single(data):
            return data['return'][0]

        def process_minion_result(minion, res):
            successful_states = []
            failed_states = 0

            def fix_state(name):
                return name.replace('_|-', ' -> ')

            def success_state(vm, state_name, state_info):
                successful_states.append(fix_state(state_name))
                #
                # We don't store successful state info, as it's useless for us, but we could!
                # We also print all the successful states together, to not add bloat
                #
                # success_info = state_info.copy()
                # success_info['state_name'] = state_name
                # self.report.success('salt.state.success', pretty_json(success_info), vm=vm)

            def bad_state(vm, state_name, state_info):
                error_info = {'state_name': fix_state(state_name), 'state_info': state_info}
                error_text = pretty_json(error_info)

                if failed_state_callback:
                    failed_state_callback(error_info)

                self.report.error('salt.state.bad', error_text, vm=vm)

            def failed_state(vm, state_name, state_info):
                error_info = state_info.copy()
                error_info['state_name'] = fix_state(state_name)

                comment = state_info.get('comment')
                if not comment or not comment.startswith('One or more requisite failed:'):
                    error_text = pretty_json(error_info)

                    if failed_state_callback:
                        failed_state_callback(error_info)

                    self.report.error('salt.state.failure', error_text, vm=vm)

            if not isinstance(res, dict):
                bad_state(minion, '—', f'result `{res}` is not a dictionary')
                return {minion: 'bad-state'}

            for state_name, state_info in res.items():
                result = state_info.get('result')
                if result is True:
                    success_state(minion, state_name, state_info)
                elif result is False:
                    failed_states += 1
                    failed_state(minion, state_name, state_info)
                else:
                    failed_states += 1
                    bad_state(minion, state_name, state_info)

            self.log.debug(
                'state.apply stats for %s: ' + 'success: %d; ' + 'failed: %d;',
                minion,
                len(successful_states),
                failed_states,
            )

            if failed_states == 0:
                self.report.debug('salt.state.success', successful_states, vm=minion)
                return {minion: 'success'}
            else:
                return {minion: 'failure'}

        async def apply_batch(m: list[str]) -> dict[str, list[str]]:
            response = await self.local_async(
                m,
                'state.apply',
                arg=state,
                kwarg={'output': 'json_out'},
                tgt_type='list',
            )
            try:
                jid = response['return'][0]['jid']
            except KeyError:
                self.log.exception('Unexpected Salt response: %s', response)
                raise

            batch_result = {}
            async for minion, res in self._wait_for_job(
                jid=jid,
                minions=m,
                parser=_parse_result_single,
                timeout=timeout,
                max_poll_interval=max_poll_interval,
                interval=interval,
            ):
                one_result = process_minion_result(minion, res)
                try:
                    batch_result |= one_result
                except ValueError as e:
                    self.log.error('Error appending data, %s, %s, %s', batch_result, one_result, e)

            self.log.debug('Batch result: %s', batch_result)
            return batch_result

        result = {}
        async for batch in pool.itermap(apply_batch, batches, yield_when=asyncio.FIRST_COMPLETED):
            if isinstance(batch, Exception):
                raise batch
            try:
                result |= batch
                batch_callback(batch)
                self.log.info('Total returned: %s minions', len(result))
            except Exception:
                self.log.exception('EXCEPTION in map')
                continue

        return result

    async def _cmd(self, request_all=False, **kwargs):
        state = {k: v for k, v in kwargs.items() if v is not None}
        if request_all:
            return await self.connection.request_all(data=[state])
        else:
            return await self.connection.request(data=[state])

    async def _get_job_info(self, jid, parser=None):
        """Internal function to get result from finished (or not) Salt job.
        Gets parser as an argument because variance in responses is too great.
        """
        if parser is None:
            parser = lambda x: x

        raw_data = await self.get_job(jid)
        result = parser(raw_data)
        return result

    async def _wait_for_job(
        self,
        jid,
        parser,
        minions,
        *,
        timeout=3600,
        interval=10,
        max_poll_interval=600,
    ) -> AsyncGenerator[typing.Any, typing.Any]:
        """Wait for job with exponentially increasing wait times
        Yields done jobs immediately."""

        all_minions = set(minions)
        self.log.debug('Waiting up to %ss for result of job %s, minions: %s', timeout, jid, repr(all_minions)[:1000])
        result = {}

        try:
            async for _attempt in AsyncRetrying(
                retry=retry_if_exception_type(TryAgain),
                wait=wait_random_exponential(
                    multiplier=interval * 2,  # to keep average interval and max
                    exp_base=1.2,
                    max=max_poll_interval * 2,
                ),
                stop=stop_after_delay(timeout),
                before_sleep=before_sleep_log(self.log, logging.DEBUG),
            ):
                with _attempt:
                    attempt = await self._get_job_info(jid, parser)
                    self.log.debug('Got %s responses', len(attempt))

                    if new_minions := set(attempt) - set(result):
                        self.log.debug('%s responses are new', len(new_minions))
                        for m in new_minions:
                            yield m, attempt[m]
                        result |= attempt

                    if set(result) < all_minions:
                        msg = f'{len(result)}/{len(minions)} minions replied'
                        raise TryAgain(msg)
        except RetryError:
            for minion in all_minions - set(result):
                yield minion, TimeoutError(f'Timeout {timeout}s on waiting for Salt job result')
        else:
            self.log.debug('All %d minions jobs returned', len(result))
