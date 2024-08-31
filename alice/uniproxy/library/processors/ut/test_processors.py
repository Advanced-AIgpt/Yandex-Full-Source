from alice.uniproxy.library.processors import register_event_processor, create_event_processor
from alice.uniproxy.library.processors import EventException, EventProcessor
import common
import pytest


@register_event_processor
class FakeProcessorA(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)


@register_event_processor
class FakeProcessorB(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)


# ====================================================================================================================
def test_decorated_processors():
    assert FakeProcessorA.event_type == "test_processors.fakeprocessora"
    assert FakeProcessorB.event_type == "test_processors.fakeprocessorb"

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("test_processors", "FakeProcessorA"))
    assert isinstance(proc, FakeProcessorA)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("test_processors", "FakeProcessorB"))
    assert isinstance(proc, FakeProcessorB)

    with pytest.raises(EventException):
        proc = create_event_processor(common.FakeSystem(), common.FakeEvent("test_processors", "FakeProcessorC"))


def test_vins_processors():
    proc_text_input = create_event_processor(common.FakeSystem(), common.FakeEvent("Vins", "TextInput"))
    assert proc_text_input.event_type == "vins.textinput"

    proc_custom_input = create_event_processor(common.FakeSystem(), common.FakeEvent("Vins", "CustomInput"))
    assert proc_custom_input.event_type == "vins.custominput"

    proc_voice_input = create_event_processor(common.FakeSystem(), common.FakeEvent("Vins", "VoiceInput"))
    assert proc_voice_input.event_type == "vins.voiceinput"

    proc_music_input = create_event_processor(common.FakeSystem(), common.FakeEvent("Vins", "MusicInput"))
    assert proc_music_input.event_type == "vins.musicinput"

    import alice.uniproxy.library.processors.vins as vins

    assert isinstance(proc_text_input, vins.TextInput)
    assert isinstance(proc_custom_input, vins.CustomInput)
    assert isinstance(proc_voice_input, vins.VoiceInput)
    assert isinstance(proc_music_input, vins.MusicInput)
