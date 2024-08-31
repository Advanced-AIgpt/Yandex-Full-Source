import json
import library.python.codecs as codecs


def compress_item_data(data, codec_id=0):
    if codec_id == 0:
        return b"\0" + data
    if codec_id == 2:
        return codecs.dumps("lz4", data)
    raise RuntimeError(f"Unable to compress data with codec_id={codec_id}")


def decompress_item_data(data):
    codec_id = data[0]
    if codec_id == 0:
        return data[1:]
    if codec_id == 2:
        return codecs.loads("lz4", data[1:])
    raise RuntimeError(f"Unable to unpack data with codec_id={codec_id}")


def extract_protobuf(data, protobuf_type):
    data = decompress_item_data(data)
    if not data.startswith(b"p_"):
        raise RuntimeError(f"There is no protobuf in data starts with {data[:10]}...")
    item = protobuf_type()
    item.ParseFromString(data[2:])
    return item


def extract_json(data):
    data = decompress_item_data(data)
    return json.loads(data)


def pack_protobuf(item, codec_id=0):
    return compress_item_data(b"p_" + item.SerializeToString(), codec_id=codec_id)


def pack_json(item, codec_id=0):
    return compress_item_data(json.dumps(item).encode("utf-8"), codec_id=codec_id)
