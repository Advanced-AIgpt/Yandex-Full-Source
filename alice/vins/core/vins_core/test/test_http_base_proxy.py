# coding: utf-8
import requests_mock
from vins_core.ext.base import BaseHTTPAPI


def test_base_http_proxy():
    url = 'http://test.org/1'
    proxy_url = 'http://megaproxy.ru/'

    resp = 'I was proxied!'
    fail_resp = 'I was NOT proxied!'

    proxy_headers = {'X-Proxy-Header': 'it\'s proxy', 'X-Other-Proxy-Header': 'it\'s another proxy header as well'}
    proxy_headers_with_url = {'X-Proxy-Header': 'it\'s proxy', 'X-Other-Proxy-Header': 'it\'s another proxy header as well', 'X-Host': url}

    # standard method doesn't check for missing headers
    def headers_inclusion_matcher(headers):
        def matcher(request):
            for header in headers:
                assert header in request.headers
                assert headers[header] == request.headers[header]
            return True
        return matcher

    with requests_mock.mock() as m:
        m.get(url, text=fail_resp)
        m.get(proxy_url, additional_matcher=headers_inclusion_matcher(proxy_headers_with_url), text=resp)

        http = BaseHTTPAPI()

        # don't proxy anything
        kwargs = {'url': url}
        assert http.get(url).text == fail_resp
        assert http.get(**kwargs).text == fail_resp

        # setup proxy
        http.PROXY_URL = proxy_url
        http.PROXY_SKIP = 'http://testt;https;htttp;http://test.org/22;   http   '
        http._default_headers = proxy_headers

        assert http.get(url).text == resp
        assert http.get(**kwargs).text == resp

        # skip proxy
        http.PROXY_SKIP = 'http://testt;https;htttp;http://test.org/11;   http   ;http://test.org'
        assert http.get(url).text == fail_resp
        assert http.get(**kwargs).text == fail_resp
