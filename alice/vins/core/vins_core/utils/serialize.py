# coding: utf-8
from __future__ import unicode_literals, absolute_import

import struct


def string_to_bytes(string):
    return struct.pack('<i%ds' % len(string), len(string), string)


def bytes_to_string(bytes, offset):
    size, = struct.unpack('<i', bytes[offset:(offset + 4)])
    bytes_read = offset + 4 + size
    return bytes[(offset + 4):bytes_read], bytes_read


def _pack_list_or_string(b, is_list):
    nb = len(b)
    return struct.pack('<ii%ds' % nb, int(is_list), nb, b)


def nested_list_items_to_bytes_with_to_bytes(items, to_bytes):
    """
    Serialize (optionally nested) list of items to contiguous byte array
    :param items:
    :param to_bytes: function taking object, returning its' bytes representation
    :return:
    """
    out = struct.pack('<i', len(items))
    for item in items:
        if isinstance(item, list):
            out += _pack_list_or_string(nested_list_items_to_bytes_with_to_bytes(item, to_bytes), is_list=True)
        else:
            out += _pack_list_or_string(to_bytes(item), is_list=False)
    return out


def nested_list_items_to_bytes(items):
    """
    Serialize (optionally nested) list of items to contiguous byte array
    Each item should implement "to_bytes" method
    :param items:
    :return:
    """
    return nested_list_items_to_bytes_with_to_bytes(items, lambda item: item.to_bytes())


def bytes_to_nested_list_items_with_from_bytes(bytes, obj_from_bytes):
    """
    :param bytes: serialized bytes
    :param obj_from_bytes: function from bytes to objects
    :return:
    """
    num_items, = struct.unpack('<i', bytes[:4])
    offset = 4
    out = []
    for i in xrange(num_items):
        is_list, obj_len = struct.unpack('<ii', bytes[offset:(offset + 8)])
        offset += 8
        object_bytes = bytes[offset:(offset + obj_len)]
        if is_list:
            out.append(bytes_to_nested_list_items_with_from_bytes(object_bytes, obj_from_bytes))
        else:
            out.append(obj_from_bytes(object_bytes))
        offset += obj_len
    return out


def bytes_to_nested_list_items(bytes, cls):
    """
    :param bytes: serialized bytes
    :param cls: class that implement "from_bytes" classmethod
    :return:
    """
    return bytes_to_nested_list_items_with_from_bytes(bytes, cls.from_bytes)
