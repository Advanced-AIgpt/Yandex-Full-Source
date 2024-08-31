import logging
import base64
from contextlib import asynccontextmanager
from alice.cuttlefish.library.python.test_utils import WebSocketWrap
from alice.cuttlefish.library.python.test_utils import messages
from alice.cuttlefish.library.python.mockingbird.canonical import get_canonical_response, make_request_from_canonical
import tornado.httpclient
import tornado.httputil


@asynccontextmanager
async def connection(uniproxy2, uniproxy_mock, cgi=None, headers=None):
    try:
        user_ws_fut = uniproxy2.ws_connect(cgi=cgi, headers=headers)
        uniproxy = WebSocketWrap(await uniproxy_mock.accept(), logger=logging.getLogger("Uniproxy"))
        user = WebSocketWrap(await user_ws_fut, logger=logging.getLogger("User"))
        yield (user, uniproxy)
    finally:
        user.close()
        await uniproxy.read_all(timeout=2)


@asynccontextmanager
async def initialized_connection(uniproxy2, uniproxy_mock, cgi=None, headers=None, payload=None):
    async with connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
        await user.write(messages.SynchronizeState(payload=payload))

        msg = await uniproxy.read(timeout=2)
        msg_id = msg["event"]["header"]["messageId"]
        await uniproxy.write(
            {
                "directive": {
                    "header": {"namespace": "Uniproxy2", "name": "UpdateUserSession", "refMessageId": msg_id},
                    "payload": {"do_not_use_user_logs": False},
                }
            }
        )
        await uniproxy.write(messages.EventProcessorFinishedFor(msg))

        await user.read_some(0.5)  # it can be empty or contains `SynchronizeStateResponse`

        yield (user, uniproxy)


async def http_request(url, request):
    client = tornado.httpclient.AsyncHTTPClient()

    headers = tornado.httputil.HTTPHeaders()
    for name, value in request["headers"]:
        headers.add(name, value)

    try:
        make_request_from_canonical
        resp = await client.fetch(request=make_request_from_canonical(request, url=url), raise_error=False)
        return get_canonical_response(resp)
    except:
        logging.exception(f"HTTP request failed (url={url}, request={request})")
        raise tornado.web.HTTPError(reason="ProxyFailure")


def proto_to_b64(proto):
    return base64.encodebytes(proto.SerializeToString()).strip().decode("ascii")
