# coding: utf-8

import logging
import traceback
import falcon

from vins_core.utils.metrics import sensors
from vins_core.utils.strings import smart_unicode
from alice.vins.api_helper.resources import set_json_response, ValidationError

logger = logging.getLogger(__name__)


def error_serialize(req, resp, exception):
    traceback_str = traceback.format_exc()

    data = {
        'message': smart_unicode(exception),
        'error': 'unhandled_error',
        'traceback': smart_unicode(traceback_str),
    }

    if isinstance(exception, falcon.HTTPError):
        data['error'] = exception.title
        data['message'] = exception.description
        set_json_response(resp, data, exception.status)
    elif isinstance(exception, ValidationError):
        data['error'] = 'validation_error'
        set_json_response(resp, data, falcon.HTTP_400)
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
