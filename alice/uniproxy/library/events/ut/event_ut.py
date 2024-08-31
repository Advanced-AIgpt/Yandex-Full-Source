import pytest

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.events import EventException
from alice.uniproxy.library.events import GoAway

from alice.uniproxy.library.settings import config


def test_event_construction():
    Event({
        'header': {
            'namespace': 'Foo',
            'name': 'Bar',
            'messageId': 'msgid',
        },
        'payload': {}
    })


def test_event_no_message_id():
    err = None

    try:
        Event({
            'header': {
                'namespace': 'Foo',
                'name': 'Bar'
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'Event without \'messageId\''


def test_event_no_namespace():
    err = None

    try:
        Event({
            'header': {
                'name': 'Bar',
                'messageId': 'msgid',
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'Event without \'namespace\''


def test_event_no_name():
    err = None

    try:
        Event({
            'header': {
                'namespace': 'Bar',
                'messageId': 'msgid',
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'Event without \'name\''


def test_event_invalid_stream_id():
    err = None

    try:
        Event({
            'header': {
                'namespace': 'Bar',
                'name': 'Foo',
                'messageId': 'msgid',
                'streamId': 2,
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'streamId is even, client streams must have odd ids.'


def test_event_max_stream_id():
    err = None

    try:
        Event({
            'header': {
                'namespace': 'Bar',
                'name': 'Foo',
                'messageId': 'BarFooMsgId',
                'streamId': config['max_stream_id'] + 1,
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex
    except GoAway as ex:
        err = ex

    assert err
    assert isinstance(err, GoAway)
    assert str(err) == 'Bad streamId in Event BarFooMsgId'


def test_event_ref_stream_id():
    event_no_ref_stream_id = Event({
        'header': {
            'namespace': 'Foo',
            'name': 'Bar',
            'messageId': 'msgid',
        },
        'payload': {}
    })

    assert event_no_ref_stream_id.ref_stream_id is None

    event_none_ref_stream_id = Event({
        'header': {
            'namespace': 'Foo',
            'name': 'Bar',
            'messageId': 'msgid',
            'refStreamId': None,
        },
        'payload': {}
    })

    assert event_none_ref_stream_id.ref_stream_id is None

    event_with_ref_stream_id = Event({
        'header': {
            'namespace': 'Foo',
            'name': 'Bar',
            'messageId': 'msgid',
            'refStreamId': 124,
        },
        'payload': {}
    })

    assert event_with_ref_stream_id.ref_stream_id == 124


@pytest.mark.parametrize("ref_stream_id,expected_error", [
    (-2, "refStreamId must be positive integer"),
    (":)", "refStreamId must be positive integer"),
    (1, "refStreamId is odd, server streams must have even ids."),
])
def test_event_invalid_ref_stream_id(ref_stream_id, expected_error):
    err = None

    try:
        Event({
            'header': {
                'namespace': 'Foo',
                'name': 'Bar',
                'messageId': 'msgid',
                'refStreamId': ref_stream_id,
            },
            'payload': {}
        })
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == expected_error
