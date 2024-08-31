from alice.uniproxy.library.processors import create_event_processor
import common


def test_create_log_processors():
    mode_changed_proc = create_event_processor(common.FakeSystem, common.FakeEvent("SoundRecorder", "ModeChanged"))
    assert mode_changed_proc.event_type == "soundrecorder.modechanged"

    direction_changed_proc = create_event_processor(common.FakeSystem, common.FakeEvent("SoundRecorder", "DirectionChanged"))
    assert direction_changed_proc.event_type == "soundrecorder.directionchanged"

    # import processors' module after their creation to validate automatic registration
    import alice.uniproxy.library.processors.soundrecorder as soundrecorder

    assert isinstance(mode_changed_proc, soundrecorder.ModeChanged)
    assert isinstance(direction_changed_proc, soundrecorder.DirectionChanged)
