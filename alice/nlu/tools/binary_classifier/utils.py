import json
import os
import yt.wrapper as yt
from contextlib import contextmanager


# ==== System ====

def is_ipython():
    try:
        from IPython import get_ipython
        return get_ipython() is not None
    except:
        return False


@contextmanager
def change_working_dir(path):
    old_path=os.getcwd()
    if path:
        os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old_path)


def make_parent_dir(file_path):
    dir_path = os.path.dirname(file_path)
    if dir_path:
        os.makedirs(dir_path, exist_ok=True)


# ==== TXT ====

def load_text(path):
    with open(path) as f:
        return f.read()


def save_text(text, path):
    make_parent_dir(path)
    with open(path, 'w') as f:
        f.write(text)


def load_lines(path):
    lines = []
    with open(path) as f:
        for line in f:
            lines.append(line.rstrip('\r\n'))
    return lines


def save_lines(lines, path):
    make_parent_dir(path)
    with open(path, 'w') as f:
        for line in lines:
            f.write(line + '\n')


# ==== TSV ====

def load_tsv_simple(path):
    header = []
    rows = []
    bad = []
    with open(path) as file:
        for row_index, row in enumerate(file):
            parts = row.rstrip('\r\n').split('\t')
            if not header:
                header = parts
            elif len(parts) == len(header):
                rows.append(parts)
            else:
                bad.append((row_index, row))
    if bad:
        print('Error! %d rows are invalid:' % len(bad))
        for row_index, row in bad[:3]:
            print('  Row %d: ' % row_index, row, end='')
        if len(bad) > 3:
            print('  ...')
        print('  Result row count:', len(rows))
    return header, rows


def _save_tsv_row_simple(row, file):
    normalized = [str(part).replace('\t', ' ').replace('\r', ' ').replace('\n', ' ') for part in row]
    file.write('\t'.join(normalized))
    file.write('\n')


def save_tsv_simple(header, rows, path):
    make_parent_dir(path)
    with open(path, 'w') as file:
        _save_tsv_row_simple(header, file)
        for row in rows:
            _save_tsv_row_simple(row, file)


# ==== JSON ====

def load_json(path):
    with open(path) as f:
        # return yaml.full_load(f)
        return json.load(f)


def format_json(d, indent=2):
    return json.dumps(d, indent=indent, ensure_ascii=False, sort_keys=True, separators=(',', ': '))


def print_dict(d):
    print(format_json(d))


# ==== YT ====

def make_yt_config(proxy='hahn', token=None):
    return {
        'token': token or os.environ.get('YT_TOKEN'),
        'proxy': {
            'url': proxy,
            'content_encoding': 'gzip',
        },
        'read_parallel': {
            'enable': True,
            'max_thread_count': 32,
        },
        'write_parallel': {
            'enable': True,
            'max_thread_count': 32,
        },
    }


def create_yt_client(proxy='hahn', token=None):
    return yt.YtClient(config=make_yt_config(proxy, token))


def is_yt_table_exists(path, yt_client):
    return yt_client.exists(path) and yt_client.get(path + '/@type') == 'table'


# ==== Other ====

def is_power_of_10(x):
    while x > 1:
        x = x / 10
    return x == 1


def crop_with_ellipsis(text, width, ellipsis='...'):
    if len(text) <= width:
        return text
    elif width <= len(ellipsis):
        return text[:width]
    else:
        return text[:width - len(ellipsis)] + ellipsis
