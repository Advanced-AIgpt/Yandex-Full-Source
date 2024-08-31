# vim: set fileencoding=utf-8
from __future__ import unicode_literals

import logging
from functools import partial

import attr
import pymongo
from pymongo.errors import PyMongoError

from vins_core.utils.data import uuid_from_str, uuid_to_str
from vins_core.utils.logging import log_call
from vins_core.utils.datetime import utcnow
from .dialog_history import DialogHistory
from .intent import Intent
from .form_filler.models import Form
from vins_core.common.annotations import AnnotationsBag

logger = logging.getLogger(__name__)


class SessionObject(object):
    """
    The base class for anything that one might want to persist in a session.
    Such objects can react to events like session invalidation or form change.
    """

    def __init__(self, value, persistent=False, default=None, transient=False):
        super(SessionObject, self).__init__()
        self._value = value
        self._persistent = persistent
        self._default = default
        self._transient = transient

    @property
    def value(self):
        return self._value

    def on_session_clear(self):
        """
        Called when the dialog manager decides to clear the session.
        """
        if not self._persistent:
            self._value = self._default

    def on_intent_change(self, intent):
        """
        Called when the dialog manager decides to change the current intent.
        """
        pass

    def on_form_change(self, form):
        """
        Called when the dialog manager decides to change the current form.
        """
        pass

    def on_before_session_save(self):
        """
        Called by SessionStorage immediately before saving a Session.
        """
        pass

    def to_dict(self):
        value = self.value.to_dict() if hasattr(self.value, 'to_dict') else self.value

        return {
            'value': None if self._transient else value,
            'persistent': self._persistent,
            'default': self._default,
            'transient': self._transient,
        }

    @classmethod
    def from_dict(cls, obj):
        return cls(obj['value'], obj['persistent'], obj['default'], obj['transient'])


class IntentSessionObject(SessionObject):
    def on_intent_change(self, intent):
        new_intent_name = None if intent is None else intent.name
        logger.debug('Session change intent %s => %s', self.value, new_intent_name)
        self._value = intent

    @classmethod
    def from_dict(cls, obj):
        return super(IntentSessionObject, cls).from_dict(
            dict(obj, value=Intent(**obj['value']) if obj['value'] else None)
        )


class FormSessionObject(SessionObject):
    def on_form_change(self, form):
        logger.debug('Session change form %s => %s', self.value, form)
        self._value = form

    @classmethod
    def from_dict(cls, obj):
        return super(FormSessionObject, cls).from_dict(
            dict(obj, value=Form.from_dict(obj['value']) if obj['value'] else None)
        )


class DialogHistoryObject(SessionObject):
    def on_session_clear(self):
        self.value.clear()

    @classmethod
    def from_dict(cls, obj):
        return super(DialogHistoryObject, cls).from_dict(
            dict(obj, value=DialogHistory.from_dict(obj['value']))
        )


class AnnotationsBagSessionObject(SessionObject):
    @classmethod
    def from_dict(cls, obj):
        return super(AnnotationsBagSessionObject, cls).from_dict(
            dict(obj, value=AnnotationsBag.from_dict(obj['value'], strict=False))
        )

    def on_session_clear(self):
        self.value.clear()

    def on_before_session_save(self):
        if self.value is not None:
            self.value.trim()


