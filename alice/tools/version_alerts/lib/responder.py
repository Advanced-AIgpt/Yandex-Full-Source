import asyncio

from aiohttp import web

from alice.tools.version_alerts.lib.common import make_logger

log = make_logger('Responder')


class Responder:
    def __init__(self):
        self.app = web.Application()
        self.cache = None
        self._timed_out = False
        self.app.add_routes(
            [web.get('/unistat', self.unistat),
             web.delete('/stop', self.stop_collector),
             web.get('/health', self.health),
             web.get('/raw', self.raw)]
        )

    async def stop_collector(self, request):
        if request.remote != '::1':
            log.error(f'Caught STOP request from {request.remote}')
            raise web.HTTPBadRequest
        self.app['data_collector']['task'].cancel()
        await self.app['data_collector']['task']
        return web.Response(text='Request sent')

    async def health(self, request):
        if not self._data_collected():
            raise web.HTTPInternalServerError(text="Collector is not up yet")
        if not self._is_alive():
            log.error(self.app['data_collector']['task'].exception())
            await self.app.shutdown()
            raise web.HTTPInternalServerError(text="Collector is stopped")
        else:
            return web.Response(text="OK")

    async def raw(self, request):
        if request.remote != '::1':
            log.error(f'Caught raw data request from {request.remote}')
            raise web.HTTPBadRequest
        log.info('Raw data request caught')
        data = await asyncio.wait_for(self.app['data_collector']['worker'].raw_data(), timeout=15)
        return web.json_response(data=data)

    async def unistat(self, request):
        try:
            data = await asyncio.wait_for(self.app["data_collector"]['worker'].report(), timeout=15)
        except asyncio.TimeoutError:
            log.error("Unistat timed out! Using cached data")
            self._timed_out = True
            return self.cache
        self.cache = data
        self._timed_out = False
        return web.json_response(data=data)

    def _is_alive(self):
        return not any([
            self._timed_out,
            self.app['data_collector']['task'].done(),
            self.app['data_collector']['task'].cancelled()
        ])

    def _data_collected(self):
        return self.app['data_collector']['worker'].is_started
