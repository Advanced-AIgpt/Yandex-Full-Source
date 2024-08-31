import logging

import yatest.common
import yatest.common.network
import tornado.httpserver


class BaseServerMock:
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def init(self, app, port_manager=None):
        self.pm = port_manager or yatest.common.network.PortManager()
        self.server = tornado.httpserver.HTTPServer(app)
        self.port = None
        self.logger = logging.getLogger(self.__class__.__name__)

    def start(self):
        self.port = self.pm.get_port()
        self.server.listen(self.port, "localhost")
        self.logger.debug(f"is listening on {self.port} port")

    def stop(self):
        try:
            self.server.stop()
        finally:
            self.pm.release_port(self.port)
            self.port = None

    @property
    def url(self):
        if self.port is None:
            return None
        return "http://localhost:{}".format(self.port)
