import tornado.web
from .common import BaseServerMock


class MdsServerMock(BaseServerMock):
    class GetHandler(tornado.web.RequestHandler):
        def initialize(self, server):
            self.server = server

        async def get(self):
            self.write(self.server.get_data(self.request.path))

    def __init__(self, content, port_manager=None):
        self.content = content
        self.requests = 0

        app = tornado.web.Application([
            (r"/.*", self.GetHandler, dict(server=self))
        ])
        self.init(app, port_manager=port_manager)

    def get_data(self, path):
        self.logger.debug(f"Got request '{path}'")

        self.requests += 1
        data = self.content.get(path)
        if data is None:
            raise tornado.web.HTTPError(status_code=404, reason="NotFound")

        return data
