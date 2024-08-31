from alice.uniproxy.library.backends_common.storage import MdsStorage


def test_storage_construction():
    mds = MdsStorage()
    assert mds
