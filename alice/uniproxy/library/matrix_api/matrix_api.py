import tornado

from alice.protos.api.matrix.schedule_action_pb2 import TScheduleAction

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, RTLogHTTPRequest
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.notificator_api.metrics import metrics_for
from alice.uniproxy.library.settings import config

from rtlog import null_logger


settings = config["matrix"]


def get_client(url):
    return QueuedHTTPClient.get_client_by_url(
        url,
        pool_size=settings["pool_size"],
        queue_size=10,
        wait_if_queue_is_full=False
    )


class MatrixApi:
    def __init__(self, url=None, rt_log=null_logger(), metrics_backend=None):
        self.url = url or settings["url"]
        self.rt_log = rt_log
        self.metrics_backend = metrics_backend
        self.headers = {
            "Content-Type": "application/protobuf",
        }

    @tornado.gen.coroutine
    def add_schedule_action(self, payload: dict):
        try:
            with metrics_for('matrix_add_schedule_action', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + "/schedule",
                    method="POST",
                    headers=self.headers,
                    body=proto_from_json(TScheduleAction, payload["schedule_action"]).SerializeToString(),
                    request_timeout=settings["request_timeout"],
                    retries=settings["retries"],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True
                )

                yield get_client(self.url).fetch(request)
        except Exception as e:
            Logger.get('.add_schedule_action').error("add_schedule_action error: {}".format(e))
