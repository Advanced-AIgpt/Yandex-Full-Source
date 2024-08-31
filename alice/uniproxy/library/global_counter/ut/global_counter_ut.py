from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings


def unistat_has(container, key):
    for item in container:
        if item[0] == key:
            return True
    return False


def unistat_get(container, key, defval=None):
    for item in container:
        if item[0] == key:
            return item[1]
    return defval


def test_metrics_count():
    GlobalCounter.init()
    GlobalTimings.init()
    total = len(GlobalCounter.get_metrics() + GlobalTimings.get_metrics())
    print(f"Total: {total} signals")
    assert total <= 2000
