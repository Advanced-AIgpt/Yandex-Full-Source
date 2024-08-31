# coding: utf-8

import logging
import time
from rtlog.thread_local import begin_request, end_request

from alice.vins.api_helper.resources import status2code
from vins_core.utils.metrics import sensors
from vins_core.utils.config import get_bool_setting
from vins_api.common.rtlog_counters import update_counters

logger = logging.getLogger(__name__)
rtlog_generate_token = get_bool_setting('RTLOG_GENERATE_TOKEN')


class MetricsMiddleware(object):
    def process_request(self, req, resp):
        self._start_time = time.time()
        sensors.inc_counter('http_requests', labels={'path': req.path})

    def process_response(self, req, resp, resource, req_succeeded):
        sensors.set_sensor(
            'http_response_time',
            (time.time() - self._start_time) * 1000,
            labels={'path': req.path}
        )

        sensors.inc_counter(
            'http_responses',
            labels={'status_code': status2code(resp.status), 'path': req.path},
        )
        sensors.storage.flush()


class RTLogMiddleware(object):
    def process_request(self, req, resp):
        app_host_reqid = req.get_header('X-AppHost-Request-Reqid')
        app_host_ruid = req.get_header('X-AppHost-Request-Ruid')
        if app_host_reqid and app_host_ruid:
            begin_request(app_host_reqid + '-' + app_host_ruid)
        else:
            token = req.get_header('X-RTLog-Token')
            if token or rtlog_generate_token:
                begin_request(token)

    def process_response(self, req, resp, resource, req_succeeded):
        end_request()
        update_counters()
