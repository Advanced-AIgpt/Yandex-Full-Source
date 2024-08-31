from apphost.lib.grpc.protos.service_pb2 import TServiceRequest, TServiceResponse
import library.python.codecs as codecs


# From protobuf to serialized
def compress_item_data(data, codec_id=0):
    if codec_id == 0:
        return b'\0' + data
    if codec_id == 2:
        return codecs.dumps('lz4', data)
    raise RuntimeError(f'Unable to compress data with codec_id={codec_id}')


def pack_protobuf(item, codec_id=0):
    return compress_item_data(b'p_' + item.SerializeToString(), codec_id=codec_id)


# From serialized to protobuf
def decompress_item_data(data):
    codec_id = data[0]
    if codec_id == 0:
        return data[1:]
    if codec_id == 2:
        return codecs.loads('lz4', data[1:])
    raise RuntimeError(f'Unable to unpack data with codec_id={codec_id}')


def unpack_protobuf(data, proto_cls):
    data = decompress_item_data(data)
    if not data.startswith(b'p_'):
        raise RuntimeError(f'There is no protobuf in data starts with {data[:10]}...')
    p = proto_cls()
    p.ParseFromString(data[2:])
    return p


class GraphBase:
    def __init__(self):
        self._proto = None

    def add_item(self, item, type='some_type', source_name='INIT'):
        answer = self._proto.Answers.add()
        answer.SourceName = source_name
        answer.Type = type
        answer.Data = pack_protobuf(item)

    def get_items(self, type):
        return [item for item in self.items if item.Type == type]

    def has_item(self, type):
        return len(self.get_items(type)) > 0

    def get_only_item_raw(self, type):
        items = self.get_items(type)
        if len(items) != 1:
            raise RuntimeError(f'there is {len(items)} items of type "{type}"')
        return items[0]

    def get_only_item(self, type, proto_cls):
        item = self.get_only_item_raw(type)
        return unpack_protobuf(item.Data, proto_cls)

    def get_only_item_or_none(self, type, proto_cls):
        if not self.has_item(type):
            return None
        return self.get_only_item(type, proto_cls)

    @property
    def items(self):
        return self._proto.Answers

    @property
    def path(self):
        return self._proto.Path

    @property
    def proto(self):
        return self._proto


class GraphRequest(GraphBase):
    def __init__(self, proto=None):
        super(GraphRequest, self).__init__()
        self._proto = proto or TServiceRequest()


class GraphResponse(GraphBase):
    def __init__(self, req, proto=None):
        super(GraphResponse, self).__init__()
        self._proto = proto or TServiceResponse()
        self._source_name = req.proto.RequestInfo.Location.Name

    def add_item(self, item, type='some_type'):
        super(GraphResponse, self).add_item(item, source_name=self._source_name, type=type)
