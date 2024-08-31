import tornado
import tornado.gen


class FakePersonalDataHelper:
    def __init__(self, system, request_info, rt_log, personal_data_response_future=None, need_future=False):
        self.system = system
        self.payload = request_info
        self.rt_log = rt_log
        self.future = None
        if need_future:
            self.future = tornado.concurrent.Future()

    @tornado.gen.coroutine
    def get_personal_data(self, only_settings=False):
        if self.future:
            res = yield self.future
            return res, None
        return {}, None