@attr.s(slots=True)
class Session(object):
    app_id = attr.ib()
    uuid = attr.ib()
    _intent = attr.ib(default=None, repr=False)
    intent_name = attr.ib(default=None, init=False)
    form = attr.ib(default=None)
    dialog_history = attr.ib(default=attr.Factory(DialogHistory), repr=False)
    annotations = attr.ib(default=attr.Factory(AnnotationsBag), repr=False)
    _storage = attr.ib(init=False, default=attr.Factory(dict))

    def __attrs_post_init__(self):
        self.intent_name = self._intent and self._intent.name

    def has(self, name):
        return name in self._storage

    def get(self, name, default=None):
        session_object = self._storage.get(name)
        return default if session_object is None else session_object.value

    def pop(self, name, default=None):
        session_object = self._storage.pop(name, None)
        return default if session_object is None else session_object.value

    def _set_obj(self, name, obj):
        assert isinstance(obj, SessionObject)
        self._storage[name] = obj

    def set(self, name, value, persistent=True, default=None, transient=False):
        self._set_obj(name, SessionObject(value, persistent, default, transient))

    def clear(self):
        self._intent = None
        self.intent_name = None
        self.form = None
        self.dialog_history.clear()
        self.annotations.clear()
        for v in self._storage.itervalues():
            v.on_session_clear()
        logger.debug('Session cleared')
        logger.debug('Session after clearing: {}'.format(self))
        return self

    def change_intent(self, intent):
        new_intent_name = intent and intent.name
        logger.debug('Session change intent %s => %s', self.intent_name, new_intent_name)
        self._intent = intent
        self.intent_name = new_intent_name

    def change_form(self, form):
        logger.debug('Session change form %s => %s', self.form, form)
        self.form = form

    def before_save_hooks(self):
        if self.annotations:
            self.annotations.trim()

    def to_dict(self):
        objects = {
            key: value.to_dict()
            for key, value in self._storage.iteritems()
        }

        def to_dict(obj):
            # comatibility with old session
            return {
                'value': obj.to_dict() if obj is not None else None,
                'default': None,
                'persistent': False,
                'transient': False,
            }

        objects.update({
            'form': to_dict(self.form),
            'intent': to_dict(self._intent),
            'dialog_history': to_dict(self.dialog_history),
            'annotations': to_dict(self.annotations),
        })

        return {
            'app_id': self.app_id,
            'uuid': uuid_to_str(self.uuid),
            'objects': objects,
        }

    @classmethod
    def from_dict(cls, raw):
        arguments = {
            'app_id': raw['app_id'],
            'uuid': uuid_from_str(raw['uuid']),
        }
        objects = {}

        from_dict_mapping = {
            'intent': Intent.from_dict,
            'form': Form.from_dict,
            'dialog_history': DialogHistory.from_dict,
            'annotations': partial(AnnotationsBag.from_dict, strict=False),
        }

        for object_name, value in raw['objects'].iteritems():
            if object_name in from_dict_mapping:
                if value['value'] is not None:
                    from_dict = from_dict_mapping[object_name]
                    arguments[object_name] = from_dict(value['value'])
            else:
                objects[object_name] = SessionObject.from_dict(value)

        obj = cls(**arguments)
        for name, value in objects.iteritems():
            obj._set_obj(name, value)

        return obj


class SessionOld(object):
    def __init__(self, app_id, uuid):
        self._app_id = app_id
        self._uuid = uuid
        self._storage = {}
        self._set_obj('intent', IntentSessionObject(None))
        self._set_obj('form', FormSessionObject(None))
        self._set_obj('dialog_history', DialogHistoryObject(
            DialogHistory()
        ))
        self._set_obj('annotations', AnnotationsBagSessionObject(
            AnnotationsBag()
        ))

    @property
    def app_id(self):
        return self._app_id

    @property
    def uuid(self):
        return self._uuid

    @property
    def intent_name(self):
        intent = self.get('intent')
        return None if intent is None else intent.name

    @property
    def form(self):
        return self.get('form')

    @property
    def dialog_history(self):
        return self.get('dialog_history')

    @property
    def annotations(self):
        return self.get('annotations')

    @annotations.setter
    def annotations(self, value):
        self._set_obj('annotations', AnnotationsBagSessionObject(value))

    def has(self, name):
        return name in self._storage

    def get(self, name, default=None):
        session_object = self._storage.get(name)
        return default if session_object is None else session_object.value

    def pop(self, name, default=None):
        session_object = self._storage.pop(name, None)
        return default if session_object is None else session_object.value

    def _set_obj(self, name, obj):
        assert isinstance(obj, SessionObject)
        self._storage[name] = obj

    def set(self, name, value, persistent=True, default=None, transient=False):
        self._set_obj(name, SessionObject(value, persistent, default, transient))

    def clear(self):
        logger.debug('Session clear')
        for v in self._storage.itervalues():
            v.on_session_clear()
        return self

    def change_intent(self, intent):
        for v in self._storage.itervalues():
            v.on_intent_change(intent)

    def change_form(self, form):
        for v in self._storage.itervalues():
            v.on_form_change(form)

    def before_save_hooks(self):
        for v in self._storage.itervalues():
            v.on_before_session_save()

    def to_dict(self):
        return {
            'app_id': self.app_id,
            'uuid': uuid_to_str(self.uuid),
            'objects': {key: value.to_dict() for key, value in self._storage.iteritems()},
        }

    @classmethod
    def from_dict(cls, raw):
        obj = cls(raw['app_id'], uuid_from_str(raw['uuid']))
        mapping = {
            'intent': IntentSessionObject,
            'form': FormSessionObject,
            'dialog_history': DialogHistoryObject,
            'annotations': AnnotationsBagSessionObject
        }

        for object_name, value in raw['objects'].iteritems():
            cls_obj = mapping.get(object_name, SessionObject)
            obj._set_obj(object_name, cls_obj.from_dict(value))

        return obj

    def __str__(self):
        return 'Session({0})'.format(', '.join([
            '{0}={1}'.format(k, v) for k, v in {
                'app_id': self.app_id,
                'uuid': self.uuid,
                'intent_name': self.intent_name,
                'form': self.form,
                'storage_keys': self._storage.keys(),
            }.items()
        ]))

    def __repr__(self):
        return str(self)


