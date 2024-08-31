# coding: utf-8

import logging
import signal

import tornado.httpserver
from tornado import gen
from tornado.web import Application
from tornado.ioloop import IOLoop

from nlu_service.web.urls import urls
from nlu_service import settings


logger = logging.getLogger(__name__)


def stophook_handler_factory(http_server):
    @gen.coroutine
    def shutdown():
        http_server.stop()
        logger.info('Will wait for %s seconds before IOLoop shutdown', settings.API_WAIT_BEFORE_SHUTDOWN)
        yield gen.sleep(settings.API_WAIT_BEFORE_SHUTDOWN)
        logger.info('Shutdown')
        IOLoop.current().stop()

    def get_signal_name(signal_code):
        for name, code in signal.__dict__.iteritems():
            if name.startswith('SIG') and not name.startswith('SIG_') and code == signal_code:
                return name
        return 'UNKNOWN'

    def signal_handler(sig, frame):
        logger.info('Received %s(%s) stophook signal', get_signal_name(sig), sig)
        IOLoop.current().add_callback_from_signal(shutdown)

    return signal_handler


class NerServiceApplication(Application):
    pass


def make_app(autoreload=False):
    return Application(urls, autoreload=autoreload)


def serve(port, autoreload=False):
    application = make_app(autoreload=autoreload)
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.stop()
    http_server.listen(port, '0.0.0.0')
    http_server.listen(port, '::')
    signal.signal(signal.SIGTERM, stophook_handler_factory(http_server))  # QLOUD StopHook
    signal.signal(signal.SIGINT, stophook_handler_factory(http_server))   # Ctrl+C
    logger.info('Starting server at port %d', port)
    IOLoop.current().start()
