# coding: utf-8

import base64

from vins_api.webim.resources.webim import BaseWebimResource
from alice.vins.api_helper.resources import ValidationError


class IncomingBasicAuthMiddleware(object):
    def __init__(self, accounts):
        self.known_users = accounts.splitlines()

    def process_request(self, req, resp):
        pass

    def process_resource(self, req, resp, resource, params):
        if not isinstance(resource, BaseWebimResource):
            return
        auth_content = req.get_header("Authorization", required=True)
        (auth_type, token) = auth_content.split(' ')
        if auth_type != "Basic":
            raise ValidationError('Unexpected auth token type')
        token_decoded = base64.b64decode(token)
        if token_decoded not in self.known_users:
            raise ValidationError('Unauthorized request')

    def process_response(self, req, resp, resource, req_succeeded):
        pass
