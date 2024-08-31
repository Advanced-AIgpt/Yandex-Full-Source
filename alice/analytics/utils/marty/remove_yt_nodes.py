#!/usr/bin/env python
# coding: utf-8

# python remove_yt_nodes.py //home/voice/robot-voice-qa/nirvana 2021-01-10
# python remove_yt_nodes.py //home/voice/robot-voice-qa/nirvana 2021-01-10 modification_time

import sys
import yt.wrapper as yt

def get_tables(folder, attr):
    return yt.get(folder, attributes=['count', attr])

def extract_paths(subtree, attr, cutoff_value, root_path="/"):
    if 'count' in subtree.attributes:  # leaf nodes don't have it
        if not hasattr(subtree, "items"):  # some subtrees don't get scanned at first
            subtree = get_tables(root_path, attr)
        for folder, node in subtree.items():  # recurse for the lower-level subtrees
            yield from extract_paths(node, attr, cutoff_value, f"{root_path}/{folder}")
    if (
            subtree.attributes[attr] <= cutoff_value  # nodes that don't satisfy condition
            or subtree.attributes.get('count', 1) < 1  # empty folders
    ):
        yield root_path

# TODO: rewrite to click
folder = sys.argv[1]
min_date = sys.argv[2].strip()
attr = sys.argv[3] if len(sys.argv) > 3 else 'access_time'
recurse = 'not_recurse' not in sys.argv # do not use without explicitly defined attr argument


with yt.Transaction() as trx:
    subtree = get_tables(folder, attr)

    if recurse:
        tables_to_remove = list(extract_paths(subtree, attr, min_date, folder))
    else:
        tables_to_remove = [f"{folder}/{name}" for name, node in subtree.items() if node.attributes[attr] <= min_date]
    print(f'Selected {len(tables_to_remove)} nodes to drop')

    removed_count = 0
    for table in tables_to_remove:
        try:
            yt.remove(table, force=True)
            print(table, 'DROP TABLE')
            removed_count += 1
        except KeyboardInterrupt:
            break
        except Exception as exception:
            print(table, 'ERROR DROPPING TABLE: ', exception)
            continue

    print(f'Removed {removed_count} tables of {len(tables_to_remove)} planned')
print('Commited on YT')
