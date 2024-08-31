import tornado.ioloop
import tornado.gen
from rtlog import begin_request
from unisystem import UniSystem


MOCK = type("UniSystemMock", (object,), {
    "set_oauth_token": UniSystem.set_oauth_token,
    "_get_uid": UniSystem._get_uid,
    "_preprocess_oauth_token": UniSystem._preprocess_oauth_token,
    "init_futures": {},
    "session_data": {},
    "client_ip": "",
    "_save_uid_and_continue": lambda *x: x,
    "_on_bad_uid": lambda *x: x,
    "rt_log": begin_request(session=True),
})()


@tornado.gen.coroutine
def _test_oauth_token_proxy():
    MOCK.set_oauth_token("test")
    assert MOCK.session_data.get("request").get("additional_options").get("oauth_token") == "test"


def test_oauth_token_proxy():
    tornado.ioloop.IOLoop.current().run_sync(_test_oauth_token_proxy)
