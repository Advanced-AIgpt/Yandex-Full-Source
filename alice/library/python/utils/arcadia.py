import pathlib
import os
import sys


CURRENT_DIR = os.path.dirname(sys.argv[0])


def yatool_location():
    dir = None

    d = CURRENT_DIR
    while d != '/' and not dir:
        d = os.path.abspath(os.path.join(d, os.pardir))
        ya = os.path.join(d, 'ya')
        if os.path.exists(ya):
            dir = d

    return dir


def arcadia_root():
    if hasattr(arcadia_root, 'dir'):
        return arcadia_root.dir

    root_from_env = os.environ.get('ARCADIA_ROOT', None)
    arcadia_root.dir = root_from_env or yatool_location()
    return arcadia_root.dir


def arcadia_path(path, *args):
    root = arcadia_root()
    if root:
        return pathlib.Path(root, path, *[_ for _ in args if _])
