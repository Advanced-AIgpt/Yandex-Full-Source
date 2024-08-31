from alice.uniproxy.library.settings import config

from alice.uniproxy.library.events.event_exception import EventException
from alice.uniproxy.library.events.go_away import GoAway
from time import monotonic


class Event(object):
    def __init__(self, obj, birth_ts=None, internal=False):
        try:
            header = obj["header"]
            self.name = header["name"]
            self.namespace = header["namespace"]
            self.message_id = header["messageId"]
            self.stream_id = header.get("streamId")
            self.ref_stream_id = header.get("refStreamId")
            self.rtlog_token = header.get("rtLogToken")
            self.payload = obj["payload"] or {}
            check_stream_id(self.stream_id)
            check_ref_stream_id(self.ref_stream_id)
            self.birth_ts = birth_ts or monotonic()
            self.internal = internal  # internal events are not from clients but generated by uniproxy on its own
        except KeyError as exc:
            raise EventException("Event without %s" % (exc,))
        if self.stream_id and self.stream_id > config["max_stream_id"]:
            raise GoAway(self.message_id)

    def event_type(self):
        return self.full_name()

    def full_name(self):
        return f"{self.namespace}.{self.name}".lower()

    @property
    def event_age(self):
        return monotonic() - self.birth_ts

    def event_age_for(self, ts):
        return ts - self.birth_ts

    def create_message(self):
        hdr = {
            "namespace": self.namespace,
            "name": self.name,
            "messageId": self.message_id,
        }
        if self.stream_id is not None:
            hdr["streamId"] = self.stream_id,
        return {"event": {"header": hdr, "payload": self.payload}}

    @staticmethod
    def try_parse(message):
        event_json = message.get("event")
        if event_json:
            return Event(event_json)
        return None


def check_stream_id(stream_id):
    if stream_id is None:
        return
    if not isinstance(stream_id, int) or stream_id <= 0:
        raise EventException("streamId must be positive integer")
    if stream_id % 2 == 0:
        raise EventException("streamId is even, client streams must have odd ids.")


def check_ref_stream_id(ref_stream_id):
    if ref_stream_id is None:
        return
    if not isinstance(ref_stream_id, int) or ref_stream_id <= 0:
        raise EventException("refStreamId must be positive integer")
    if ref_stream_id % 2 == 1:
        raise EventException("refStreamId is odd, server streams must have even ids.")
