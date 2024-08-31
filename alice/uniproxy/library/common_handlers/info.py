from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.utils.hostname import current_hostname


class InfoHandler(CommonRequestHandler):
    unistat_handler_name = 'info'

    def get(self):
        self.set_status(200)
        self.write(current_hostname())
