import collections

from alice.uniproxy.library.notificator_api.metrics import metrics_for
from alice.uniproxy.library.notificator_api.metrics import ScopedMetric
from alice.uniproxy.library.notificator_api.metrics import MetricsBackend
from alice.uniproxy.library.notificator_api.metrics import NullScopedMetric
from alice.uniproxy.library.notificator_api.metrics import NOTIFICATION_METRICS_MAPPING

from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyTimings

from alice.uniproxy.library.global_counter import GolovanBackend

from alice.uniproxy.library.async_http_client.http_request import HTTPError


class StubMetricsBackend(MetricsBackend):
    def __init__(self):
        super().__init__()
        self._rates = collections.defaultdict(int)
        self._hgram = collections.defaultdict(float)

    def rate(self, name, count=1):
        self._rates[name] += count

    def hgram(self, name, duration):
        self._hgram[name] = duration


def test_metrics_backend():
    b = StubMetricsBackend()
    b.rate("hello")
    assert b._rates["hello"] == 1

    b.rate("world")
    assert b._rates["world"] == 1

    b.rate("hello")
    assert b._rates["hello"] == 2

    b.rate("world", 2)
    assert b._rates["world"] == 3


def test_null_metrics():
    with metrics_for("world", None) as m:
        assert m.__class__ == NullScopedMetric
        assert m.label == "world"

    b = StubMetricsBackend()
    with metrics_for("hello", b):
        assert m.__class__ == NullScopedMetric
        assert m.label == "world"


def test_scoped_metrics_ok():
    for k, v in NOTIFICATION_METRICS_MAPPING.items():
        b = StubMetricsBackend()

        with metrics_for(k, b) as m:
            assert m.__class__ == ScopedMetric
            assert m.label == k
            assert m.name == v
            assert m.backend is not None
            assert m.backend.__class__ == StubMetricsBackend

        assert len(b._rates) == 1
        assert len(b._hgram) == 1
        assert b._rates[v + "_OK_SUMM"] == 1
        assert b._hgram[(v + "_time").lower()] > 0.00000001


def test_scoped_metrics_other_error():
    for k, v in NOTIFICATION_METRICS_MAPPING.items():
        b = StubMetricsBackend()

        try:
            with metrics_for(k, b) as m:
                assert m.__class__ == ScopedMetric
                assert m.label == k
                assert m.name == v
                assert m.backend is not None
                assert m.backend.__class__ == StubMetricsBackend
                raise RuntimeError("OtherError")
        except Exception:
            pass

        assert len(b._rates) == 1
        assert len(b._hgram) == 1
        assert b._rates[v + "_OTHER_ERR_SUMM"] == 1
        assert b._hgram[(v + "_time").lower()] > 0.00000001


def test_scoped_metrics_http_error():
    for k, v in NOTIFICATION_METRICS_MAPPING.items():
        b = StubMetricsBackend()

        try:
            with metrics_for(k, b) as m:
                assert m.__class__ == ScopedMetric
                assert m.label == k
                assert m.name == v
                assert m.backend is not None
                assert m.backend.__class__ == StubMetricsBackend
                raise HTTPError(code=503)
        except Exception:
            pass

        assert len(b._rates) == 1
        assert len(b._hgram) == 1
        assert b._rates[v + "_ERR_SUMM"] == 1
        assert b._hgram[(v + "_time").lower()] > 0.00000001


def test_scoped_metrics_timeout():
    for k, v in NOTIFICATION_METRICS_MAPPING.items():
        b = StubMetricsBackend()

        try:
            with metrics_for(k, b) as m:
                assert m.__class__ == ScopedMetric
                assert m.label == k
                assert m.name == v
                assert m.backend is not None
                assert m.backend.__class__ == StubMetricsBackend
                raise HTTPError(code=HTTPError.CODE_REQUEST_TIMEOUT)
        except Exception:
            pass

        assert len(b._rates) == 1
        assert len(b._hgram) == 1
        assert b._rates[v + "_TIMEOUT_SUMM"] == 1
        assert b._hgram[(v + "_time").lower()] > 0.00000001


def test_scoped_metrics_with_golovan_backend():
    UniproxyCounter.init()
    UniproxyTimings.init()
    for k, v in NOTIFICATION_METRICS_MAPPING.items():
        b = GolovanBackend()

        with metrics_for(k, b) as m:
            assert m.__class__ == ScopedMetric
            assert m.label == k
            assert m.name == v
            assert m.backend is not None
