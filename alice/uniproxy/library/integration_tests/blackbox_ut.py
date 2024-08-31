import tornado.ioloop
import tornado.gen
from alice.uniproxy.library.auth.blackbox import blackbox_client
from alice.uniproxy.library.global_counter import GlobalCounter

import yatest.common

TEST_TOKEN = yatest.common.source_path('alice/uniproxy/library/integration_tests/test.token')


@tornado.gen.coroutine
def _test_good_token():
    GlobalCounter.init()
    token = open(TEST_TOKEN, "r").read().strip()

    try:
        res = yield blackbox_client().uid4oauth(token, "127.0.0.1", 0)
    except Exception as exc:
        assert str(exc) is None
    assert res[0].isdigit()
    assert res[1] is None


@tornado.gen.coroutine
def _test_bad_token():
    GlobalCounter.init()
    token = "DOESNTLOOKLIKEOUATHTOKEN"

    try:
        yield blackbox_client().uid4oauth(token, "127.0.0.1", 0)
        assert False
    except Exception as exc:
        return


@tornado.gen.coroutine
def _test_ticket4oauth():
    GlobalCounter.init()
    token = open(TEST_TOKEN, "r").read().strip()
    user_ticket = None

    user_ticket = yield blackbox_client().ticket4oauth(token, '127.0.0.1', 0)

    assert(user_ticket is not None)


def test_good_token():
    tornado.ioloop.IOLoop.current().run_sync(_test_good_token)


def test_bad_token():
    tornado.ioloop.IOLoop.current().run_sync(_test_bad_token)


def test_ticket4oauth():
    tornado.ioloop.IOLoop.current().run_sync(_test_ticket4oauth)
