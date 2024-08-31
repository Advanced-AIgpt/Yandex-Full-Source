import library.python.codecs as codecs
import json
from .constants import ItemFormat, Codecs


_PB_PREFIX = b"p_"


def compress_item_data(data, codec=0):
    prefix = codec.to_bytes(1, "little")
    if codec == Codecs.NULL:
        return prefix + data
    if codec == Codecs.LZ4:
        return prefix + codecs.dumps("lz4", data)

    raise RuntimeError(f"Unable to compress data with codec={codec}")


def decompress_item_data(data):
    codec_id = data[0]
    if codec_id == Codecs.NULL:
        raw = data[1:]
    elif codec_id == Codecs.LZ4:
        raw = codecs.loads("lz4", data[1:])
    else:
        raise RuntimeError(f"Unable to unpack data with codec={codec_id}")

    if raw.startswith(_PB_PREFIX):
        return (ItemFormat.PROTOBUF, raw[len(_PB_PREFIX) :])
    return (ItemFormat.JSON, raw)


def extract_protobuf(data, protobuf_type):
    item_format, data = decompress_item_data(data)
    if item_format != ItemFormat.PROTOBUF:
        raise RuntimeError(f"There is no protobuf in data {data[:10]}...")
    item = protobuf_type()
    item.ParseFromString(data)
    return item


def extract_json(data):
    item_format, data = decompress_item_data(data)
    if item_format != ItemFormat.JSON:
        raise RuntimeError(f"There is no json in data {data[:10]}...")
    return json.loads(data)


def pack_protobuf(item, codec=Codecs.NULL):
    return compress_item_data(_PB_PREFIX + item.SerializeToString(), codec=codec)


def pack_json(item, codec=Codecs.NULL):
    return compress_item_data(json.dumps(item).encode("utf-8"), codec=codec)
