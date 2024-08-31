import json
import os
import pytest

from yatest.common import source_path


def get_arcadia_path(folder, file_name):
    return os.path.join(source_path(folder), file_name)


def load_package(folder, package):
    path = get_arcadia_path(folder, package)
    with open(path, 'r') as f:
        return json.load(f)


@pytest.fixture(scope='function')
def megamind_package():
    return load_package('alice/megamind/deploy/packages', 'megamind_standalone.json')


@pytest.fixture(scope='function')
def vins_package():
    return load_package('alice/vins/packages', 'vins_package.json')


def get_formulas_version(package):
    for item in package.get('data', []):
        if item.get('destination', {}).get('path', '').endswith('/formulas'):
            return item.get('source', {}).get('id')
    raise ValueError('invalid package')


def test_same_formulas(megamind_package, vins_package):
    version = get_formulas_version(megamind_package)
    assert version
    assert version == get_formulas_version(vins_package)
