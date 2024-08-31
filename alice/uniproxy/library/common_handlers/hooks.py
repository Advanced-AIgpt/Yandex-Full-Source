from alice.uniproxy.library.global_state import GlobalState

from .internal import InternalHandler


class StopHookHandler(InternalHandler):
    unistat_handler_name = 'stop_hook'

    def get(self):
        if self.is_external_request():
            self.set_status(404, "Not Found")
        else:
            GlobalState.set_stopping()
            self.write("Stopping")


class StartHookHandler(InternalHandler):
    unistat_handler_name = 'start_hook'

    def get(self):
        if self.is_external_request():
            self.set_status(404, "Not Found")
        else:
            GlobalState.set_listening()
            self.write("Starting")
