import ast
import os
import tempfile
import shutil
from contextlib import contextmanager

from yalibrary.svn import svn_co


ARCADIA_TRUNK_PATH = 'svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/'
VINS_PKG_ARCADIA_PATH = 'alice/vins/packages'
VINS_PKG_FILE_NAME = 'vins_package.json'


@contextmanager
def temp_dir(dst_dir=None):
    tmp_dir = tempfile.mkdtemp(dir=dst_dir)
    try:
        yield tmp_dir
    finally:
        if os.path.exists(tmp_dir):
            shutil.rmtree(tmp_dir)


def _read_vins_pkg(package_path):
    with open(package_path, 'r') as stream:
        return stream.read()


def _read_local_vins_pkg(arcadia_dir, vins_package_abs_path):
    if vins_package_abs_path:
        return _read_vins_pkg(vins_package_abs_path)
    return _read_vins_pkg(os.path.join(arcadia_dir, VINS_PKG_ARCADIA_PATH,
                                       VINS_PKG_FILE_NAME))


def _checkout_vins_pkg():
    svn_path = ARCADIA_TRUNK_PATH + VINS_PKG_ARCADIA_PATH
    with temp_dir() as dst_path:
        svn_co(src=svn_path, dst=dst_path, revision=None, quiet=True)
        return _read_vins_pkg(os.path.join(dst_path, VINS_PKG_FILE_NAME))


def load(use_local, arcadia_dir, vins_package_abs_path):
    vins_pkg = _read_local_vins_pkg(arcadia_dir, vins_package_abs_path) if use_local else _checkout_vins_pkg()
    return ast.literal_eval(vins_pkg)


def iter_sandbox_resource(vins_pkg):
    for data in vins_pkg['data']:
        if data['source']['type'] == 'SANDBOX_RESOURCE':
            yield data['source']['id'], data['destination']['path']
