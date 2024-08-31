# coding: utf-8

from __future__ import unicode_literals

import os
from uuid import uuid4

import pytest
import attr
from freezegun import freeze_time
from pymongo.errors import ConnectionFailure

from vins_core.utils.data import list_resource_files_with_prefix, open_resource_file
from vins_core.dm.request import create_request
from vins_core.dm.session import MongoSessionStorage, Session as SessionNew, SessionOld
from vins_core.dm.form_filler.models import SlotConcatenation
from vins_core.common.annotations import BaseAnnotation, register_annotation


@pytest.fixture(
    params=['new_session_cls', 'old_session_cls', 'mixed1_session', 'mixed2_session'],
    scope='function',
    autouse=True,
)
def session_cls(request, mocker, autouse=True):
    (storage_cls, new_cls) = {
        'new_session_cls': (SessionNew, SessionNew),
        'old_session_cls': (SessionOld, SessionOld),
        'mixed1_session': (SessionNew, SessionOld),
        'mixed2_session': (SessionOld, SessionNew),
    }[request.param]

    mocker.patch('vins_core.dm.session.BaseSessionStorage.session_cls', new=storage_cls)
    yield new_cls


@attr.s
class TestAnnotation(BaseAnnotation):
    string = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(string=data.get('string'))


register_annotation(TestAnnotation, 'test_session_annotation')


class MongoCollectionErrorMock(object):
    def create_index(self, *args, **kwargs):
        raise ConnectionFailure


def test_mongo_error_tolerance():
    collection = MongoCollectionErrorMock()

    session_storage_no_errors = MongoSessionStorage(collection, ignore_mongo_errors=True)
    uuid = uuid4()
    request = create_request(uuid)

    assert session_storage_no_errors.load(app_id='123', uuid=uuid, req_info=request) is None

    session_storage = MongoSessionStorage(collection, ignore_mongo_errors=False)
    with pytest.raises(ConnectionFailure):
        session_storage.load(app_id='123', uuid=uuid, req_info=request)


def create_session_dict(app_id, uuid, intent=None, form=None, annotations=None, dialog_history_turns=()):
    if annotations is None:
        annotations = {}

    return {
        'app_id': app_id,
        'uuid': uuid,
        'objects': {
            'annotations': {'value': annotations, 'default': None, 'persistent': False, 'transient': False},
            'intent': {'value': intent, 'default': None, 'persistent': False, 'transient': False},
            'form': {'value': form, 'default': None, 'persistent': False, 'transient': False},
            'dialog_history': {
                'default': None,
                'persistent': False,
                'transient': False,
                'value': {
                    'app_id': 'vins',
                    'uuid': '__uuid:deadbeef-6898-4c0a-af93-7a8821aab244',
                    'turns': list(dialog_history_turns),
                },
            }
        }
    }


def create_form(name, nlg_phrase, slot_value, slot_type='string', shares_slots=False):
    return {
        'form': name,
        'events': [{
            'event': 'submit',
            'handlers': [{
                'handler': 'callback',
                'name': 'nlg_callback',
                'params': {
                    'phrase_id': nlg_phrase
                }
            }]
        }],
        'required_slot_groups': [],
        'is_ellipsis': False,
        'shares_slots_with_previous_form': shares_slots,
        'slots': [{
            'active': False,
            'disabled': False,
            'events': [],
            'expected_values': None,
            'import_tags': ['thing'],
            'import_entity_tags': [],
            'import_entity_types': [],
            'import_entity_pronouns': [],
            'matching_type': 'exact',
            'optional': True,
            'normalize_to': None,
            'share_tags': ['thing'],
            'slot': 'thing',
            'types': [slot_type],
            'value': slot_value,
            'source_text': slot_value,
            'value_type': slot_type,
            'concatenation': SlotConcatenation.forbid.name,
            'allow_multiple': False
        }],
    }


def test_session_serialize_empty(session_cls):
    session = session_cls('test-app', 'test-uuid')
    assert session.to_dict() == create_session_dict('test-app', 'test-uuid')


