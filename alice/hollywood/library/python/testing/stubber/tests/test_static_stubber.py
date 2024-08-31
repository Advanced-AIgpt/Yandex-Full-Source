import requests

from yatest import common as yc

from alice.hollywood.library.python.testing.stubber.static_stubber import StaticStubber


def test_static_stubber(port):
    static_data_file_path = yc.source_path('alice/hollywood/library/python/testing/stubber/tests/data_static_stubber/file')
    expected_resp_content = b'Some static text data'
    with StaticStubber(port, '/path', static_data_file_path) as stubber:
        url = f'http://localhost:{stubber.port}/path'

        get_resp = requests.get(url)
        assert get_resp.content == expected_resp_content

        post_resp = requests.post(url)
        assert post_resp.content == expected_resp_content

        put_resp = requests.put(url)
        assert put_resp.content == expected_resp_content
