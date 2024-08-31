import errno
import logging
import os
from typing import Union, Optional
import yatest.common as yc

from alice.megamind.mit.library.proto.dummy_pb2 import TDummy

logger = logging.getLogger(__name__)

Json = Union[dict, list, str, int, float, bool, None]


def is_generator_mode():
    return 'MIT_GENERATOR' in yc.context.flags


def create_file_dir_if_not_exist(file_name: str):
    out_dir = os.path.dirname(file_name)
    if not os.path.exists(out_dir):
        try:
            os.makedirs(out_dir)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise


def get_dummy_proto():
    proto = TDummy()
    proto.Dummy = 'dummy'
    return proto


def get_value_by_path(item: Json, path: str, delim: str = '.') -> Optional[Json]:
    for key in path.split(delim):
        if not isinstance(item, dict):
            return None
        item = item.get(key)
        if item is None:
            return None
    return item


def iterate_markers(pytest_request, marker_name):
    for node in pytest_request.node.listchain():
        for marker in node.own_markers:
            if marker.name == marker_name:
                yield marker
