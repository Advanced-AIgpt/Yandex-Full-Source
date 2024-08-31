# coding: utf-8

import logging
import traceback

import falcon
from falcon.util import uri

from alice.vins.api_helper.resources import ValidationError, set_json_response
from vins_core.dm.session import MongoSessionStorage, DummySessionStorage
from vins_core.utils.config import get_bool_setting
from vins_core.utils.metrics import RedisMetricsStorage
from vins_sdk.connectors import ConnectorBase
from vins_api.common.db import get_redis_connection, get_db_connection

from vins_api.common.vins_apps import create_app

logger = logging.getLogger(__name__)


def parse_urlencoded_request(req, body=False):
    if body:
        urlencoded_str = req.stream.read()
    else:
        urlencoded_str = req.query_string

    try:
        urlencoded_str = urlencoded_str.decode('utf-8')
    except UnicodeDecodeError:
        raise ValidationError('Invalid request body, need non empty form urlencoded')

    params = uri.parse_query_string(uri.decode(urlencoded_str))
    logger.debug('Params: %r', params)
    return params


def process_exception(self, req, resp, exception):
    traceback_str = traceback.format_exc()

    data = {
        'message': unicode(exception),
        'error': 'unhandled_error',
        'traceback': traceback_str,
    }

    if isinstance(exception, ValidationError):
        data['error'] = 'validation_error'
        set_json_response(resp, data, falcon.HTTP_400)
    else:

        logger.error(
            'Internal Server Error: %s', req.path,
            exc_info=traceback_str,
        )
        set_json_response(resp, data, falcon.HTTP_500)


class BaseConnectedAppResource(object):
    connector_cls = ConnectorBase
    storage_cls = MongoSessionStorage
    use_dummy_storages = False

    def __init__(self, settings):
        self._connected_apps = {}
        self._settings = settings

    def _get_storage(self, app_conf):
        use_dummy_storages = get_bool_setting('USE_DUMMY_STORAGES') or self.use_dummy_storages
        if use_dummy_storages:
            logger.info("Using Dummy storages.")
            return DummySessionStorage()
        else:
            db = get_db_connection(self._settings)

            ignore_mongo_errors = app_conf.get('ignore_mongo_errors', False)
            session_storage = self.storage_cls(db.sessions, ignore_mongo_errors=ignore_mongo_errors)
            return session_storage

    def get_or_create_connected_app(self, app_id):
        app_conf = self._settings.CONNECTED_APPS.get(app_id)
        if not app_conf:
            raise falcon.HTTPNotFound(
                description='Not found app with id=%s' % app_id
            )

        if app_id not in self._connected_apps:
            session_storage = self._get_storage(app_conf)
            app = create_app(app_id, app_conf, session_storage)
            self._connected_apps[app_id] = self.connector_cls(
                vins_app=app
            )
        return self._connected_apps[app_id]

    def set_app(self, app_id, app):
        self._connected_apps[app_id] = self.connector_cls(vins_app=app)


class SolomonMetricsResorce(object):
    def __init__(self, settings):
        self._settings = settings
        self._metrics_storage = None

    def _get_state(self):
        if self._metrics_storage is None:
            redis = get_redis_connection()
            self._metrics_storage = RedisMetricsStorage(
                redis, self._settings.VINS_METRICS_CONF
            )

        return self._metrics_storage.get_state()

    def on_get(self, request, resp):
        sensors = []
        for metric in self._get_state():
            kind = self._metrics_storage.conf.get_kind(metric.name)
            res = {
                'labels': dict(metric.labels, name=metric.name),
                'value': metric.value,
                'kind': kind,
            }
            ts = int(metric.time)
            if ts:
                res['ts'] = ts

            sensors.append(res)

        set_json_response(resp, {'sensors': sensors})

    def set_app(self, app_id, app):
        pass
