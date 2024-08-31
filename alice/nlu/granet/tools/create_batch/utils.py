import os
import yt.wrapper as yt


# ==== System ====

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


# ==== YT ====

def create_yt_client(proxy=None, token=None):
    config = yt.default_config.get_config_from_env()
    config['proxy']['content_encoding'] = 'gzip'
    config['read_parallel']['enable'] = True
    config['read_parallel']['max_thread_count'] = 32
    config['write_parallel']['enable'] = True
    config['write_parallel']['max_thread_count'] = 32
    return yt.YtClient(proxy=proxy, token=token, config=config)


def is_yt_table_exists(path, yt_client):
    return yt_client.exists(path) and yt_client.get_attribute(path, 'type') == 'table'


def get_yt_table_column_names(path, yt_client):
    return [column['name'] for column in yt_client.get_attribute(path, 'schema')]
