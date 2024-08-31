import json

from alice.uniproxy.library.common_handlers import UnistatHandler
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings


def test_unistat_metrics_count():
    class HandlerMock:
        def __init__(self):
            self.metrics_count = 0

        def set_header(self, *args):
            pass

        def write(self, data):
            self.metrics_count = len(json.loads(data))

    mock = HandlerMock()

    GlobalCounter.init()
    GlobalTimings.init()
    UnistatHandler.get(mock)
    assert mock.metrics_count > 100
    assert mock.metrics_count < 1000
