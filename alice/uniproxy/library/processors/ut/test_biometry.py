from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.biometry as biometry
import common


def test_create_biometry_processors():
    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Biometry", "Identify"))
    assert proc.event_type == "biometry.identify"
    assert isinstance(proc, biometry.Identify)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Biometry", "Classify"))
    assert proc.event_type == "biometry.classify"
    assert isinstance(proc, biometry.Classify)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Biometry", "CreateOrUpdateUser"))
    assert proc.event_type == "biometry.createorupdateuser"
    assert isinstance(proc, biometry.CreateOrUpdateUser)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Biometry", "GetUsers"))
    assert proc.event_type == "biometry.getusers"
    assert isinstance(proc, biometry.GetUsers)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Biometry", "RemoveUser"))
    assert proc.event_type == "biometry.removeuser"
    assert isinstance(proc, biometry.RemoveUser)
