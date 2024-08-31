# coding: utf-8

import io as std_io

import pytest

from alice.acceptance.modules.request_generator.scrapper.lib import io


def test_write_str():
    bytes_io = std_io.BytesIO()
    io.write('xxx', bytes_io)
    content = bytes_io.getvalue()
    assert content == b'3         xxx'


def test_write_bytes():
    bytes_io = std_io.BytesIO()
    io.write(b'xxxbbbcccddd', bytes_io)
    content = bytes_io.getvalue()
    assert content == b'12        xxxbbbcccddd'


def test_max_length():
    content_length_test_size = 2
    content_length_test_max_size = int('9' * content_length_test_size)
    io.CONTENT_LENGTH_BYTES_SIZE = content_length_test_size
    io.CONTENT_LENGTH_FORMAT = '{:<' + str(content_length_test_size) + 'd}'
    io.CONTENT_MAX_LENGTH = content_length_test_max_size

    bytes_io = std_io.BytesIO()
    content = 'a' * (content_length_test_max_size + 1)
    with pytest.raises(io.IOException):
        io.write(content, bytes_io)
