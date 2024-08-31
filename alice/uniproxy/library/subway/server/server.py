import logging

import tornado.ioloop
import tornado.httpserver
import tornado.web
import tornado.gen

from alice.uniproxy.library.common_handlers import PingHandler
from alice.uniproxy.library.common_handlers import UnistatHandler

from alice.uniproxy.library.subway.server.subway import Subway
from alice.uniproxy.library.subway.server.subway import ClientHandler
from alice.uniproxy.library.subway.server.subway import PushHandler
from alice.uniproxy.library.subway.server.subway import PullHandler
from alice.uniproxy.library.subway.server.subway import StatHandler


# ====================================================================================================================
class SubwayServer(object):
    def __init__(self, port=7777, nocache=False):
        super(SubwayServer, self).__init__()
        self._port = port

        self._log = logging.getLogger('subway.server')

        self._sub = Subway(nocache=nocache)

        self._app = tornado.web.Application([
            (r'/client',    ClientHandler, dict(subway=self._sub)),
            (r'/pull',      PullHandler, dict(subway=self._sub)),
            (r'/push',      PushHandler, dict(subway=self._sub)),
            (r'/delivery',  PushHandler, dict(subway=self._sub)),
            (r'/stat',      StatHandler, dict(subway=self._sub)),
            (r'/ping',      PingHandler),
            (r'/unistat',   UnistatHandler)
        ])

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(self._port)
        self._srv.start(1)

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._srv.stop()
