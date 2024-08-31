import os

import yt.wrapper as yt
from tqdm import tqdm


def mkdir_for_file(path):
    directory = os.path.dirname(path)
    if not os.path.exists(directory) and directory != '':
        os.makedirs(directory)


def row_iterator(table, in_parallel, use_tqdm=False):
    yt.config.set_proxy('hahn.yt.yandex.net')

    rows = yt.read_table(table, enable_read_parallel=in_parallel, unordered=in_parallel)
    if use_tqdm:
        rows = tqdm(rows, total=yt.row_count(table))
    return rows
