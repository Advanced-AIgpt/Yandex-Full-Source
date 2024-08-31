from alice.uniproxy.library.events import Directive

from alice.uniproxy.library.utils.dict_object import DictObj


class SystemMock:
    def __init__(self):
        self._next_message_id = 1

    def next_message_id(self):
        x = '%8.8x' % (self._next_message_id, )
        self._next_message_id += 1
        return x


def test_construction_simple():
    x = Directive('Namespace', 'Name', {'foo': 42})
    x = DictObj(x.create_message(SystemMock()))

    assert x.directive.header.namespace == 'Namespace'
    assert x.directive.header.name == 'Name'
    assert x.directive.header.messageId is not None
    assert x.directive.header.eventId is None
    assert x.directive.header.transferId is None
    assert x.directive.header.refMessageId is None
    assert x.directive.payload.foo == 42


def test_construction_asr_result():
    x = Directive('ASR', 'Result', {'foo': 24}, event_id='evi')
    x = DictObj(x.create_message(SystemMock()))

    assert x.directive.header.namespace == 'ASR'
    assert x.directive.header.name == 'Result'
    assert x.directive.header.messageId is not None
    assert x.directive.header.eventId == 'evi'
    assert x.directive.header.refMessageId == 'evi'
    assert x.directive.payload.foo == 24


def test_construction_with_async_event_id():
    x = Directive('ASR', 'Result', {'foo': 22}, event_id='evi', async_message_id='bar')
    x = DictObj(x.create_message(SystemMock()))

    assert x.directive.header.namespace == 'ASR'
    assert x.directive.header.name == 'Result'
    assert x.directive.header.messageId is not None
    assert x.directive.header.eventId == 'evi'
    assert x.directive.header.refMessageId == 'evi'
    assert x.directive.header.transferId == 'bar'
    assert x.directive.payload.foo == 22
