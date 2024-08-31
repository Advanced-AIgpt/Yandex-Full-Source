import time
import tornado
from uuid import uuid4

from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.global_counter import Unistat
from rtlog import null_logger


UniproxyCounter.init()
UniproxyTimings.init()


class FakeCache:
    def lookup(self, key):
        return None

    def store(self, key, value):
        pass


class FakeLogger:
    def __init__(self):
        self._logs = []

    def log_directive(self, *args, **kwargs):
        self._logs.append(args)


class FakeCacheManager(object):
    def add_cache(self):
        pass

    def remove_cache(self, cache):
        pass

    def get_cache(self, cache_id):
        return None


class FakeSrcrwr:
    def __init__(self, rewrites=None, ports=None):
        self.ports = ports or {}
        self._rewrites = rewrites or {}

    def __getitem__(self, source):
        return self._rewrites.get(source)


class FakeResponsesStorage:
    def __init__(self):
        self._storages = {}

    def _get_or_create_storage(self, reqid):
        if reqid not in self._storages:
            self._storages[reqid] = {}
        return self._storages[reqid]

    def store(self, reqid, hash, response):
        storage = self._get_or_create_storage(reqid)
        storage[hash] = response

    def load(self, reqid):
        return self._storages.get(reqid, None)


class FakeSystem:
    session_id = 1
    session_data = {"uuid": "25228bce3cf7e8553b1954b38e3c5b7f"}
    client_ip = "127.0.0.1"
    client_port = 0
    hostname = "localhost"
    cache_manager = FakeCacheManager()
    is_quasar = False
    oauth_token = 'oauth_token'
    srcrwr = FakeSrcrwr()
    puid = '12345'
    app_id = 'app_id'
    uuid = lambda _: "25228bce3cf7e8553b1954b38e3c5b7f"

    def __init__(self, *args, **kwargs):
        self._directives = {}
        self._messages = []
        self.rt_log = null_logger()
        self.device_model = None
        self.closed = False
        self.unistat = Unistat(self)
        self.icookie_for_uaas = None
        self.uaas_asr_flags = None
        self.uaas_bio_flags = None
        self.test_ids = []
        self.suspend_future = None
        self.use_balancing_hint = True
        self.use_spotter_glue = False
        self.use_laas = True
        self.use_datasync = True
        self.use_personal_cards = True
        self.ab_asr_topic = False
        self.responses_storage = FakeResponsesStorage()
        self.logger = FakeLogger()
        self.notification_supported = False

    def write_directive(self, directive, processors=None, log_message=True):
        self._directives[directive.name] = directive

    def write_message(self, message, log_message=True):
        self._messages.append(message)

    def DLOG(self, *args, **kwargs):
        pass

    def next_stream_id(self):
        return 2

    def next_message_id(self):
        return str(uuid4())

    def is_feature_supported(self, feature):
        return False

    def is_notification_supported(self):
        return self.notification_supported

    def on_close_event_processor(self, *args, **kwargs):
        pass

    @tornado.gen.coroutine
    def get_bb_user_ticket(self):
        return None


class FakeEvent:
    def __init__(self, namespace, name, message_id="12345", payload=None):
        self.namespace = namespace
        self.name = name
        self.message_id = message_id
        self.payload = payload or {}
        self.birth_ts = time.monotonic()

    def event_type(self):
        return "{}.{}".format(self.namespace, self.name).lower()

    @property
    def event_age(self):
        return time.monotonic() - self.birth_ts

    def event_age_for(self, ts):
        return ts - self.birth_ts
