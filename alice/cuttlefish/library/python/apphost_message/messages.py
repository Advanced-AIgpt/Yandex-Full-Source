from apphost.lib.grpc.protos.service_pb2 import TServiceRequest, TServiceResponse
from .packing import pack_json, pack_protobuf, decompress_item_data
from .constants import ItemFormat, Codecs
from collections import namedtuple
import json
import uuid


Item = namedtuple("Item", ["format", "type", "source", "data"])


def get_items(answers, item_type=None, source_name=None, item_format=None, proto_type=None):
    for item in answers:
        if (item_type is not None) and (item.Type != item_type):
            continue
        if (source_name is not None) and (item.SourceName != source_name):
            continue

        fmt, raw = decompress_item_data(item.Data)
        if (item_format is not None) and (fmt != item_format):
            continue

        if fmt == ItemFormat.JSON:
            data = json.loads(raw)
        elif fmt == ItemFormat.PROTOBUF:
            if proto_type is not None:
                data = proto_type()
                data.ParseFromString(raw)
            else:
                data = raw

        yield Item(format=fmt, type=item.Type, source=item.SourceName, data=data)


# -------------------------------------------------------------------------------------------------
class Request:
    @staticmethod
    def make_guid():
        guid = uuid.uuid4().hex
        return f"{guid[:8]}-{guid[8:16]}-{guid[16:24]}-{guid[24:]}"  # the only format acceptable by AppHost

    @staticmethod
    def ensure_guid(guid):
        guid = guid.replace("-", "")
        return f"{guid[:8]}-{guid[8:16]}-{guid[16:24]}-{guid[24:]}"  # the only format acceptable by AppHost

    def __init__(self, path, items={}, guid=None):
        if guid is None:
            guid = self.make_guid()

        self._sr = TServiceRequest()
        self._sr.Path = path
        self._sr.Guid = guid

        for item_type, data in items.items():
            self.add_item(item_type=item_type, data=data)

    def __str__(self):
        return f"AppHostRequest(GUID={self.guid}, path={self.path}, {len(self._sr.Answers)} items)"

    @property
    def protobuf(self):
        return self._sr

    @property
    def path(self):
        return self._sr.Path

    @property
    def guid(self):
        return self._sr.Guid

    def get_items(self, **kwargs):
        for i in get_items(self._sr.Answers, **kwargs):
            yield i

    def add_params(self, params, codec=Codecs.LZ4):
        self.add_item(item_type="app_host_params", source_name="APP_HOST_PARAMS", data=params, codec=codec)

    def add_item(self, item_type, data, source_name="INIT", codec=Codecs.LZ4):
        if isinstance(data, list):
            for part in data:
                self.add_item(item_type, part, source_name, codec)
        else:
            answer = self._sr.Answers.add()
            answer.SourceName = source_name
            answer.Type = item_type
            if isinstance(data, dict):
                answer.Data = pack_json(data, codec)
            else:
                answer.Data = pack_protobuf(data, codec)

    def get_raw(self):
        return self._sr.SerializeToString()


# -------------------------------------------------------------------------------------------------
class Response:
    def __init__(self, raw, protobuf=None):
        if protobuf is None:
            protobuf = TServiceResponse()
            protobuf.ParseFromString(raw)
        elif raw is not None:
            raise RuntimeError("invalid arguments")

        self._sr = protobuf

    def __str__(self):
        return f"AppHostResponse(GUID={self.guid}, {len(self._sr.Answers)} items)"

    @property
    def protobuf(self):
        return self._sr

    @property
    def guid(self):
        return self._sr.RequestId

    def has_exception(self):
        return len(self._sr.Exception) > 0

    def get_exception(self):
        return self._sr.Exception

    def get_flags(self):
        return self._sr.Flags

    def get_items(self, **kwargs):
        for i in get_items(self._sr.Answers, **kwargs):
            yield i

    def get_only_item(self, **kwargs):
        res = None
        for item in self.get_items(**kwargs):
            if res is not None:
                raise RuntimeError("more than one item")
            res = item
        if res is None:
            raise RuntimeError("no one item")
        return res

    def get_item_datas(self, **kwargs):
        for i in self.get_items(**kwargs):
            yield i.data

    def get_only_item_data(self, **kwargs):
        return self.get_only_item(**kwargs).data
