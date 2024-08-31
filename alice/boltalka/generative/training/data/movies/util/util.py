import yt.wrapper as yt

from tqdm import tqdm


def yt_read_rows(table, first=None):
    print('Reading from `{}`'.format(table))
    n_rows = yt.row_count(table)
    if first is None:
        n_rows = yt.row_count(table)

        for row in tqdm(yt.read_table(table), total=n_rows):
            yield row
    else:
        n_rows = min(n_rows, first)
        for i, row in enumerate(tqdm(yt.read_table(table), total=n_rows)):
            if i < first:
                yield row
            else:
                return


def yt_write_table(films, path):
    yt.write_table(yt.TablePath(path), films)
