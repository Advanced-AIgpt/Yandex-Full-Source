# coding: utf-8

import logging
import pymongo
import json
import zlib
import base64
from pymongo.errors import PyMongoError

from vins_core.dm.session import BaseSessionStorage
from vins_core.utils.logging import log_call
from vins_core.utils.datetime import utcnow, datetime_to_timestamp
from vins_core.utils.metrics import sensors

logger = logging.getLogger(__name__)

MONGO_MAX_FIND_TIME = 300


def uniproxy_vins_sessions(req_info):
    return (
        req_info.experiments['uniproxy_vins_sessions'] is not None
        or req_info.experiments['stateless_uniproxy_session'] is not None
    )


def is_smart_speaker(app_id):
    return app_id and ('quasar' in app_id or 'aliced' in app_id)


class SKSessionStorage(BaseSessionStorage):
    def __init__(self, collection, ignore_mongo_errors=False, **kwargs):
        super(SKSessionStorage, self).__init__(**kwargs)
        self._collection = collection
        self._is_index_created = False
        self._ignore_mongo_errors = ignore_mongo_errors

    def _ensure_index(self):
        if not self._is_index_created:
            self._collection.create_index([
                ('app_id', pymongo.ASCENDING),
                ('uuid', pymongo.ASCENDING),
                ('chat_id', pymongo.ASCENDING),
                ('request_id', pymongo.ASCENDING),
            ], background=True)

            self._collection.create_index([
                ('sequence_number', pymongo.DESCENDING),
                ('hypothesis_number', pymongo.DESCENDING)
            ], background=True)

            self._collection.create_index(
                [('updated_at', pymongo.ASCENDING)],
                expireAfterSeconds=12 * 3600,  # 12 hours
                background=True,
            )
            self._is_index_created = True

    def load(self, app_id, uuid, req_info, **kwargs):
        serialized = self._load(app_id, req_info, **kwargs)
        if serialized is None:
            return None
        # MEGAMIND-571: treat session with only __megamind__ field as empty
        if len(serialized) == 1 and '__megamind__' in serialized:
            return None
        try:
            return self.deserialize(serialized)
        except Exception as e:
            logger.exception('Session deserialization failed: %s', e)
            return None

    def _load(self, app_id, req_info, **kwargs):
        if uniproxy_vins_sessions(req_info):
            sensors.inc_counter('sessions_received')
            if req_info.session is not None:
                try:
                    result = base64.b64decode(req_info.session)
                    result = zlib.decompress(result)
                    result = json.loads(result)
                    return result
                except Exception as e:
                    logger.exception('Failed to load uniproxy session from [%s], exception %s', req_info.session, e)
            return None
        else:
            return self._mongo_load(app_id, req_info, **kwargs)

    def _mongo_load(self, app_id, req_info, **kwargs):
        sequence_number = req_info.sequence_number
        prev_req_id = req_info.prev_req_id

        if is_smart_speaker(req_info.app_info.app_id):
            sequence_number = datetime_to_timestamp(req_info.client_time)
            prev_req_id = {'$ne': req_info.request_id}

        if sequence_number is None:
            prev_req_id = None

        try:
            self._ensure_index()

            query = {
                'app_id': app_id,
                'uuid': req_info.uuid,
                'chat_id': req_info.dialog_id,
                'request_id': prev_req_id,
            }

            with log_call('%s::load_session' % self.__class__.__name__), sensors.timer('mongo_load_session_time'):
                stored_session = self._collection.find_one(
                    query,
                    sort=[
                        ('sequence_number', pymongo.DESCENDING),
                        ('hypothesis_number', pymongo.DESCENDING),
                    ],
                    modifiers={'$maxTimeMS': MONGO_MAX_FIND_TIME},
                )
                sensors.inc_counter('mongo_load_session')

            return None if not stored_session else stored_session['serialized']

        except PyMongoError as e:
            sensors.inc_counter('mongo_error', labels={'error': e.__class__.__name__})
            if self._ignore_mongo_errors:
                logger.warning('Error while loading session for uuid %s: "%s".'
                               ' Using empty session instead.', req_info.uuid, e)
                return None
            else:
                raise

    def save(self, session, req_info, response, **kwargs):
        if uniproxy_vins_sessions(req_info):
            try:
                serialized = self.serialize(session)
                serialized = json.dumps(serialized)
                serialized = zlib.compress(serialized)
                serialized = base64.b64encode(serialized)
                response.set_session(req_info.dialog_id, serialized)
            except Exception as e:
                logger.exception('Failed to save uniproxy session: %s', e)
                response.set_session(req_info.dialog_id, '')
            else:
                sensors.inc_counter('sessions_sent')
                sensors.set_sensor('session_size', len(serialized))
        else:
            return self._mongo_save(session, req_info, response, **kwargs)

    def _mongo_save(self, session, req_info, response, **kwargs):
        hypothesis_number = req_info.utterance and req_info.utterance.hypothesis_number
        sequence_number = req_info.sequence_number
        request_id = req_info.request_id

        if is_smart_speaker(req_info.app_info.app_id):
            sequence_number = datetime_to_timestamp(req_info.client_time)

        if sequence_number is None:
            # update same document if get request from old speechkit
            request_id = None
            sequence_number = None
            hypothesis_number = None

        try:
            self._ensure_index()

            query = {
                'app_id': session.app_id,
                'uuid': req_info.uuid,
                'chat_id': req_info.dialog_id,
                'request_id': request_id,
            }

            doc = query.copy()

            doc.update({
                'sequence_number': sequence_number,
                'hypothesis_number': hypothesis_number,
                'serialized': self.serialize(session),
                'updated_at': utcnow(),
            })

            with log_call('%s::save_session' % self.__class__.__name__), sensors.timer('mongo_save_session_time'):
                if sequence_number is None:
                    # update same document
                    self._collection.replace_one(query, doc, upsert=True)
                else:
                    self._collection.insert_one(doc)

                sensors.inc_counter('mongo_save_session')

        except PyMongoError as e:
            sensors.inc_counter('mongo_error', labels={'error': e.__class__.__name__})
            if self._ignore_mongo_errors:
                logger.warning('Error while saving session: \"%s\".', e)
            else:
                raise
