from common import AsrClient
from alice.uniproxy.library.backends_asr import SpotterStream
import tornado.gen
from tornado.locks import Event
import alice.uniproxy.library.testing
import time


def test_create_spotter_stream():
    client = AsrClient(SpotterStream, params={"topic": "SpotterTopic", "vins": {"application": {"device_id": "LOL"}}})
    assert client.asr_stream


class SpotterConditionTst(object):
    def __init__(self):
        super().__init__()
        self.sleep_timeout = 3
        self._cv = Event()

    def run(self):
        return [self.wait_on_cv(), self.notify_cv()]

    @tornado.gen.coroutine
    def wait_on_cv(self):
        before = time.time()
        yield self._cv.wait()
        assert(time.time() - before >= self.sleep_timeout)

    @tornado.gen.coroutine
    def notify_cv(self):
        yield tornado.gen.sleep(self.sleep_timeout)
        self._cv.set()


@alice.uniproxy.library.testing.ioloop_run
def test_two_steps_tornado_conditions():
    test_cond = SpotterConditionTst()
    yield test_cond.run()
