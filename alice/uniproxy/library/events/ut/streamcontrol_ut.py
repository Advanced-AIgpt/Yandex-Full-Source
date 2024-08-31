from alice.uniproxy.library.events import StreamControl
from alice.uniproxy.library.events import EventException


def test_construction():
    StreamControl({
        'messageId': 'foo',
        'action': 0,
        'reason': 0,
        'streamId': 1
    }, True)


def test_no_message_id():
    err = None

    try:
        StreamControl({
            'action': 0,
            'reason': 0,
            'streamId': 1
        }, True)
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'StreamControl without \'messageId\''


def test_no_action():
    err = None

    try:
        StreamControl({
            'messageId': 'foo',
            'reason': 0,
            'streamId': 1,
        }, True)
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'StreamControl without \'action\''


def test_no_reason():
    err = None

    try:
        StreamControl({
            'messageId': 'foo',
            'action': 0,
            'streamId': 1,
        }, True)
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'StreamControl without \'reason\''


def test_no_stream_id():
    err = None

    try:
        StreamControl({
            'messageId': 'foo',
            'action': 0,
            'reason': 1,
        }, True)
    except EventException as ex:
        err = ex

    assert err
    assert str(err) == 'StreamControl without \'streamId\''