class BaseSessionStorage(object):
    session_cls = Session

    def serialize(self, session):
        return session.to_dict()

    def deserialize(self, serialized):
        return self.session_cls.from_dict(serialized)

    def load(self, app_id, uuid, req_info, **kwargs):
        serialized = self._load(app_id, uuid, req_info.dialog_id)
        if serialized is None:
            return None

        try:
            return self.deserialize(serialized)
        except Exception as e:
            logger.error('Session deserialization failed: %s', e, exc_info=True)
            return None

    def save(self, session, req_info, **kwargs):
        session.before_save_hooks()
        serialized = self.serialize(session)
        self._save(session.app_id, session.uuid, req_info.dialog_id, serialized)

    def load_or_create(self, app_id, uuid, req_info, **kwargs):
        session = self.load(app_id, uuid, req_info, **kwargs)
        if session is None:
            session = self.session_cls(app_id=app_id, uuid=uuid)
        return session

    def _load(self, app_id, uuid, dialog_id):
        raise NotImplementedError

    def _save(self, app_id, uuid, dialog_id, serialized):
        raise NotImplementedError


class InMemorySessionStorage(BaseSessionStorage):
    def __init__(self, **kwargs):
        super(InMemorySessionStorage, self).__init__(**kwargs)
        self._data = {}

    def _load(self, app_id, uuid, dialog_id):
        return self._data.get((app_id, uuid, dialog_id))

    def _save(self, app_id, uuid, dialog_id, serialized):
        self._data[app_id, uuid, dialog_id] = serialized


class DummySessionStorage(BaseSessionStorage):
    def _load(self, app_id, uuid, dialog_id):
        return None

    def _save(self, app_id, uuid, dialog_id, serialized):
        pass


class MongoSessionStorage(BaseSessionStorage):
    def __init__(self, mongo_collection, ignore_mongo_errors=False, **kwargs):
        super(MongoSessionStorage, self).__init__(**kwargs)
        self._collection = mongo_collection
        self._is_index_created = False
        self._ignore_mongo_errors = ignore_mongo_errors

    def _ensure_index(self):
        if not self._is_index_created:
            self._collection.create_index([
                ('app_id', pymongo.ASCENDING),
                ('uuid', pymongo.ASCENDING),
                ('dialog_id', pymongo.ASCENDING),
            ], background=True)

            self._collection.create_index(
                [('updated_at', pymongo.ASCENDING)],
                expireAfterSeconds=12 * 3600,  # 12 hours
                background=True,
            )
            self._is_index_created = True

    def _load(self, app_id, uuid, dialog_id):
        try:
            self._ensure_index()

            query = {'app_id': app_id, 'uuid': uuid, 'dialog_id': dialog_id}
            with log_call('MongoSessionStorage::load_session'):
                stored_session = self._collection.find_one(
                    query, modifiers={'$maxTimeMS': 300},
                )
            return None if stored_session is None else stored_session.get('serialized')
        except PyMongoError as e:
            if self._ignore_mongo_errors:
                logger.warning('Error while loading session for uuid %s: "%s". Using empty session instead.', uuid, e)
                return None
            else:
                raise

    def _save(self, app_id, uuid, dialog_id, serialized):
        try:
            self._ensure_index()

            query = {'app_id': app_id, 'uuid': uuid, 'dialog_id': dialog_id}
            contents = query.copy()
            contents['serialized'] = serialized
            contents['updated_at'] = utcnow()
            with log_call('MongoSessionStorage::save_session'):
                self._collection.replace_one(query, contents, upsert=True)

        except PyMongoError as e:
            if self._ignore_mongo_errors:
                logger.warning('Error while saving session: \"%s\".', e)
            else:
                raise
