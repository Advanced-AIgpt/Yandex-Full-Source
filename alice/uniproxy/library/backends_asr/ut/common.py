from tornado.queues import Queue
from tornado.concurrent import Future
from tornado.httputil import HTTPHeaders
from alice.uniproxy.library.utils.srcrwr import Srcrwr


class AsrClient:  # just to create ASR stream
    def increment_stats(self, *args, **kw):
        pass

    def __init__(self, asr_stream_type, params={}, uuid=None):
        self.use_balancing_hint = True
        self.uid = None
        self.uuid_ = uuid
        self.uaas_flags = None
        self.uaas_asr_flags = None
        self.hostname = "localhost"
        self.params = {
            "asr_balancer": "localhost",
        }
        self.params.update(params)
        self.session_id = 1
        self.message_id = "bla-bla-bla-message-id"
        self.app_id = 'app_id'
        self.client_ip = 'ip'
        self.srcrwr = Srcrwr(HTTPHeaders({}))
        self.asr_stream = asr_stream_type(
            self.on_asr_stream_result,
            self.on_asr_stream_error,
            self.params,
            self.session_id,
            self.message_id,
            close_callback=self.on_asr_stream_close,
            system=self
        )

        self.results = Queue()
        self.error = Future()
        self.closed = Future()

    def on_asr_stream_result(self, result):
        self.results.put_nowait(result)

    def on_asr_stream_error(self, err):
        self.error.set_result(err)
        self.results.put_nowait(None)

    def on_asr_stream_close(self):
        self.closed.set_result(True)
        self.results.put_nowait(None)

    async def pop_result(self):
        res = await self.results.get()
        self.results.task_done()
        return res

    def uuid(self):
        return self.uuid_
