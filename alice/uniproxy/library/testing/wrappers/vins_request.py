from alice.uniproxy.library.vins.vinsrequest import VinsRequest, VinsApplyRequest
from tornado.concurrent import Future
import tornado.gen


class WrappedVinsRequest(VinsRequest):
    REQUESTS = []

    def __init__(self,  *args, **kwargs):
        super().__init__(*args, **kwargs)
        if self.type == VinsRequest.RequestType.Apply:
            WrappedVinsRequest.APPLY_REQUESTS.append(self)
        else:
            WrappedVinsRequest.REQUESTS.append(self)
        self.start_request_finished = Future()  # need synchronization with start_request coroutine

    @tornado.gen.coroutine
    def start_request_coro(self):
        self.DLOG("WrappedVinsRequest.start_request_coro")
        yield super().start_request_coro()
        self.start_request_finished.set_result(True)


class WrappedVinsApplyRequest(VinsApplyRequest):
    REQUESTS = []

    def __init__(self,  *args, **kwargs):
        super().__init__(*args, **kwargs)
        WrappedVinsApplyRequest.REQUESTS.append(self)
        self.start_request_finished = Future()  # need synchronization with start_request coroutine

    @tornado.gen.coroutine
    def start_request_coro(self):
        yield super().start_request_coro()
        self.start_request_finished.set_result(True)
