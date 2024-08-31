from alice.cuttlefish.library.python.apphost_message import Request, Response, ItemFormat
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
import urllib.request
import socket
import logging


CuttlefishNodes = {
    "CONVERT_IN": "raw_to_protobuf",
    "CONVERT_OUT": "protobuf_to_raw",
    "SYNCHRONIZE_STATE_PRE": "synchronize_state-pre",
    "SYNCHRONIZE_STATE_POST": "synchronize_state-post",
    "SYNCHRONIZE_STATE_BLACKBOX_SETDOWN": "synchronize_state-blackbox_setdown",
    "CONTEXT_LOAD_PRE": "context_load-pre",
    "CONTEXT_LOAD_POST": "context_load-post",
    "CONTEXT_LOAD_BLACKBOX_SETDOWN": "context_load-blackbox_setdown",
    "STORE_AUDIO_PRE": "store_audio-pre",
    "STORE_AUDIO_POST": "store_audio-post",
}

ItemTypeToProtobuf = {
    "session_context": TSessionContext,
    "context_load_response": TContextLoadResponse
}


def srcrwr_for_whole_cuttlefish(endpoint):
    srcrwr = {}
    for name, handle in CuttlefishNodes.items():
        srcrwr[name] = f"{endpoint}/{handle}"
    return srcrwr


def run(apphost_url, request):
    http_response = urllib.request.urlopen(urllib.request.Request(
        url=f"{apphost_url}/{request.path}",
        method="POST",
        data=request.get_raw()
    ))
    response = Response(http_response.read())
    logging.debug(f"Raw response protobuf:\n{response.protobuf}")

    n = 1
    for item in response.get_items():
        data = item.data
        if (item.format == ItemFormat.PROTOBUF):
            proto_type = ItemTypeToProtobuf.get(item.type)
            if proto_type is not None:
                data = proto_type()
                data.ParseFromString(item.data)
        print(f"-- ITEM #{n}: source={item.source} type={item.type} data:\n{data}")
        n += 1


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--apphost-url", "-a", help="AppHost URL (like 'http://my.host.ru:3000')", type=str, metavar="ADDR", required=True)
    parser.add_argument("--verbose", "-v", help="Enable debug output", action="store_true")
    parser.add_argument("--debug", "-d",  help="Add 'dbg' AppHost parameter", action="store_true")
    parser.add_argument("--local-cuttlefish-port", "-c", type=int, help="Use localy running cuttlefish binary via srcrwr")
    
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    apphost_params = {}

    if args.local_cuttlefish_port:
        logging.info(f"Use local cuttlefish via srcrwr, gRPC port={args.local_cuttlefish_port}")
        endpoint =  f"{socket.gethostname()}:{args.local_cuttlefish_port}"
        apphost_params["srcrwr"] = srcrwr_for_whole_cuttlefish(endpoint)

    if args.debug:
        apphost_params["dbg"] = True

    # configure your request here
    graph = "context_load"
    items = {
        "session_context": TSessionContext(
            UserInfo=TUserInfo(
                Uuid="some-uuid"
            )
        )
    }

    request = Request(path=graph, items=items)
    if apphost_params:
        request.add_params(apphost_params)
    run(args.apphost_url, request)
