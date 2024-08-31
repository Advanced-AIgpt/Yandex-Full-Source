from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.messenger as messenger
import common


def test_create_messenger_processors():
    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "PostMessage"))
    assert proc.event_type == "messenger.postmessage"
    assert isinstance(proc, messenger.PostMessage)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "HistoryRequest"))
    assert proc.event_type == "messenger.historyrequest"
    assert isinstance(proc, messenger.HistoryRequest)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "SubscriptionRequest"))
    assert proc.event_type == "messenger.subscriptionrequest"
    assert isinstance(proc, messenger.SubscriptionRequest)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "EditHistoryRequest"))
    assert proc.event_type == "messenger.edithistoryrequest"
    assert isinstance(proc, messenger.EditHistoryRequest)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "Whoami"))
    assert proc.event_type == "messenger.whoami"
    assert isinstance(proc, messenger.Whoami)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "VoiceInput"))
    assert proc.event_type == "messenger.voiceinput"
    assert isinstance(proc, messenger.VoiceInput)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "SetVoiceChats"))
    assert proc.event_type == "messenger.setvoicechats"
    assert isinstance(proc, messenger.SetVoiceChats)

    proc = create_event_processor(common.FakeSystem, common.FakeEvent("Messenger", "Fanout"))
    assert proc.event_type == "messenger.fanout"
    assert isinstance(proc, messenger.Fanout)
