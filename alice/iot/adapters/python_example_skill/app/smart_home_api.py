from flask import Flask, Response, Request, request, jsonify
import logging
from typing import Dict, List
from app.provider import api


app = Flask(__name__)

logging.basicConfig(format="%(levelname)s PID %(process)d TID %(thread)d [%(asctime)s]: %(message)s\n",
                    level=logging.INFO, filename="/app/app.log")


HEADER_REQUEST_ID = "X-Request-Id"
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


@app.route("/v1.0", methods=["HEAD"])
def check() -> Response:
    logging.info("Checking Endpoint URL")
    return Response()


def make_discovery_payload():
    return {
        USER_ID: "???",
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


def make_payload(request: Request):
    if request.json[REQUEST_TYPE] == RequestType.discovery:
        payload = make_discovery_payload()
    elif request.json[REQUEST_TYPE] == RequestType.query:
        payload = make_query_payload(request.json[DEVICES])
    elif request.json[REQUEST_TYPE] == RequestType.action:
        payload = make_action_payload(request.json[PAYLOAD][DEVICES])
    elif request.json[REQUEST_TYPE] == RequestType.unlink:
        payload = make_unlink_payload()
    else:
        payload = None
    return payload


def make_response(request_id: str, payload: Dict) -> Response:
    response = {
        REQUEST_ID: request_id,
        PAYLOAD: payload
    }
    if not response[PAYLOAD]:
        del response[PAYLOAD]

    return jsonify(response)


@app.route("/v1.0/rpc", methods=["POST"])
def rpc() -> Response:
    logging.info("Rpc")

    payload = make_payload(request)

    logging.info("Payload: %r", payload)
    response = make_response(request.json[HEADERS][HEADER_REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response


@app.route("/v1.0/user/unlink", methods=["POST"])
def unlink() -> Response:
    logging.info("Unlinking account")
    logging.info("Headers: %r", request.headers)

    payload = None

    logging.info("Payload: %r", payload)
    response = make_response(request.headers[HEADER_REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response


@app.route("/v1.0/user/devices", methods=["GET"])
def discovery() -> Response:
    logging.info("Discovering user devices")
    logging.info("Headers: %r", request.headers)

    payload = make_discovery_payload()

    logging.info("Payload: %r", payload)
    response = make_response(request.headers[HEADER_REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response


@app.route("/v1.0/user/devices/query", methods=["POST"])
def query() -> Response:
    logging.info("Querying the states of user devices")
    logging.info("Headers: %r", request.headers)
    logging.info("List of devices: %r", request.json[DEVICES])

    payload = make_query_payload(request.json[DEVICES])

    logging.info("Payload: %r", payload)
    response = make_response(request.headers[HEADER_REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response


@app.route("/v1.0/user/devices/action", methods=["POST"])
def action() -> Response:
    logging.info("Changing device state")
    logging.info("Headers: %r", request.headers)
    logging.info("List of devices and actions: %r", request.json[PAYLOAD][DEVICES])

    payload = make_action_payload(request.json[PAYLOAD][DEVICES])

    logging.info("Payload: %r", payload)
    response = make_response(request.headers[HEADER_REQUEST_ID], payload)
    logging.info("Response: %r", response)
    return response


@app.route("/ui/devices", methods=["GET"])
def ui() -> Response:
    origin = request.headers['Origin'] if 'Origin' in request.headers else '*'

    raw_response = {
        DEVICES: api.ui_devices()
    }

    response = jsonify(raw_response)
    response.headers['Access-Control-Allow-Origin'] = origin
    return response
