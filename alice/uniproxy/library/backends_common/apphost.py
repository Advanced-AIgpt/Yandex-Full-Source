import grpc

import tornado.gen
import tornado.concurrent

from .async_grpc_call import AsyncGrpcStreamCall

from alice.uniproxy.library.backends_common.httpstream import AsyncHttpStream
from alice.uniproxy.library.settings import config
from alice.cuttlefish.library.python.apphost_message import Request, Response, ItemFormat  # noqa

import apphost.lib.grpc.protos.service_pb2_grpc as apphost_grpc


# -------------------------------------------------------------------------------------------------
class AppHostHttpClient:
    def __init__(self, url=None):
        self.url = url or config["apphost"]["url"]

    async def fetch(self, request : Request, request_timeout=10):
        fut = tornado.concurrent.Future()

        def on_success(http_reponse):
            data = b"" if (http_reponse.body is None) else http_reponse.body
            fut.set_result(Response(data))

        def on_error(http_err):
            fut.set_exception(http_err)

        AsyncHttpStream(
            url=f"{self.url}/{request.path}",
            on_result=on_success,
            on_error=on_error,
            method="POST",
            body=request.get_raw(),
            request_timeout=request_timeout
        )

        return await fut


# -------------------------------------------------------------------------------------------------
class AppHostGrpcStream(AsyncGrpcStreamCall):
    def __init__(self, stub, path, guid=None, params=None, timeout=None):
        if guid is None:
            guid = Request.make_guid()
        else:
            guid = Request.ensure_guid(guid)

        super().__init__(stub, "InvokeEx", timeout=timeout)
        if path.startswith("/"):
            self._path = path
        else:
            self._path = f"/{path}"
        self._guid = guid

        self._params = params
        self._is_first_chunk = True

    def make_request(self):
        return Request(path=self._path, guid=self._guid)

    def write(self, request, last=False):
        msg = request.protobuf
        msg.Last = last
        super().write(msg)
        if last:
            self.writes_done()

    async def read(self, timeout=None):
        proto_resp = await super().read(timeout=timeout)
        if proto_resp is None:
            return None

        return Response(raw=None, protobuf=proto_resp)

    def write_items(self, items, last=False, **kwargs):
        req = self.make_request()

        for item_type, item_datas in items.items():
            for item_data in item_datas:
                req.add_item(item_type=item_type, data=item_data, **kwargs)

        if self._is_first_chunk:
            if self._params is not None:
                req.add_params(self._params)
            self._is_first_chunk = False

        self.write(req, last=last)


# -------------------------------------------------------------------------------------------------
class AppHostGrpcClient:
    def __init__(self, endpoint=None):
        endpoint = endpoint or config["apphost"]["grpc_url"]

        if isinstance(endpoint, (tuple, list)):
            endpoint = f"{endpoint[0]}:{endpoint[1]}"
        self.channel = grpc.insecure_channel(endpoint)
        self.stub = apphost_grpc.TServantStub(self.channel)

    def create_stream(self, path, **kwargs):
        return AppHostGrpcStream(stub=self.stub, path=path, **kwargs)
