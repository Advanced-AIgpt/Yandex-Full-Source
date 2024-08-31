from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings


def test_uniproxy():
    UniproxyCounter.init()
    UniproxyTimings.init()
    total = len(GlobalCounter.get_metrics() + GlobalTimings.get_metrics())
    print(f"Total: {total} signals")
    assert total <= 2000
