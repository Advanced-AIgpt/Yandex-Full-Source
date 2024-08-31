from tornado import web

from jupytercloud.backend.handlers.base import JCAPIHandler, JCPageHandler

from .utils import admin_only


class SolomonMetricsHandler(JCAPIHandler):
    async def get(self):
        self.write(self.app.solomon_dumps())


class HTTPClientsHandler(JCPageHandler):
    @web.authenticated
    @admin_only
    async def get(self):
        html = await self.render_template(
            'httpclients.html',
            http_clients_info=self.app.http_clients_info,
        )

        self.write(html)


default_handlers = [
    ('/api/solomon', SolomonMetricsHandler),
    ('/httpclients', HTTPClientsHandler),
]
