from alice.uniproxy.library.processors import create_event_processor
import common


def test_create_asr_processors():
    proc = create_event_processor(common.FakeSystem, common.FakeEvent("ASR", "Recognize"))
    assert proc.event_type == "asr.recognize"

    import alice.uniproxy.library.processors.asr as asr

    assert isinstance(proc, asr.Recognize)
