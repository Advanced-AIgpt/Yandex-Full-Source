import json

from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings


class UnistatHandler(CommonRequestHandler):
    unistat_handler_name = 'unistat'

    def get(self):
        self.set_header('Content-Type', 'application/json;charset=utf-8')
        data = GlobalCounter.get_metrics()
        data.extend(GlobalTimings.get_metrics())
        self.write(json.dumps(data))
