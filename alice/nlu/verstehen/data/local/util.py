import json
import os


def read_tsv_columns(path, column_ids=None, skip_first_row=True):
    if column_ids is None:
        column_ids = [0]

    collections = [[] for _ in range(len(column_ids))]
    collections_and_column_ids = zip(collections, column_ids)
    with open(path) as fp:
        for i, line in enumerate(fp):
            if skip_first_row and i == 0:
                continue
            line_items = line.split('\t')
            for collection, column_id in collections_and_column_ids:
                collection.append(line_items[column_id])

    if len(collections) == 1:
        return collections[0]
    return collections


def json_read(path):
    with open(path) as f:
        return json.load(f)


def json_dump(obj, path):
    directory = os.path.dirname(path)
    if not os.path.exists(directory) and directory != '':
        os.makedirs(directory)

    with open(path, 'w') as f:
        json.dump(obj, f)
