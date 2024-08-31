from tornado.httputil import HTTPHeaders, HTTPServerRequest
from tornado.websocket import WebSocketHandler

from alice.uniproxy.library.utils.srcrwr import Srcrwr


def test_zero_srcrwr():
    h = HTTPHeaders()
    s = Srcrwr(h)
    assert s.header == {}


def test_one_srcrwr():
    h = HTTPHeaders()
    h.add(Srcrwr.NAME, 'BASS=http://bass.yandex.ru')
    s = Srcrwr(h)
    assert s.header == {Srcrwr.NAME: 'BASS=http://bass.yandex.ru'}
    assert s['BASS'] == 'http://bass.yandex.ru'
    assert s['VINS'] is None


def test_several_srcrwrs():
    h = HTTPHeaders()
    h.add(Srcrwr.NAME, 'BASS=http://bass.yandex.ru;VINS=https://vins.yandex.ru')
    s = Srcrwr(h)
    assert s.header == {Srcrwr.NAME: 'BASS=http://bass.yandex.ru;VINS=https://vins.yandex.ru'}
    assert s['BASS'] == 'http://bass.yandex.ru'
    assert s['VINS'] == 'https://vins.yandex.ru'
    assert s['YALDI'] is None


def test_several_srcrwr_headers():
    h = HTTPHeaders()
    h.add(Srcrwr.NAME, 'BASS=http://bass.yandex.ru;VINS=https://vins.yandex.ru')
    h.add(Srcrwr.NAME, 'YALDI=http://yaldi.yandex.net')
    s = Srcrwr(h)
    assert s.header == {
        Srcrwr.NAME: 'BASS=http://bass.yandex.ru;VINS=https://vins.yandex.ru;YALDI=http://yaldi.yandex.net',
    }
    assert s['BASS'] == 'http://bass.yandex.ru'
    assert s['VINS'] == 'https://vins.yandex.ru'
    assert s['YALDI'] == 'http://yaldi.yandex.net'
    assert s['YABIO'] is None


def test_smarthome_srcrwr_header():
    h = HTTPHeaders()
    h.add(Srcrwr.NAME, 'IoT=http://sm.fake.yandex.net')
    s = Srcrwr(h)
    assert s.header == {
        Srcrwr.NAME: 'IoT=http://sm.fake.yandex.net',
    }
    assert s['IoT'] == 'http://sm.fake.yandex.net'


# monkeypatched WebSocketHandler to reach .get_query_arguments method without application initialisation
class UninitializedWebSocketHandler(WebSocketHandler):
    def __init__(self, request):
        self.request = request


def test_zero_srcrwr_args():
    h = HTTPHeaders()
    rh = UninitializedWebSocketHandler(request=HTTPServerRequest(uri="wss://example.com:8000/path/to/ws"))
    s = Srcrwr(h, rh.get_query_arguments(Srcrwr.CGI, strip=True))
    assert s.header == {}


def test_one_srcrwr_args():
    h = HTTPHeaders()
    rh = UninitializedWebSocketHandler(
        request=HTTPServerRequest(
            uri="wss://example.com:8000/path/to/ws?srcrwr=IoT:https://smart.home/path/to/handler"
        )
    )
    s = Srcrwr(h, rh.get_query_arguments(Srcrwr.CGI, strip=True))

    assert s.header
    assert 'x-srcrwr' in s.header
    assert s.header['x-srcrwr'] == 'IoT=https://smart.home/path/to/handler'

    assert s['IoT'] == 'https://smart.home/path/to/handler'
    assert s['VINS'] is None


def test_several_srcrwrs_in_args():
    h = HTTPHeaders()
    rh = UninitializedWebSocketHandler(
        request=HTTPServerRequest(
            uri="wss://example.com:8000/path/to/ws?srcrwr=IoT:https://smart.home,BASS:http://bass.yandex.ru"
        )
    )
    s = Srcrwr(h, rh.get_query_arguments(Srcrwr.CGI, strip=True))

    assert s.header
    assert 'x-srcrwr' in s.header
    assert 'IoT=https://smart.home' in s.header['x-srcrwr']
    assert 'BASS=http://bass.yandex.ru' in s.header['x-srcrwr']

    assert s['BASS'] == 'http://bass.yandex.ru'
    assert s['IoT'] == 'https://smart.home'
    assert s['YALDI'] is None


def test_several_srcrwr_args():
    h = HTTPHeaders()
    rh = UninitializedWebSocketHandler(
        request=HTTPServerRequest(
            uri="wss://example.com:8000/path/to/ws?srcrwr=IoT:https://smart.home,YALDI:http://yaldi.yandex.net:10300&srcrwr=BASS:http://bass.yandex.ru"
        )
    )
    s = Srcrwr(h, rh.get_query_arguments(Srcrwr.CGI, strip=True))

    assert s.header
    assert 'x-srcrwr' in s.header
    assert 'BASS=http://bass.yandex.ru' in s.header['x-srcrwr']
    assert 'IoT=https://smart.home' in s.header['x-srcrwr']
    assert 'YALDI=http://yaldi.yandex.net' in s.header['x-srcrwr']

    assert s['BASS'] == 'http://bass.yandex.ru'
    assert s['IoT'] == 'https://smart.home'
    assert s['YALDI'] == 'http://yaldi.yandex.net'
    assert s.ports['YALDI'] == '10300'
    assert s['YABIO'] is None


def test_srcrwr_cgi_headers_override():
    h = HTTPHeaders()
    h.add(Srcrwr.NAME, 'BEE=http://bee.yandex.net')
    rh = UninitializedWebSocketHandler(
        request=HTTPServerRequest(
            uri="wss://example.com:8000/path/to/ws?srcrwr=BEE:https://bee.dev.yandex.net"
        )
    )
    s = Srcrwr(h, rh.get_query_arguments(Srcrwr.CGI, strip=True))

    assert s.header == {'x-srcrwr': 'BEE=https://bee.dev.yandex.net'}
    assert s['BEE'] == 'https://bee.dev.yandex.net'
