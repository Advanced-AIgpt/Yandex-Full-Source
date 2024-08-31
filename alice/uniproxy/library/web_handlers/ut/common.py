import tornado.web
import tornado.httpserver
import yatest.common.network


class HttpServer:
    def __init__(self, *handlers):
        self._app = tornado.web.Application(handlers)
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._port_manager = None
        self.address = None
        self.port = None

    @property
    def endpoint(self):
        return f"http://{self.address}:{self.port}"

    def start(self, address="", port_manager=None):
        self._port_manager = port_manager or yatest.common.network.PortManager()
        self.port = self._port_manager.get_port()
        self.address = address or "localhost"
        self._srv.listen(self.port, self.address)

    def stop(self):
        self._srv.stop()
        self._port_manager.release_port(self.port)

    def __enter__(self, *args, **kwargs):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()
