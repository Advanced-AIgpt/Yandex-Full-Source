import json

from rtlog import null_logger, begin_request

from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json, proto_to_json
from alice.uniproxy.library.logging import Logger


class NotificatorCommonRequestHandler(CommonRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._logger = Logger.get('.' + self.unistat_handler_name)

    def prepare(self):
        super().prepare()
        self._user_ticket = self.request.headers.get('X-Ya-User-Ticket')
        self._service_ticket = self.request.headers.get('X-Ya-Service-Ticket')
        self._content_type = self.request.headers.get('Content-Type', "application/protobuf")
        self._json_api = (self._content_type == 'application/json')
        self.init_rtlog()

    def init_rtlog(self):
        self._rt_log_token = self.request.headers.get('X-RTLog-Token')

        if self._rt_log_token is None:
            # Try to get token from apphost headers
            apphost_request_reqid = self.request.headers.get('X-AppHost-Request-Reqid')
            apphost_request_ruid = self.request.headers.get('X-AppHost-Request-Ruid')

            if apphost_request_reqid is not None and apphost_request_ruid is not None:
                self._rt_log_token = f"{apphost_request_reqid}-{apphost_request_ruid}"

        if self._rt_log_token:
            self._rt_log = begin_request(token=self._rt_log_token, session=False)
        else:
            self._rt_log = null_logger()

    def get_proto_request(self, proto):
        request = None
        if self._json_api:
            request = proto_from_json(proto, json.loads(self.request.body.decode()))
        else:
            request = proto()
            request.ParseFromString(self.request.body)

        self.INFO(request)
        return request

    def send_response(self, resp):
        jresp = proto_to_json(resp)
        self.INFO(jresp)
        self.set_status(200)
        if self._json_api:
            self.finish(jresp)
        else:
            self.finish(resp.SerializeToString())

    def on_finish(self):
        super().on_finish()
        self._rt_log.end_request()

    def DEBUG(self, *args, rt_log=None):
        self._logger.debug(*args, rt_log=rt_log or self._rt_log)

    def INFO(self, *args, rt_log=None):
        self._logger.info(*args, rt_log=rt_log or self._rt_log)

    def WARN(self, *args, rt_log=None):
        self._logger.warning(*args, rt_log=rt_log or self._rt_log)

    def ERROR(self, *args, rt_log=None):
        self._logger.error(*args, rt_log=rt_log or self._rt_log)

    def EXC(self, exception, rt_log=None):
        self._logger.exception(exception, rt_log=rt_log or self._rt_log)
