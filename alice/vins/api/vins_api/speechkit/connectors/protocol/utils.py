# coding: utf-8

from __future__ import unicode_literals

import logging

import attr

from alice.vins.api.vins_api.speechkit.connectors.protocol.protos.state_pb2 import TState

from google.protobuf import struct_pb2

logger = logging.getLogger(__name__)

_OAUTH_PREFIX = 'OAuth '


@attr.s()
class Headers(object):
    oauth_token = attr.ib(default=None)
    user_ticket = attr.ib(default=None)

    @classmethod
    def from_dict(cls, headers):
        kwargs = {}

        oauth_token = headers.get('X-OAUTH-TOKEN')
        if oauth_token is not None:
            if oauth_token.startswith(_OAUTH_PREFIX):
                kwargs['oauth_token'] = oauth_token[len(_OAUTH_PREFIX):]
            else:
                logger.warning('Invalid oauth token: %s' % oauth_token)

        kwargs['user_ticket'] = headers.get('X-YA-USER-TICKET')

        return cls(**kwargs)


def create_state(req_info, vins_response):
    state = TState()

    req_id = req_info.request_id
    if req_id:
        state.PrevReqId = req_id

    sessions = vins_response.sessions or {}
    session = sessions.get(req_info.dialog_id or '')
    if session:
        state.Session = session

    return state


def unpack_state(state_data):
    state = TState()
    if state_data.Is(struct_pb2.Value.DESCRIPTOR):
        value = struct_pb2.Value()
        state_data.Unpack(value)
        state.Session = value.string_value
    else:
        state_data.Unpack(state)
    return state
