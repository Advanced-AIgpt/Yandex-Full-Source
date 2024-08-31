# coding:utf-8
from __future__ import unicode_literals

import pytest

from google.protobuf import struct_pb2
from vins_core.utils.json_util import dict_to_struct


def __struct1():
    struct = struct_pb2.Struct()
    lst = struct.get_or_create_list('list')
    lst.add_struct().CopyFrom(struct_pb2.Struct())
    lst.add_struct()['sub_key'] = 'привет'
    return struct


def __struct2():
    struct = struct_pb2.Struct()
    sub_struct = struct.get_or_create_struct('sub_struct')
    sub_struct['int'] = 123e24
    sub_struct['float'] = 1.234
    sub_struct['bool'] = True
    return struct


@pytest.mark.parametrize(
    'dict_, expected',
    [
        ({}, struct_pb2.Struct()),
        ({'list': [{}, {'sub_key': 'привет'}]}, __struct1()),
        ({'sub_struct': {'int': 123e24, 'float': 1.234, 'bool': True}}, __struct2()),
    ]
)
def test_dict_to_struct(dict_, expected):
    assert dict_to_struct(dict_) == expected
