import grpc
import apphost.lib.grpc.protos.service_pb2_grpc as apphost_grpc
from alice.cuttlefish.library.python.apphost_message import Request, Response
from .async_grpc_call import AsyncGrpcStreamCall


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
    def __init__(self, endpoint):
        if isinstance(endpoint, (tuple, list)):
            endpoint = f"{endpoint[0]}:{endpoint[1]}"
        self.channel = grpc.insecure_channel(endpoint)
        self.stub = apphost_grpc.TServantStub(self.channel)

    def create_stream(self, path, **kwargs):
        return AppHostGrpcStream(stub=self.stub, path=path, **kwargs)
