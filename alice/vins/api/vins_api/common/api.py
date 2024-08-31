# coding: utf-8

import falcon

from alice.vins.api_helper.error import error_handler, error_serialize
from alice.vins.api_helper.resources import PingResource
from vins_api.common.middleware import MetricsMiddleware, RTLogMiddleware
from rtlog import is_active as rtlog_is_active


def make_app():
    middleware = [MetricsMiddleware()]
    if rtlog_is_active():
        middleware.append(RTLogMiddleware())
    app = falcon.API(middleware=middleware)
    app.req_options.strip_url_path_trailing_slash = True
    app.add_error_handler(Exception, error_handler)
    app.set_error_serializer(error_serialize)
    app.add_route('/ping', PingResource())
    return app
