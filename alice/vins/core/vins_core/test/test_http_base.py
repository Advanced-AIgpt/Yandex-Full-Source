# coding: utf-8
import logging

import attr
import pytest
import requests
import requests_mock
from vins_core.ext.base import BaseHTTPAPI


@attr.s
class FakeRequest(object):
    method = attr.ib()
    url = attr.ib()


def test_base_http(caplog):
    with caplog.at_level(logging.INFO):
        with requests_mock.mock() as m:
            http = BaseHTTPAPI()

            # url in 'args'
            m.get('http://test.org/1', text='OK')
            http.get('http://test.org/1')
            assert 'GET request to http://test.org/1 returned 200' in caplog.text

            # url in 'kwargs'
            m.get('http://test.org/2', text='OK')
            http.get(url='http://test.org/2')
            assert 'GET request to http://test.org/2 returned 200' in caplog.text

            # url in 'args'
            m.post('http://test.org/3', text='OK')
            http.post('http://test.org/3')
            assert 'POST request to http://test.org/3 returned 200' in caplog.text

            # url in 'kwargs'
            m.post('http://test.org/4', text='OK')
            http.post(url='http://test.org/4')
            assert 'POST request to http://test.org/4 returned 200' in caplog.text

            # url in 'args'
            m.get('http://test.org/5', exc=requests.exceptions.ConnectTimeout(
                'error message',
                request=FakeRequest('GET', 'http://test.org/5')))
            with pytest.raises(requests.exceptions.ConnectTimeout):
                http.get('http://test.org/5')
            assert 'GET request to http://test.org/5 raised error: error message' in caplog.text

            # url in 'kwargs'
            m.get('http://test.org/6', exc=requests.exceptions.ConnectTimeout(
                'error message',
                request=FakeRequest('GET', 'http://test.org/6')))
            with pytest.raises(requests.exceptions.ConnectTimeout):
                http.get(url='http://test.org/6')
            assert 'GET request to http://test.org/6 raised error: error message' in caplog.text
