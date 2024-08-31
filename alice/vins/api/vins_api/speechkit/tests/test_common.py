# coding: utf-8
from vins_core.dm.request import Experiments
from vins_core.dm.request_events import TextInputEvent
from vins_api.speechkit.resources.common import get_req_info


def _event():
    event = TextInputEvent('test')
    return event


def _application_data():
    application_data = {
        "lang": "ru-RU",
        "platform": "iphone",
        "app_id": "ru.yandex.mobile.inhouse",
        "app_version": "500",
        "uuid": "507b3326aa8948b49a9f5f87de715133",
        "os_version": "12.1.4",
        "timestamp": "1565181104",
        "timezone": "Europe/Moscow"
    }
    return application_data


def _req_data():
    req_data = {
        'application': _application_data(),
        'request': {},
        'header': {}
    }
    return req_data


def _experiments():
    experiments = Experiments()
    return experiments


class FakeHttpRequest:
    def __init__(self):
        self.headers = {}

    def add_header(self, key, value):
        self.headers[key] = value

    def get_header(self, key, default=None):
        return self.headers.get(key, default)


def test_req_info_headers_processing():
    http_request = FakeHttpRequest()
    http_request.add_header('x-yandex-proxy-header-aaa', 'bbb')
    http_request.add_header('x-yandex-proxy-header-ccc', 'ddd')

    req_info = get_req_info(_event(), _req_data(), _experiments(), http_request)
    assert req_info.proxy_header['aaa'] == 'bbb'
    assert req_info.proxy_header['ccc'] == 'ddd'
