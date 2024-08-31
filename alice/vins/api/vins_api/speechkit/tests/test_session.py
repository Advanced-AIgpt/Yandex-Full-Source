# coding: utf-8
from __future__ import unicode_literals

import logging
from uuid import uuid4

import pytest

from vins_core.dm.intent import Intent
from vins_core.dm.response import VinsResponse
from vins_core.dm.session import Session, MongoSessionStorage
from vins_core.dm.request import create_request, AppInfo
from vins_api.speechkit.session import SKSessionStorage
from vins_core.utils.datetime import utcnow


@pytest.fixture
def sk_storage(mongodb):
    return SKSessionStorage(mongodb.sessions)


@pytest.fixture
def mongo_storage(mongodb):
    return MongoSessionStorage(mongodb.sessions)


@pytest.fixture
def app(sk_app, sk_storage, mocker):
    mocker.patch.object(sk_app.vins_app, '_session_storage', sk_storage)
    yield sk_app


class SequenceGen(object):
    def __init__(self, uuid=None):
        self.uuid = uuid or uuid4()
        self.clear()

    def clear(self, sequence=0, hypothesis=0):
        self._request_id = uuid4()
        self._prev_req_id = None
        self._sequence_number = sequence
        self._hypothesis = hypothesis

    def current(self, **kwargs):
        state = dict(
            request_id=self._request_id,
            prev_req_id=self._prev_req_id,
            sequence_number=self._sequence_number,
            hypothesis_number=self._hypothesis,
        )
        state.update(kwargs)
        return create_request(self.uuid, **state)

    def next_hypothesis(self, **kwargs):
        self._hypothesis += 1
        return self.current(**kwargs)

    def next_seq(self, **kwargs):
        self._sequence_number += 1
        return self.current(**kwargs)

    def next_request(self, **kwargs):
        self._hypothesis = self._hypothesis and 0
        self._prev_req_id = self._request_id
        self._request_id = uuid4()

        if self._sequence_number is not None:
            self._sequence_number += 1

        return self.current(**kwargs)


@pytest.fixture
def sg():
    return SequenceGen()


def test_session_transition_forward(sk_storage, mongo_storage):
    uuid = uuid4()
    app_id = 'sk_app'

    sess = Session(app_id, uuid)

    req = create_request(uuid,
                         request_id=uuid4(),
                         prev_req_id=None,
                         sequence_number=0,
                         hypothesis_number=0,
                         end_of_utterance=True)

    mongo_storage.save(sess, req_info=req)

    sk_sess = sk_storage.load(app_id, uuid, req_info=req)
    sk_dct = sk_sess.to_dict()

    assert sess.to_dict() == sk_dct


def test_uniproxy_sessions_save_to_response(sk_storage, mocker):
    uuid = uuid4()
    app_id = 'sk_app'
    mocker.patch('vins_api.speechkit.session.uniproxy_vins_sessions', return_value=True)

    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'])
    response = VinsResponse()
    session = Session(app_id, uuid)
    session.change_intent(Intent('test-intent'))
    sk_storage.save(session, req_info=req_info, response=response)

    assert response.sessions
    assert response.sessions['']


def test_uniproxy_sessions_do_not_use_mongo(sk_storage, mocker):
    uuid = uuid4()
    app_id = 'sk_app'
    mocker.patch('vins_api.speechkit.session.uniproxy_vins_sessions', return_value=True)

    load_mock = mocker.patch.object(sk_storage, '_mongo_load')
    save_mock = mocker.patch.object(sk_storage, '_mongo_save')

    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'])
    response = VinsResponse()
    session = Session(app_id, uuid)
    session.change_intent(Intent('test-intent'))
    sk_storage.save(session, req_info=req_info, response=response)

    # asserts
    save_mock.assert_not_called()
    load_mock.assert_not_called()
    assert response.sessions


def test_uniproxy_sessions_restore_from_request(sk_storage, mongodb, mocker):
    uuid = uuid4()
    app_id = 'sk_app'
    mocker.patch('vins_api.speechkit.session.uniproxy_vins_sessions', return_value=True)

    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'])
    response = VinsResponse()
    session = Session(app_id, uuid)
    session.change_intent(Intent('test-intent'))
    sk_storage.save(session, req_info=req_info, response=response)
    mongodb.sessions.remove()
    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'], session=response.sessions[''])
    loaded_session = sk_storage.load(app_id, uuid, req_info)

    assert session.to_dict() == loaded_session.to_dict()


def test_uniproxy_sessions_empty_session_if_serialization_failed(sk_storage, mongodb, mocker):
    uuid = uuid4()
    app_id = 'sk_app'
    mocker.patch('vins_api.speechkit.session.uniproxy_vins_sessions', return_value=True)
    mocker.patch('vins_api.speechkit.session.zlib.compress', side_effect=Exception())

    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'])
    response = VinsResponse()
    session = Session(app_id, uuid)
    session.change_intent(Intent('test-intent'))

    sk_storage.save(session, req_info=req_info, response=response)
    assert response.sessions[''] == ''


