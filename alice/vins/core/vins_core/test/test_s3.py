# coding: utf-8
from __future__ import unicode_literals

import pytest
import tempfile

from StringIO import StringIO
from moto import mock_s3
from vins_core.ext.s3 import S3Bucket


@mock_s3
@pytest.mark.parametrize(
    'test_key,test_data',
    [
        ('test1.txt', 'qqq'),
        ('test2.txt', '123' * 100),
        ('test3.txt', 'привет'.encode('utf-8')),
    ]
)
def test_s3(test_key, test_data):
    s3 = S3Bucket('-', '-')
    s3.put(test_key, test_data)
    assert s3.get(test_key) == test_data
    sio = StringIO()
    s3.download_fileobj(test_key, sio)
    assert sio.getvalue() == test_data
    with tempfile.NamedTemporaryFile() as temp_file:
        s3.download_file(test_key, temp_file.name)
        with open(temp_file.name) as f:
            assert f.read() == test_data


def test_s3_url():
    s3 = S3Bucket('-', '-', 'http://s3.mds.yandex.net/')
    assert s3.get_url('test.txt') == 'http://vins-bucket.s3.mds.yandex.net/test.txt'
