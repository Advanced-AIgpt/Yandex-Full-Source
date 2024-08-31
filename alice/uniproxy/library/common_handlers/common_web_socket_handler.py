import tornado.websocket

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger

logger = Logger.get('uniproxy.common_web_socket_handler')


class CommonWebSocketHandler(tornado.websocket.WebSocketHandler):
    """ request handler which support unistat counters
    """
    unistat_handler_name = 'common'

    def prepare(self):
        counter_name = 'handler_{}_reqs_summ'.format(self.unistat_handler_name).upper()
        req_counter = getattr(GlobalCounter, counter_name, None)
        if req_counter is not None:
            req_counter.increment()
        else:
            logger.error('invalid request({}) handler unistat counter: {}'.format(self.request.path, counter_name))
