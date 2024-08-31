# coding: utf-8

import json
import falcon
import logging
import urllib

from jsonschema import validate as validate_json, ValidationError as SchemaValidationError

from vins_core.utils.data import delete_keys_from_dict, to_json_str
from vins_core.utils.logging import lazy_logging, REQUEST_FIELDS_BLACKLIST

logger = logging.getLogger(__name__)


class ValidationError(falcon.HTTPBadRequest):
    pass


def parse_json_request(req, schema=None):
    if not req.content_length:
        raise ValidationError('Empty request body.')

    try:
        content_type = req.content_type.split(';')[0]
        if content_type == 'application/x-www-form-urlencoded':
            data = json.loads(urllib.unquote_plus(req.stream.read()))
        elif content_type in ('application/json', ''):
            data = json.load(req.stream)
        else:
            raise ValidationError('Unsupported media type "%s" in request.' % content_type)
    except ValueError, e:
        raise ValidationError('Failed to parse json %s' % unicode(e))

    if schema:
        try:
            validate_json(data, schema)
        except SchemaValidationError as e:
            raise ValidationError(unicode(e))
    return data


def status2code(status):
    # http://falcon.readthedocs.io/en/stable/api/request_and_response.html#falcon.Response.status
    return str(status).partition(' ')[0]


def set_json_response(resp, data, status=falcon.HTTP_200, serialize_data=True):
    resp.set_header('Content-Type', 'application/json')
    resp.status = status
    if serialize_data:
        resp.body = to_json_str(data, ensure_ascii=False, indent=2)
    else:
        resp.body = data


def set_protobuf_response(resp, data, status=falcon.HTTP_200):
    resp.set_header('Content-Type', 'application/protobuf')
    resp.status = status
    resp.body = data


@lazy_logging
def dump_request_for_log(request):
    if isinstance(request, basestring):
        return request

    data = delete_keys_from_dict(request, REQUEST_FIELDS_BLACKLIST)
    return to_json_str(data, indent=2, ensure_ascii=False)


@lazy_logging
def dump_response_for_log(resp):
    if isinstance(resp, basestring):
        return resp

    return ' '.join([status2code(resp.status), resp.body])


class PingResource(object):
    def on_get(self, req, resp):
        resp.status = falcon.HTTP_200
        resp.body = 'Ok'
        resp.set_header('Content-Type', 'text/plain')
