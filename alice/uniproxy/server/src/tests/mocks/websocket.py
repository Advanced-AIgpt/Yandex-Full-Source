from unisystem import UniSystem
from tornado.queues import Queue
from tornado.gen import TimeoutError
from alice.uniproxy.library.utils import SrcrwrHeaders
import datetime, json, struct


def _initialize():
    from alice.uniproxy.library.global_state import GlobalState
    try:
        if GlobalState.is_ready():
            return
    except:
        pass
    GlobalState.init()
    GlobalState.set_listening()
    GlobalState.set_ready()


async def _get(queue: Queue, timeout=None):
    if isinstance(timeout, (int, float)):
        timeout = datetime.timedelta(seconds=timeout)
    elif timeout is None:  # without timeout a test may hang forever
        timeout = datetime.timedelta(seconds=3)
    return await queue.get(timeout)


class WebsocketMock:
    def __init__(self, client_ip=None):
        _initialize()
        self.directives = Queue()  # outgoing directives (dicts)
        self.binaries = Queue()  # outgoing binary data

        self.unisystem = UniSystem(self)
        self.unisystem.srcrwr_headers = SrcrwrHeaders({})

        self.client_ip = client_ip or "127.0.0.1"

    def send_data(self, stream_id, data):
        self.unisystem.on_message(struct.pack(">I", stream_id) + data)

    def send_event(self, event: dict):
        """ Pass given 'event' to UniSystem instance
        Also it sets default values into omitted mandatory fields
        """

        header = event.get("header", {})
        payload = event.setdefault("payload", {})
        if header["namespace"] == "System" and header["name"] == "SynchronizeState":
            if "messageId" not in payload:
                header["messageId"] = self.unisystem.next_message_id()
            if "auth_token" not in payload:
                payload["auth_token"] = "developers-simple-key"
            if "uuid" not in payload:
                payload["uuid"] = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
            # disable UaaS
            payload.setdefault("request", {
                "experiments": {"disregard_uaas": True}
            })

        self.unisystem.on_message(json.dumps({"event": event}))

    def write_message(self, msg, binary=False):
        queue = self.binaries if binary else self.directives
        queue.put_nowait(msg)

    async def pop(self, binary=False, timeout=None):
        queue = self.binaries if binary else self.directives
        msg = await _get(queue, timeout)
        queue.task_done()
        return msg

    async def has_some(self, timeout, binary=False):
        try:
            _ = await _get(self.binaries if binary else self.directives, timeout)
            return True
        except TimeoutError:
            return False

    async def pop_directive(self, namespace=None, name=None, timeout=None):
        while True:
            directive = (await self.pop(False, timeout))["directive"]

            header = directive.get("header")
            if header is None:
                continue
            if (namespace is not None) and (header.get("namespace") != namespace):
                continue
            if (name is not None) and (header.get("name") != name):
                continue

            return directive
