import argparse
import json
import sys

import numpy as np


def _parse_args():
    parser = argparse.ArgumentParser(description='Difftool to compare two g2p outputs')
    parser.add_argument('lhs', help='lhs file with werp output')
    parser.add_argument('rhs', help='rhs file with werp output')
    return parser.parse_args()


def ensure_deep_equal(a, b, path='<root>'):
    if isinstance(a, list):
        if not isinstance(b, list):
            raise ValueError(f'The left object is a list, but the right object is not at "{path}"')

        if len(a) != len(b):
            raise ValueError(f'The left object has length {len(a)}, but the right object {len(b)} at "{path}"')

        for i in range(len(a)):
            ensure_deep_equal(a[i], b[i], f"{path}[{i}]")
    elif isinstance(a, dict):
        if not isinstance(b, dict):
            raise ValueError(f'The left object is a dict, but the right object is not at "{path}"')
        for key in a:
            if key not in b:
                raise ValueError(f'The right object doesn\'t have "{key}" child at "{path}"')
            ensure_deep_equal(a[key], b[key], path + "." + key)
        for key in b:
            if key not in a:
                raise ValueError(f'The left object doesn\'t have "{key}" child at "{path}"')

    elif isinstance(a, str):
        if not isinstance(b, str):
            raise ValueError(f'The left object is a str, but the right object is not at "{path}"')
        if a != b:
            raise ValueError(f'"{a}" != "{b}" at "{path}"')

    elif isinstance(a, float) or isinstance(a, int):
        if not (isinstance(b, float) or isinstance(b, int)):
            raise ValueError(f'The left object is a number, but the right object is not at "{path}"')

        if not np.isclose(a, b, rtol=1e-5, atol=1e-4):
            raise ValueError(f'{a} != {b} at "{path}"')
    else:
        raise ValueError(f'Unsupported type {type(a)} at "{path}"')


def main():
    args = _parse_args()
    with open(args.lhs) as fin:
        a = json.load(fin)
    with open(args.rhs) as fin:
        b = json.load(fin)
    if len(a) != len(b):
        raise ValueError(b'Left file has {len(a)} records, but right file has {len(b)}')

    for ea, eb in zip(a, b):
        ensure_deep_equal(a, b)
