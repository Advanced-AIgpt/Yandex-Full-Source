from alice.uniproxy.tools.wsproxy_admin import RequestArguments, _make_request
import asyncio


def test_make_request(uniproxy2, argparser):
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 0}))
    req_arg = RequestArguments(argparser.parse_args(
        ['--service', 'wsproxy-vla', 'disconnect', '--ratio', '1.0']))
    url, status, exc = asyncio.run(_make_request(
        f'{uniproxy2.http_url}/admin', req_arg))
    assert url.rstrip('/admin') == uniproxy2.http_url
    assert status == 200
    assert exc is None
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 1}))

    parser = RequestArguments(argparser.parse_args(
        ['--host', uniproxy2.http_url.lstrip("http://"), 'disconnect', '--ratio', '1.0']))
    parser._ratio = 'abc'
    url, status, exc = asyncio.run(_make_request(
        f'{uniproxy2.http_url}/admin', parser))

    assert status == 400
    assert exc == 'bad response'
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 1}))

    parser = RequestArguments(argparser.parse_args(
        ['--host', uniproxy2.http_url.lstrip("http://"), 'disconnect', '--surface', '123', '--ratio', '1.0']))
    parser._ratio = 'abc'
    url, status, exc = asyncio.run(_make_request(
        f'{uniproxy2.http_url}/admin', parser))

    assert status == 400
    assert exc == 'bad response'
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 1}))
