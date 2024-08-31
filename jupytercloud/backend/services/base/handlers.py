from tornado import web


class BaseServiceHandler(web.RequestHandler):
    def set_default_headers(self):
        self.set_header('Content-Type', 'application/json')

    @property
    def app(self):
        return self.settings['app']


class RootHandler(BaseServiceHandler):
    def get(self):
        self.write({
            'name': self.app.__class__.__name__,
        })


class SolomonMetricsHandler(BaseServiceHandler):
    async def get(self):
        self.write(self.app.solomon_dumps())


base_handlers = [
    ('/?', RootHandler),
    ('/solomon', SolomonMetricsHandler),
]
