import time
from alice.uniproxy.library.logging import Logger


class AccessLogger(object):
    def __init__(self, handle, system, event_id=None, rt_log=None):
        self.start_time = time.time()
        self.handle = handle
        self.session_id = system.session_id
        self.uuid = system.session_data.get("uuid", "")
        self.client_ip = system.client_ip
        self.hostname = system.hostname
        self.key = system.session_data.get("apiKey", "")
        self.rt_log = rt_log or system.rt_log
        self.resource = "-"
        self.event_id = event_id

    def start(self, event_id=None, resource="-"):
        self.start_time = time.time()
        self.event_id = event_id
        self.resource = resource

    def end(self, code=200, size=0, timing=None):
        if timing is None:
            timing = time.time() - self.start_time
        Logger.access_ex('ACCESSLOG: {} {} "POST /{} 1/1" {} "{}" "{}" "{}" "{}" 0 {} 0 "{}" {}'.format(
            self.hostname,
            self.client_ip,
            self.handle,
            code,
            self.event_id,
            self.uuid,
            self.session_id,
            self.key,
            self.resource,
            timing,
            size), rt_log=self.rt_log)
