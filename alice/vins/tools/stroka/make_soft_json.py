from __future__ import unicode_literals

import os
import json
import codecs
import argparse

import yt.wrapper as yt

from collections import defaultdict


def make_soft_json(path_from, path_to):
    yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')

    table_path = yt.TablePath(path_from, columns=['key', 'normalized_name'])
    table = yt.read_table(table_path, format=yt.create_format('<encode_utf8=%false>json'))

    soft_json = defaultdict(list)
    for row in table:
        soft_json[row['normalized_name']].append(row['key'])

    with codecs.open(path_to, 'w', encoding='utf8') as fout:
        json.dump(soft_json, fout, ensure_ascii=False, indent=2)


def get_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('--src', help='Path to YT table. Table must have "key", "normalized_name" columns.')
    parser.add_argument('--out', '-o', help='Filename to save final json to.')

    return parser


if __name__ == "__main__":
    parser = get_parser()
    args = parser.parse_args()

    assert args.src, 'You must provide "--src" argument.'
    assert args.out, 'You must provide "--out" argument.'

    make_soft_json(args.src, args.out)
