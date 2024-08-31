# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals


def test_simple_execute(execute, inspect):
    execute("a = 5")
    execute("b = a + 99997")

    content = inspect('b')
    assert '100002' in content['data']['text/plain']


def test_arcadia_only_import(execute, inspect):
    execute("import jupytercloud.arcadia_kernel.lib.kernel")

    content = inspect('jupytercloud.arcadia_kernel.lib.kernel.ArcadiaKernelApp')

    assert 'ArcadiaKernelApp' in content['data']['text/plain']


def test_magic(execute):
    content = execute('%yql?')

    data = content['payload'][0]['data']['text/plain']

    assert 'yql/library/python/yql/ipython/magic.py' in data


def test_complete_root_module(complete):
    content = complete('import ')
    matches = content['matches']

    # binary module
    assert '__res' in matches

    # standard arcadia pure-python namespace
    assert 'jupytercloud' in matches

    # real builtin module
    assert 'sys' in matches


def test_complete_import(complete):
    content = complete('import jupytercloud.')
    matches = content['matches']

    assert sorted(matches) == sorted(['jupytercloud.arcadia_kernel', 'jupytercloud.library', 'jupytercloud.nirvana'])

    content = complete('import sys.builtin_module_name')
    matches = content['matches']

    assert matches == []


def test_complete_from_symbol(complete):
    content = complete('from jupytercloud.arcadia_kernel import ')
    matches = content['matches']

    assert matches == ['lib']

    content = complete('from __res import iter_')
    matches = content['matches']

    assert matches == ['iter_keys', 'iter_prefixes', 'iter_py_modules']

    content = complete('from sys import builtin_module_name')
    matches = content['matches']

    assert matches == ['builtin_module_names']


def test_complete_from_module(complete):
    content = complete('from jupytercloud.a')
    matches = content['matches']

    assert matches == ['jupytercloud.arcadia_kernel']