def test_session_transition_backward(sk_storage, mongo_storage):
    uuid = uuid4()
    app_id = 'sk_app'

    req = create_request(uuid,
                         request_id=uuid4(),
                         prev_req_id=None,
                         sequence_number=0,
                         hypothesis_number=None,
                         end_of_utterance=True)
    sk_sess = Session(app_id, uuid)
    sk_storage.save(sk_sess, req_info=req, response=None)

    sess = mongo_storage.load(app_id, uuid, req_info=req)

    assert sess.to_dict() == sk_sess.to_dict()


def test_session_partials(app, sk_storage, sg):
    utt = []
    parts = 'что такое хорошо и что такое плохо'.split()

    for i, part in enumerate(parts):
        utt.append(part)
        req = sg.next_hypothesis(utterance=' '.join(utt))
        app.handle_request(req)

    session = sk_storage.load('sk_app', sg.uuid, req_info=sg.next_request())

    assert len(session.dialog_history) == 1
    assert sk_storage._collection.find({'uuid': sg.uuid}).count() == 7


@pytest.mark.parametrize('app_id', ['ru.yandex.quasar.vins_test', 'aliced.vins_test'])
def test_session_partials_smart_speakers(app, sk_storage, sg, app_id):
    sg.clear(sequence=None)
    now = utcnow()

    utt = []
    parts = 'что такое хорошо и что такое плохо'.split()
    app_info = AppInfo(app_id=app_id)

    for i, part in enumerate(parts):
        utt.append(part)
        req = sg.next_hypothesis(utterance=' '.join(utt), client_time=now, app_info=app_info)
        app.handle_request(req)

    session = sk_storage.load(
        'sk_app', sg.uuid,
        req_info=sg.next_request(
            prev_req_id=None,
            sequence_number=None,
            hypothesis_number=0,
            end_of_utterance=True,
            app_info=app_info,
        )
    )

    assert len(session.dialog_history) == 1
    assert sk_storage._collection.find({'uuid': sg.uuid}).count() == 7


def test_session_sequence(app, sk_storage, sg):
    req1 = sg.current(utterance='погода')
    req2 = sg.next_seq(utterance='я сказал погода')

    # "parallel" requests
    app.handle_request(req1)
    app.handle_request(req2)

    req3 = sg.next_request(utterance='а на завтра')
    app.handle_request(req3)

    session = sk_storage.load('sk_app', sg.uuid, req_info=sg.next_request())

    assert len(session.dialog_history) == 2
    assert map(lambda x: x.utterance.text, session.dialog_history.last(2)) == [
        'а на завтра',
        'я сказал погода',
    ]
    assert sk_storage._collection.find({'uuid': sg.uuid}).count() == 3


def test_without_sequence(app, sk_storage):
    uuid = uuid4()
    app.handle_request(create_request(uuid, utterance='1'))
    app.handle_request(create_request(uuid, utterance='2'))
    app.handle_request(create_request(uuid, utterance='3'))

    req = create_request(uuid)
    session = sk_storage.load('sk_app', uuid, req_info=req)

    assert len(session.dialog_history) == 3
    assert sk_storage._collection.find({'uuid': uuid}).count() == 1


def test_sequential_without_partials(app, sk_storage, sg):
    sg.clear(hypothesis=None)

    req1 = sg.current(end_of_utterance=True, utterance='1')
    req2 = sg.next_request(end_of_utterance=True, utterance='2')

    app.handle_request(req1)
    app.handle_request(req2)

    session = sk_storage.load('sk_app', sg.uuid, req_info=sg.next_request())

    assert len(session.dialog_history) == 2
    assert sk_storage._collection.find({'uuid': sg.uuid}).count() == 2


def test_sequential_without_partials_after_partials(app, sk_storage, sg):
    parts = 'a b c d'.split()
    utt = []

    for i, part in enumerate(parts):
        utt.append(part)
        req = sg.next_hypothesis(utterance=' '.join(utt))
        app.handle_request(req)

    req2 = sg.next_request(hypothesis_number=None, end_of_utterance=None, utterance='test')

    app.handle_request(req2)

    session = sk_storage.load('sk_app', sg.uuid, req_info=sg.next_request())

    assert len(session.dialog_history) == 2
    assert map(lambda x: x.utterance.text, session.dialog_history.last(2)) == [
        'test',
        'a b c d',
    ]
    assert sk_storage._collection.find({'uuid': sg.uuid}).count() == 5


def test_req_info_session_none(sk_storage, mocker, caplog):
    caplog.set_level(logging.ERROR)
    uuid = uuid4()
    app_id = 'sk_app'
    mocker.patch('vins_api.speechkit.session.uniproxy_vins_sessions', return_value=True)
    req_info = create_request(uuid, experiments=['uniproxy_vins_sessions'])
    assert sk_storage.load(app_id, uuid, req_info) is None
    assert 'Failed to load uniproxy session from' not in caplog.text
    assert 'Session deserialization failed' not in caplog.text
