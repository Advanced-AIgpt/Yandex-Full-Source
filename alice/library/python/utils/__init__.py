import re
import logging
import json

import google.protobuf.text_format as text

from . import arcadia
from . import network


__all__ = ['arcadia', 'network']


def to_snake_case(value):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', value).lower()


def to_camel_case(value):
    if '_' not in value:
        return value[0].upper() + value[1:]
    return ''.join([_.capitalize() for _ in value.split('_')])


def iter_files(path, *args):
    dirname = arcadia.arcadia_path(path, *args)
    for filename in dirname.iterdir():
        if filename.name != 'ya.make':
            yield filename


def load_file(filename, proto_obj=None):
    path = arcadia.arcadia_path(filename)
    with path.open('r') as stream:
        if not proto_obj:
            return json.load(stream)
        text.Parse(stream.read(), proto_obj)
    return proto_obj


def write_file(filename, data, to_json=False):
    path = arcadia.arcadia_path(filename)
    logging.info(f'Write file: {path}')
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w') as stream:
        if to_json:
            data_json = data if isinstance(data, dict) else json.loads(data)
            json.dump(data_json, stream, sort_keys=True, indent=4)
            stream.write('\n')
        else:
            stream.write(data)