def test_session_to_dict(simple_app, session_cls):
    req_info = create_request('123', 'что такое яблоко')
    simple_app.handle_request(req_info)

    req_info = create_request('123', 'какого цвета яблоко')
    simple_app.handle_request(req_info)

    session = simple_app.vins_app._load_session(req_info)
    session_dct = session.to_dict()

    assert session_dct == session_cls.from_dict(session_dct).to_dict()

    # Manually add annotations to the session and check again.
    from vins_core.common.annotations import AnnotationsBag

    session.annotations = AnnotationsBag(bag={
        'test_session_annotation': TestAnnotation(string='resolved string stored here')
    })

    session_dct = session.to_dict()

    assert session_dct == session_cls.from_dict(session_dct).to_dict()


@pytest.mark.slowtest
@freeze_time('2017-01-25 19:45:00')
def test_mongo_storage(function_scoped_simple_app, mongodb):
    app = function_scoped_simple_app

    collection = mongodb.sessions
    app.vins_app._session_storage = MongoSessionStorage(collection)

    req_info = create_request('123', 'что такое яблоко')
    app.handle_request(req_info)
    req_info = create_request('123', 'какого цвета яблоко')
    app.handle_request(req_info)

    data = next(collection.find({}))
    data.pop('_id')

    expected = create_session_dict(
        'test_app', '123',
        intent={
            'name': 'intent_2'
        },
        annotations={},
        form=create_form(name='intent_2', nlg_phrase='say_2', slot_value='яблоко', shares_slots=True),
        dialog_history_turns=[
            {
                'annotations': {},
                'dt': 1485373500,
                'form': create_form('intent_1', 'say_1', 'яблоко'),
                'response': None,
                'response_text': 'яблоко это...',
                'utterance': {
                    'input_source': 'text',
                    'text': 'что такое яблоко',
                },
                'voice_text': 'яблоко это...'
            }, {
                'annotations': {},
                'dt': 1485373500,
                'form': create_form('intent_2', 'say_2', 'яблоко', shares_slots=True),
                'response': {
                    'cards': [{'type': 'simple_text', 'text': 'яблоко... цвета', 'tag': None}],
                    'templates': {},
                    'directives': [],
                    'meta': [],
                    'should_listen': None,
                    'special_buttons': [],
                    'force_voice_answer': False,
                    'autoaction_delay_ms': None,
                    'suggests': [],
                    'voice_text': 'яблоко... цвета',
                    'features': {},
                    'frame_actions': {},
                    'scenario_data': {},
                    'stack_engine': None,
                },
                'response_text': 'яблоко... цвета',
                'utterance': {
                    'input_source': 'text',
                    'text': 'какого цвета яблоко',
                },
                'voice_text': 'яблоко... цвета'
            }
        ]
    )

    assert data['serialized'] == expected


def test_mongo_storage_presave_hooks_called(mocker, simple_app):
    m1 = mocker.patch.object(SessionNew, 'before_save_hooks')
    m2 = mocker.patch.object(SessionOld, 'before_save_hooks')

    req_info = create_request('123', 'что такое яблоко')
    simple_app.handle_request(req_info)

    assert m1.call_count + m2.call_count == 1


def test_serialization_after_reset(simple_app, session_cls):
    req_info = create_request('123', 'что такое яблоко')
    simple_app.handle_request(req_info)

    session = simple_app.vins_app._load_session(req_info)
    session.clear()

    session_dct = session.to_dict()
    new_session = session_cls.from_dict(session_dct)
    assert session_dct == new_session.to_dict()


@pytest.mark.parametrize('s2_cls', [SessionNew, SessionOld])
def test_empty_annotations(session_cls, s2_cls):
    s1_cls = session_cls
    dct = s1_cls('app', 'uuid').to_dict()
    assert dct == s1_cls.from_dict(dct).to_dict()
    assert dct == s2_cls.from_dict(dct).to_dict()


def load_sessions_data():
    from bson.json_util import loads

    prefix = 'vins_core/test/test_data/sessions'
    for filename in list_resource_files_with_prefix(prefix):
        with open_resource_file(os.path.join(prefix, filename)) as f:
            yield loads(f.read())


@pytest.mark.parametrize('s2_cls', [SessionNew, SessionOld])
@pytest.mark.parametrize('session_dct', load_sessions_data())
def test_convert_real(session_cls, s2_cls, session_dct):
    dct = session_cls.from_dict(session_dct).to_dict()

    assert dct == s2_cls.from_dict(session_dct).to_dict()
    assert dct == s2_cls.from_dict(dct).to_dict()
