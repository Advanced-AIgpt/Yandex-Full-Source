import logging
import threading
from wsgiref import simple_server

import falcon
from alice.library.python.utils.network import wait_port

logger = logging.getLogger(__name__)


class StaticStubber:
    def __init__(self, port, path, static_data_filepath_abs):
        self._stubber_server_thread = StaticStubberThread(port, path, static_data_filepath_abs)

    def __enter__(self):
        self._stubber_server_thread.start_sync()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self._stubber_server_thread.stop_sync()

    @property
    def port(self):
        return self._stubber_server_thread.port


class StaticStubberThread(threading.Thread):
    def __init__(self, port, path, static_data_filepath_abs):
        super(StaticStubberThread, self).__init__()
        self._port = port
        self._path = path
        self._static_data_filepath = static_data_filepath_abs
        self.daemon = True

    def run(self):
        logger.info('Starting StaticStubber thread...')
        try:
            self._start_http_server()
        except Exception as exc:
            logger.exception(exc)
        finally:
            logger.info('StaticStubber thread finished.')

    def start_sync(self):
        self.start()  # non blocking
        wait_port('StaticStubber', self._port)
        logger.info('StaticStubber is ready.')

    def stop_sync(self):
        logger.info('StaticStubber thread is about to stop...')
        self._stop_http_server()
        self.join(300)
        logger.info('StaticStubber thread stopped.')

    def _start_http_server(self):
        app = falcon.API()

        def default_error_handler(ex, req, resp, params):
            self._has_errors = True
            logger.error(f'Exception in stubber {ex!r}, request is {req}')
            raise falcon.HTTPInternalServerError('Exception in stubber', f'{ex!r}')
        app.add_error_handler(Exception, default_error_handler)

        handler = StaticStubberHandler(self._static_data_filepath)
        app.add_route(self._path, handler)

        logger.debug("StaticStubber listens port %s", self._port)
        self._server = simple_server.make_server('', self._port, app)
        self._server.serve_forever()
        logger.debug("StaticStubber finished.")

    def _stop_http_server(self):
        logger.debug("Stopping StaticStubber...")
        self._server.shutdown()
        logger.debug('Stopped StaticStubber')

    @property
    def port(self):
        return self._port


class StaticStubberHandler:
    def __init__(self, static_data_filepath):
        self._static_data_filepath = static_data_filepath
        with open(static_data_filepath, 'r') as in_file:
            self._static_data = in_file.read()

    def on_get(self, req, resp, **kwargs):
        logger.info('on_get')
        self._on_request(req, resp)

    def on_post(self, req, resp, **kwargs):
        logger.info('on_post')
        self._on_request(req, resp)

    def on_put(self, req, resp, **kwargs):
        logger.info('on_put')
        self._on_request(req, resp)

    def _on_request(self, req, resp):
        try:
            self._handle_request_unsafe(req, resp)
        except Exception as exc:
            logger.exception(exc)
            raise
        finally:
            logger.info('Request handled')

    def _handle_request_unsafe(self, req, resp):
        resp.body = self._static_data
