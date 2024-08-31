# coding: utf-8

import falcon
import logging
import traceback
import os

from vins_core.utils.metrics import sensors
from vins_core.utils.strings import smart_unicode
from alice.vins.api_helper.resources import PingResource, set_json_response, ValidationError
from vins_api.webim import settings
from vins_api.speechkit.resources import speechkit as speechkit_resources
from vins_api.webim.resources import webim as webim_resources
from vins_api.webim.resources import ocrm as ocrm_resources
from vins_api.webim.middleware.auth import IncomingBasicAuthMiddleware
from vins_api.webim.middleware.log import IOLogMiddleware
from vins_api.webim.middleware.request_id import MarketRequestIdMiddleware

logger = logging.getLogger(__name__)


def error_serialize(req, resp, exception):
    traceback_str = traceback.format_exc()

    data = {
        'message': smart_unicode(exception),
        'error': 'unhandled_error',
    }
    if os.environ.get('VINS_FEATURE_DEBUG') == '1':
        data['traceback'] = smart_unicode(traceback_str)

    if isinstance(exception, ValidationError):
        data['error'] = 'validation_error'
        set_json_response(resp, data, falcon.HTTP_400)
    elif isinstance(exception, falcon.HTTPError):
        data['error'] = exception.title
        data['message'] = exception.description
        set_json_response(resp, data, exception.status)
    else:
        logger.error('Internal Server Error: %s', req.path, exc_info=traceback_str)
        set_json_response(resp, data, falcon.HTTP_500)


def error_handler(ex, req, resp, params):
    sensors.inc_counter(
        'http_response_exceptions',
        labels={'type': ex.__class__.__name__},
    )
    sensors.storage.flush()
    error_serialize(req, resp, ex)


def make_app():
    middleware = [
        IncomingBasicAuthMiddleware(os.environ.get('CRMBOT_WEBIM_USER_DATABASE', '')),
        MarketRequestIdMiddleware(settings.REQUID_HEADER),
        IOLogMiddleware()
    ]
    app = falcon.API(middleware=middleware)
    app.req_options.strip_url_path_trailing_slash = True
    app.add_error_handler(Exception, error_handler)
    app.set_error_serializer(error_serialize)
    app.add_route('/ping', PingResource())

    app_resources = {
        'webim': webim_resources.WebimV1Resource(settings),
        'webim_v2': webim_resources.WebimV2Resource(settings),
        'ocrm': ocrm_resources.OCRMResource(settings),
        'ocrm_v2': ocrm_resources.OCRMResource2(settings),
    }

    app.add_route('/webim/{app_id}', app_resources['webim'])
    app.add_route('/webim/v1/{app_id}', app_resources['webim'])
    app.add_route('/webim/v2/{app_id}', app_resources['webim_v2'])
    app.add_route('/ocrm/{app_id}', app_resources['ocrm'])
    app.add_route('/ocrm/v2/{app_id}', app_resources['ocrm_v2'])

    if os.environ.get('VINS_ENABLE_CRMBOT_PA_INTERFACE') == '1':
        app_resources['sk'] = speechkit_resources.SKResource(settings)
        app.add_route('/speechkit/app/{app_id}', app_resources['sk'])

    return app, app_resources


app, app_resources = make_app()
