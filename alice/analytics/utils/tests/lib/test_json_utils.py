#!/usr/bin/env python
# encoding: utf-8

import os
from alice.analytics.utils.json_utils import json_dumps, get_path_str


def test_simple():
    assert json_dumps({'k1': 'v1'}) == """{
  "k1": "v1"
}"""


def test_multiple_keys():
    assert json_dumps({'a': 1, 'b': 2}) == """{
  "a": 1,
  "b": 2
}"""


def test_unicode():
    assert json_dumps({'k1': 'привет', 'k2': 'Ⴊ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣'}) == """{
  "k1": "привет",
  "k2": "Ⴊ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣"
}"""


def test_unicode_with_files(tmp_path):
    source = {'k1': 'привет', 'k2': 'Ⴊ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣'}

    file_path = os.path.join(str(tmp_path), 'test_dump.json')
    with open(file_path, 'w') as f:
        f.write(json_dumps(source))

    with open(file_path) as f:
        data = f.read()

    assert data == """{
  "k1": "привет",
  "k2": "Ⴊ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣"
}"""


def test_get_path_str():
    assert get_path_str({'a': 1}, 'a') == 1
    assert get_path_str({'nested': {'key': {'inner': 2}}}, 'nested.key.inner') == 2
    assert get_path_str([0, 1, 2, 3], '3') == 3
    assert get_path_str([{}, {'key1': [{}, {}, {'key2': 'value'}]}], '1.key1.2.key2') == 'value'
    assert get_path_str({'a': 1}, 'b') is None
    assert get_path_str({'a': 1}, 'b', 'DEFAULT') == 'DEFAULT'
