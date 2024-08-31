from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.log as log
import common


def test_create_log_processors():
    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("Log", "Spotter"))
    assert proc.event_type == "log.spotter"
    assert isinstance(proc, log.Spotter)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("Log", "RequestStat"))
    assert proc.event_type == "log.requeststat"
    assert isinstance(proc, log.RequestStat)
