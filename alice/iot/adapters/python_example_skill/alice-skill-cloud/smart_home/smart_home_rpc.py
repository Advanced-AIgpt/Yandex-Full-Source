import json
import logging
from typing import Dict, List
from smart_home.provider import api


logging.basicConfig(format="%(levelname)s PID %(process)d TID %(thread)d [%(asctime)s]: %(message)s\n",
                    level=logging.INFO, filename="/smart_home/smart_home_api.log")


HEADERS = "headers"
REQUEST_ID = "request_id"
USER_ID = "user_id"
PAYLOAD = "payload"
DEVICES = "devices"
REQUEST_TYPE = "request_type"


class RequestType:
    discovery = "discovery"
    query = "query"
    action = "action"
    unlink = "unlink"


def make_discovery_payload():
    return {
        USER_ID: "alone_user",
        DEVICES: api.discovery_devices()
    }


def make_query_payload(devices: List):
    return {
        DEVICES: api.query_devices(devices)
    }


def make_action_payload(devices: List):
    return {
        DEVICES: api.action_devices(devices)
    }


def make_unlink_payload():
    return


def make_payload(request):
    if request[REQUEST_TYPE] == RequestType.discovery:
        payload = make_discovery_payload()
    elif request[REQUEST_TYPE] == RequestType.query:
        payload = make_query_payload(request[PAYLOAD][DEVICES])
    elif request[REQUEST_TYPE] == RequestType.action:
        payload = make_action_payload(request[PAYLOAD][DEVICES])
    elif request[REQUEST_TYPE] == RequestType.unlink:
        payload = make_unlink_payload()
    else:
        payload = None
    return payload


def make_response(request_id: str, payload: Dict):
    response = {
        REQUEST_ID: request_id,
        PAYLOAD: payload
    }
    if not response[PAYLOAD]:
        del response[PAYLOAD]

    return json.dumps(response)


def handler(events, context):
    logging.info("Rpc")

    payload = make_payload(events)

    logging.info("Payload: %r", payload)
    response = make_response(events[HEADERS][REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response
