from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
from alice.cuttlefish.library.python.apphost_grpc_client.ut_protos.example_pb2 import TExampleRequest, TExampleResponse
from .apphost_mock import AppHostMock

import pytest
import time
import asyncio
import logging


req_type = "friendly_request"
resp_type = "friendly_response"
names = ("Friend", "Pal", "Bro")


async def server_coro(apphost, with_params):
    def check_no_params_items(req):
        try:
            params = req.get_json_item("app_host_params")
            assert False, f"No params items expected, but at least one found: {params}"
        except KeyError:
            pass

    try:
        req = await apphost.get_request(timeout=3)
        async with req:
            logging.debug(f"New request (GUID={req.id})")

            is_first_chunk = True
            for n in names:
                if with_params and is_first_chunk:
                    assert req.get_json_item("app_host_params") == {'reqid': b'some_reqid', 'type': b'app_host_params'}
                else:
                    check_no_params_items(req)

                item = req.get_item(req_type, TExampleRequest)
                logging.debug(f"got request: {item}")
                assert item == TExampleRequest(ReqMethod="GET", Values=["Hello", n])

                req.add_item(resp_type, TExampleResponse(RespCode="OK", Values=["Hi", n]))
                req.intermediate_flush()

                assert (await req.next_input()) is True
                is_first_chunk = False

            check_no_params_items(req)
            item = req.get_item(req_type, TExampleRequest)
            logging.debug(f"got request: {item}")
            assert item == TExampleRequest(ReqMethod="POST", Values=["Bye", "Fellow"])
            assert (await req.next_input()) is False  # no more input

            req.add_item(resp_type, TExampleResponse(RespCode="OK", Values=["See", "You", "Fellow"]))
            req.flush()

    except:
        logging.exception("server failed")
        raise


async def client_coro(stream):
    async with stream:
        for n in names:
            stream.write_items({req_type: [TExampleRequest(ReqMethod="GET", Values=["Hello", n])]})

            resp = await stream.read()
            logging.debug(f"got response: {resp}")
            item = resp.get_only_item(item_type=resp_type, proto_type=TExampleResponse)
            assert item.data == TExampleResponse(RespCode="OK", Values=["Hi", n])

            await asyncio.sleep(0.1)

        stream.write_items({req_type: [TExampleRequest(ReqMethod="POST", Values=["Bye", "Fellow"])]}, last=True)

        resp = await stream.read()
        logging.debug(f"got response: {resp}")
        item = resp.get_only_item(item_type=resp_type, proto_type=TExampleResponse)
        assert item.data == TExampleResponse(RespCode="OK", Values=["See", "You", "Fellow"])

        assert (await stream.read()) is None


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
# Check all path formats
@pytest.mark.parametrize(
    "path",
    [
        "/some_path",
        "some_path",
    ],
)
@pytest.mark.parametrize(
    "params",
    [
        None,
        {"reqid": "some_reqid"},
    ],
)
async def test_single_request(path, params):
    async with AppHostMock(path) as ah:
        client = AppHostGrpcClient(ah.grpc_endpoint)

        done, _ = await asyncio.wait(
            [server_coro(ah, params is not None), client_coro(client.create_stream(path=path, params=params))]
        )
        for d in done:
            d.result()


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_timeout():
    path = "/some_path"

    timeout = 0.5
    inf_timeout = 10000

    async def server_coro(apphost):
        req = await apphost.get_request(timeout=inf_timeout)
        async with req:
            logging.debug(f"New request (GUID={req.id})")

            while True:
                item = req.get_item(req_type, TExampleRequest)
                logging.debug(f"got request: {item}")
                if item == TExampleRequest(ReqMethod="GET", Values=["End"]):
                    break

                assert (await req.next_input(timeout=inf_timeout)) is True

            req.add_item(resp_type, TExampleResponse(RespCode="OK", Values=["Ok"]))
            req.flush()

    async def client_coro(stream):
        async with stream:
            # Start request
            stream.write_items({req_type: [TExampleRequest(ReqMethod="GET", Values=["Start"])]})

            for coro in [
                stream.read(timeout=timeout),
                stream.read_all(chunk_timeout=timeout),
                stream.read_all(timeout=timeout),
                stream.read_all(timeout=inf_timeout, chunk_timeout=timeout),
                stream.read_all(timeout=timeout, chunk_timeout=inf_timeout),
            ]:
                try:
                    start_time = time.time()
                    await coro
                except asyncio.TimeoutError:
                    end_time = time.time()
                    time_diff = end_time - start_time
                    logging.debug(f"Got timeout in {time_diff}")
                    assert (
                        time_diff > timeout / 2.0
                    ), f"Request timeout is {timeout}, but execution time is {time_diff}, request cancelled too fast"
                    assert (
                        time_diff < 2 * timeout
                    ), f"Request timeout is {timeout}, but execution time is {time_diff}, request cancellation is too slow"
                except Exception as e:
                    assert False, f"Expected timeout, but {e} found"

            stream.write_items({req_type: [TExampleRequest(ReqMethod="GET", Values=["End"])]})
            resp = await stream.read_all(timeout=timeout)
            logging.debug(f"got response: {resp}")
            assert len(resp) == 1
            item = resp[0].get_only_item(item_type=resp_type, proto_type=TExampleResponse)
            assert item.data == TExampleResponse(RespCode="OK", Values=["Ok"])

    async with AppHostMock(path) as ah:
        client = AppHostGrpcClient(ah.grpc_endpoint)

        done, _ = await asyncio.wait([server_coro(ah), client_coro(client.create_stream(path))])
        for d in done:
            d.result()


# # -------------------------------------------------------------------------------------------------
# @pytest.mark.asyncio
# async def test_multiple_requests():
#     count = 2

#     async with AppHostMock("/some_graph") as ah:
#         client = AppHostGrpcClient(ah.grpc_endpoint)

#         coros = []
#         for _ in range(count):
#             coros += [
#                 server_coro(ah),
#                 client_coro(client.create_stream(path="/some_graph"))
#             ]
#             await asyncio.sleep(1.0)

#         done, _ = await asyncio.wait(coros)
#         for d in done:
#             d.result()


# # -------------------------------------------------------------------------------------------------
# @pytest.skip()  # crash from apphost server
# @pytest.mark.asyncio
# async def test_client_cancels():
#     req_type = "friendly_request"
#     resp_type = "friendly_response"

#     async def server_coro(ah):
#         try:
#             req = await ah.get_request(timeout=3)

#             async with req:
#                 while True:
#                     item = req.get_item(req_type, TExampleRequest)
#                     logging.debug(f"got request: {item}")

#                     req.add_item(resp_type, TExampleResponse(RespCode="OK", Values=["Hi", item.Values[1]]))
#                     req.intermediate_flush()

#                     if (await req.next_input()) is False:
#                         break

#         except:
#             logging.exception("server failed")
#             raise

#     async with AppHostMock("/first", "/second", "/third") as ah:
#         server_task = asyncio.create_task(server_coro(ah))
#         client = AppHostGrpcClient(ah.grpc_endpoint)

#         stream = client.create_stream(path="/first")

#         stream.write_items({req_type: TExampleRequest(ReqMethod="GET", Values=["Hello", "Man"])})
#         stream.write_items({req_type: TExampleRequest(ReqMethod="GET", Values=["Hello", "Bud"])})
#         stream.cancel()

#         with pytest.raises(grpc.RpcError):
#             await stream.read()  # call has been cancelled

#         await server_task
