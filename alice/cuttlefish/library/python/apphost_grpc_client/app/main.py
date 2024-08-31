from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
from alice.cuttlefish.library.python.apphost_grpc_client.ut_protos.example_pb2 import TExampleRequest, TExampleResponse
import asyncio
import logging


async def run():
    endpoint = ("uniproxy2-man-1.man.yp-c.yandex.net", 3001)
    path = "/synchronize_state_2"
    guid = "aaaaaaaa-bbbbbbbb-cccccccc-dddddddd"

    client = AppHostGrpcClient(endpoint)
    async with client.create_stream(path=path, guid=guid) as stream:
        stream.write_items(
            items={
                "ws_message": [TExampleRequest(ReqMethod="GET", Values=["Hello", "Friend"])],
                "session_context": [TExampleRequest(ReqMethod="GET", Values=["Hi", "Mate"])],
            },
            last=True,
        )
        logging.debug("request sent")

        resp = await stream.read()
        logging.debug(f"got response: {resp}")


def main():
    logging.basicConfig(level=logging.DEBUG)
    asyncio.run(run())
