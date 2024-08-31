from alice.uniproxy.tools.wsproxy_admin import perform_requests
import asyncio


def test_perform_requests(uniproxy2, argparser):
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 0}))
    failed_requests = perform_requests(argparser.parse_args(
        ['--host', f'local,lhost,{uniproxy2.http_url.lstrip("http://")}', '--apply-to-all', 'disconnect', '--ratio', '1.0']))

    assert len(failed_requests) == 2
    failed_requests.sort(key=lambda x: x[0])
    assert failed_requests[0][0] == 'http://lhost/admin'
    assert failed_requests[1][0] == 'http://local/admin'
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 1}))

    failed_requests = perform_requests(argparser.parse_args(
        ['--host', uniproxy2.http_url.lstrip("http://"), '--apply-to-all', 'disconnect', '--ratio', '0.5']))
    assert len(failed_requests) == 0
    asyncio.run(uniproxy2.check_unistat(
        {'on_disconnect_clients_connections_summ': 2}))
