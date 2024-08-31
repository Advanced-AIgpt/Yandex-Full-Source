from alice.uniproxy.library.backends_common.apikey import check_key
import common
import tornado.gen
import tornado.concurrent


tests_config = {
    "apikeys": {
        "whitelist": [
            "some-valid-key"
        ],
        "url": "",
        "mobile_token": "my-mobile-token",
        "js_token": "my-js-token"
    }
}


def test_empty_key_and_local_check():
    result = None

    def on_ok(key):
        nonlocal result
        result = key

    def on_fail():
        nonlocal result
        result = False

    with common.ConfigPatch(tests_config):
        for key in ("", None):
            check_key(key=key, client_ip="127.0.0.1", on_ok=on_ok, on_fail=on_fail)
            assert result is False

        result = None
        check_key(key="some-valid-key", client_ip="127.0.0.1", on_ok=on_ok, on_fail=on_fail)
        assert result == "some-valid-key"


@common.run_async
async def test_remote_check():
    result_fut = tornado.concurrent.Future()

    def on_ok(key):
        result_fut.set_result(key)

    def on_fail():
        result_fut.set_result(False)

    with common.QueuedTcpServer() as srv:
        server_url = "http://localhost:{}".format(srv.port)
        tests_config["apikeys"]["url"] = server_url

        with common.ConfigPatch(tests_config):

            # valid, not mobile, IPv4
            check_key(key="remote-valid-key", client_ip="127.1.1.1", on_ok=on_ok, on_fail=on_fail)

            server_stream = await srv.pop_stream()

            request = await common.read_http_request(server_stream)
            assert request.method == "GET"
            assert request.path == "/check_key"
            assert request.arguments["service_token"] == [b"my-mobile-token"]
            assert request.arguments["key"] == [b"remote-valid-key"]
            assert request.arguments["user_ip"] == [b"127.1.1.1"]
            assert request.arguments["ip_v"] == [b"4"]

            await server_stream.write(b"HTTP/1.1 200 OK\r\n\r\n")
            server_stream.close()

            assert "remote-valid-key" == await result_fut

            # not valid, mobile, IPv6
            result_fut = tornado.concurrent.Future()
            check_key(key="remote-invalid-key", client_ip="fe80::1", on_ok=on_ok, on_fail=on_fail)

            server_stream = await srv.pop_stream()

            request = await common.read_http_request(server_stream)
            assert request.method == "GET"
            assert request.path == "/check_key"
            assert request.arguments["service_token"] == [b"my-mobile-token"]
            assert request.arguments["key"] == [b"remote-invalid-key"]
            assert request.arguments["user_ip"] == [b"fe80::1"]
            assert request.arguments["ip_v"] == [b"6"]

            await server_stream.write(b"HTTP/1.1 403 Forbidden\r\n\r\n")
            server_stream.close()

            assert not (await result_fut)
