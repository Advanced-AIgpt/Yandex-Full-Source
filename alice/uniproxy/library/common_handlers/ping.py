from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.common_handlers import CommonRequestHandler


class PingHandler(CommonRequestHandler):
    unistat_handler_name = 'ping'

    def get(self):
        if GlobalState.is_online():
            self.write('pong')
        else:
            self.set_status(503)
            self.write("Backend is down")
