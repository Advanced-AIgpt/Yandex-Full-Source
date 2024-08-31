from alice.uniproxy.library.common_handlers import CommonWebSocketHandler


class LoggingWebSocket(CommonWebSocketHandler):
    unistat_handler_name = 'logsocket_ws'

    def open(self):
        # disabled because qloud (and RTC, lol)
        return

    def get_compression_options(self):
        return {}

    def on_message(self, message):
        # disabled because qloud
        return

    def on_close(self):
        # disabled because qloud
        return

    def check_origin(self, origin):
        return True
