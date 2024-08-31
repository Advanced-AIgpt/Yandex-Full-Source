from . import create_grpc_request
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import create_ws_message, get_answer_items, get_answer_item
from apphost.lib.proto_answers.http_pb2 import THttpRequest


def _or(val, default):
    return default if val is None else val


def find_http_request_draft(response, request_type):
    for item in get_answer_items(response, item_type=ItemTypes.HTTP_REQUEST_DRAFT, proto=THttpRequest):
        r_type, r_path = item.Path.split("@")
        if r_type == request_type:
            return item
    return None


def find_header(req, name):
    for h in req.Headers:
        if h.Name.lower() == name.lower():
            return h.Value
    return None


def find_header_json(headers, name):
    for h in headers:
        if len(h) == 2 and h[0].lower() == name.lower():
            return h[1]
    return None


def get_synchronize_state_event(cuttlefish, payload=None, message_id=None):
    event = {
        "event": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeState",
                "messageId": _or(message_id, "00000000-0000-0000-0000-000000000001"),
            },
            "payload": {"uuid": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "auth_token": "some-vaid-app-token"},
        }
    }
    if payload is not None:
        event["event"]["payload"].update(payload)
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.CONVERT_IN,
        request=create_grpc_request(items=[{"type": ItemTypes.WS_MESSAGE, "data": create_ws_message(event)}]),
    )
    return get_answer_item(response, ItemTypes.SYNCRHONIZE_STATE_EVENT)
