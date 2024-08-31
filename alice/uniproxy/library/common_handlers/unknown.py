from alice.uniproxy.library.common_handlers import CommonRequestHandler

from alice.uniproxy.library.logging import Logger

logger = Logger.get('uniproxy.unknown_handler')


class UnknownHandler(CommonRequestHandler):
    unistat_handler_name = 'unknown'

    def get(self):
        self.set_status(404)
        self.write("404: Not found")
        logger.warn('GET request to unsupported handler={}'.format(self.request.path))

    def post(self):
        self.set_status(404)
        self.write("404: Not found")
        logger.warn('POST request to unsupported handler={}'.format(self.request.path))
