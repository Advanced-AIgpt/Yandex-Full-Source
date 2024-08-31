from alice.uniproxy.library.global_counter.delivery import DeliveryCounter, DeliveryTimings
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings


def test_uniproxy():
    DeliveryCounter.init()
    DeliveryTimings.init()
    assert len(GlobalCounter.get_metrics() + GlobalTimings.get_metrics()) <= 1000
