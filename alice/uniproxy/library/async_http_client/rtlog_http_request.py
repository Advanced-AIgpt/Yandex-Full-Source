from alice.uniproxy.library.async_http_client.http_request import HTTPRequest
from rtlog import null_logger


# ====================================================================================================================
class RTLogHTTPRequest(HTTPRequest):
    def __init__(self, query, method='GET', headers=None, body=None, request_timeout=None, host=None, retries=1,
                 rt_log=null_logger(), rt_log_label='', need_str=False):
        super().__init__(query, method, headers, body, request_timeout, host, retries)
        self._rt_log = rt_log
        self._rt_log_label = rt_log_label
        self._rt_log_token = ''
        self.need_str = need_str

    # ----------------------------------------------------------------------------------------------------------------
    def prepare(self, host=None):
        if self._rt_log and self._rt_log_label:
            self._rt_log_token = self._rt_log.log_child_activation_started(self._rt_log_label)
            rt_log_token = self._rt_log_token
            if self.need_str and isinstance(rt_log_token, bytes):
                rt_log_token = rt_log_token.decode('ascii')
            self.headers['X-RTLog-Token'] = rt_log_token
        super().prepare(host)

    # ----------------------------------------------------------------------------------------------------------------
    def set_ready(self):
        super().set_ready()
        self._finish_rt_log(True)

    # ----------------------------------------------------------------------------------------------------------------
    def set_timedout(self):
        super().set_timedout()
        self._finish_rt_log(False)

    # ----------------------------------------------------------------------------------------------------------------
    def set_failed(self):
        super().set_failed()
        self._finish_rt_log(False)

    # ----------------------------------------------------------------------------------------------------------------
    def _finish_rt_log(self, ok):
        if self._rt_log_token:
            self._rt_log.log_child_activation_finished(self._rt_log_token, ok)
            self._rt_log_token = ''
