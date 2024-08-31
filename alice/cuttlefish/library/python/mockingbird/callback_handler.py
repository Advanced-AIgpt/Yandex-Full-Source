import tornado.web
from . import canonical


def create_callback_http_handler(name, callback):
    class Handler(tornado.web.RequestHandler):
        NAME = "callback"

        def initialize(self, logger):
            self.logger = logger

        async def _do_process(self):
            req = canonical.get_canonical_request(self.request)

            resp = {}
            try:
                resp = await callback(req)
            except:
                self.logger.exception(f"{name} failed to handle {req}")
                raise tornado.web.HTTPError(reason="MockingbirdFailure")

            self.set_status(status_code=resp.get("code", 200), reason=resp.get("reason", "OK"))
            for n, v in resp.get("headers", []):
                self.add_header(n, v)
            self.write(resp.get("body", ""))

            self.logger.info(f"REQUEST: {req}\nRESPONSE: {resp}")

        async def get(self, *args, **kwargs):
            await self._do_process()

        async def post(self, *args, **kwargs):
            await self._do_process()

        async def put(self, *args, **kwargs):
            await self._do_process()

        async def delete(self, *args, **kwargs):
            await self._do_process()

        async def head(self, *args, **kwargs):
            await self._do_process()

    return Handler
