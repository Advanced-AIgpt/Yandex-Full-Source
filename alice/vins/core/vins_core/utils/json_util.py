# coding: utf-8

from __future__ import unicode_literals

import six
from google.protobuf import json_format, struct_pb2

MessageToDict = json_format.MessageToDict

# Monkey patch _INT64_TYPES to change default way of serialization of Int64 and Uint64 proto-types, from strings to int
json_format._INT64_TYPES = []


def _convert_to_value(value, proto):
    if value is None:
        proto.null_value = 0
    elif isinstance(value, bool):
        proto.bool_value = value
    elif isinstance(value, six.integer_types + (float,)):
        proto.number_value = value
    elif isinstance(value, six.string_types):
        proto.string_value = value
    elif isinstance(value, list):
        proto.list_value.CopyFrom(struct_pb2.ListValue())
        _list_to_list_value(value, proto.list_value)
    elif isinstance(value, dict):
        proto.struct_value.CopyFrom(struct_pb2.Struct())
        _dict_to_struct(value, proto.struct_value)
    else:
        raise ValueError("can't convert type %s to Value" % type(value))


def _list_to_list_value(list_, list_value):
    for v in list_:
        _convert_to_value(v, list_value.values.add())


def _dict_to_struct(dict_, struct):
    for k, v in six.iteritems(dict_):
        _convert_to_value(v, struct.fields[k])


def dict_to_struct(dict_):
    struct = struct_pb2.Struct()
    _dict_to_struct(dict_, struct)
    return struct
