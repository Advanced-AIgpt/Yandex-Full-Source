import tornado.gen
import tornado.httpserver
import tornado.web

import yatest.common.network


class HttpServerBase:
    handlers = None

    def __init__(self):
        self.config = dict()
        self.port = yatest.common.network.PortManager().get_port()

    def __enter__(self):
        self._app = tornado.web.Application([
            (path, handler, self.config)
            for path, handler in self.handlers
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(self.port)
        self._srv.start(1)

        return self

    def __exit__(self, *args, **kwargs):
        self._srv.stop()
