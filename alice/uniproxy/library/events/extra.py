from alice.uniproxy.library.events.event_exception import EventException
import base64
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse


# Messages of this type is intended to receive AppHost-made protobufs from uniproxy2
# Common way is to put base64-encoded protobuf message into `payload/response`
class ExtraData(object):

    # Exact type of protobuf should be derivable from `ext_type` of wrapping ExtraData
    PROTOTYPES = {
        "contextloadapply": TContextLoadResponse
    }

    def __init__(self, obj, is_client):
        try:
            header = obj["header"]

            # used to construct target processor's ID
            self.namespace = header["namespace"].lower()
            self.name = header["name"].lower()
            self.init_message_id = header["initMessageId"].lower()

            # used to determine what type of protobuf is inside
            self.ext = header["extType"].lower()

            self.payload = obj.get("payload", {})
        except KeyError as exc:
            raise EventException(f"ExtraData without {exc}")

    def __repr__(self):
        return f"ExtraData(proc_id={self.proc_id}, ext={self.ext})"

    # ID of target event processor
    @property
    def proc_id(self):
        return f"{self.namespace}.{self.name}-{self.init_message_id}"

    def get_response_protobuf(self):
        response_b64 = self.payload.get("response")
        if response_b64 is None:
            return None

        proto_type = self.PROTOTYPES.get(self.ext)
        if proto_type is None:
            return None

        response = proto_type()
        response.ParseFromString(base64.b64decode(response_b64))
        return response

    @staticmethod
    def try_parse(message):
        extra_json = message.get("extra")
        if extra_json:
            return ExtraData(extra_json, True)
        return None
