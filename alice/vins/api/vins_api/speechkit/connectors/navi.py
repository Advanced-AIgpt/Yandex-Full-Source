# coding: utf-8
from __future__ import unicode_literals

import logging

from urllib import quote

from vins_api.speechkit.connectors.speechkit import SpeechKitConnector
from vins_core.dm.response import ClientActionDirective, VinsResponse
from vins_core.dm.request_events import VoiceInputEvent
from vins_core.utils.strings import smart_unicode
from vins_core.utils.strings import smart_utf8
from navi_app.lib.navirequest import wrap_new_request

logger = logging.getLogger(__name__)


def build_directive(response, path, params, name='open_uri', scheme='yandexnavi://', sub_name_prefix='navi'):
    if not params:
        uri = scheme + path
    else:
        # urllib.urlencode uses urllib.quote_plus
        # Reassign use to urllib.quote is possible only in python3
        quoted_params = []
        for k, v in params.items():
            k = quote(smart_utf8(k), b'')
            v = quote(smart_utf8(v), b'')
            quoted_params.append(k + '=' + v)
        uri = ''.join([scheme, path, '?', '&'.join(quoted_params)])

    response.directives.append(ClientActionDirective(
        name=name,
        sub_name=sub_name_prefix + '_' + path.replace('/', '_'),
        payload={'uri': smart_unicode(uri)}
    ))


def _support_confirmation(additional_options):
    return 'confirmation' in additional_options.get('supported_features', [])


def _dont_change_action(data, additional_options, action, response):
    response.directives.append(action)


def _switch_layers(data, additional_options, action, response):
    if 'traffic' in data['slots']:
        build_directive(
            response=response,
            path='traffic',
            params={'traffic_on': 1 if data['command'] == 'on' else 0}
        )
    elif 'parking' in data['slots']:
        build_directive(
            response=response,
            path='show_ui/map',
            params={'carparks_enabled': 1 if data['command'] == 'on' else 0}
        )
    else:
        _dont_change_action(data, additional_options, action, response)


def _add_point(data, additional_options, action, response):
    params = {'category': data['category']}
    if data.get('lane', None):
        params['where'] = data['lane']
    if 'comment' in data:
        params['comment'] = data['comment']
    if _support_confirmation(additional_options):
        params['confirmation'] = 1

    build_directive(response=response, path='add_point', params=params)


def _confirmation(data, additional_options, action, response):
    build_directive(
        response=response,
        path='external_confirmation',
        params={'confirmed': 1 if data['type'] == 'confirmation' else 0}
    )


def _parking_route(data, additional_options, action, response):
    build_directive(response=response, path='carparks_route', params={})


def _show_me(data, additional_options, action, response):
    build_directive(response=response, path='show_user_position', params={})


def _route(data, additional_options, action, response):
    params = {}
    if data['from']:
        params['lat_from'] = data['from']['lat']
        params['lon_from'] = data['from']['lon']
    if data['via']:
        params['lat_via_0'] = data['via']['lat']
        params['lon_via_0'] = data['via']['lon']
    if data['to']:
        params['lat_to'] = data['to']['lat']
        params['lon_to'] = data['to']['lon']
    if _support_confirmation(additional_options):
        params['confirmation'] = '1'

    build_directive(response=response, path='build_route_on_map', params=params)


def _route_view(data, additional_options, action, response):
    build_directive(
        response=response,
        path='show_route_overview',
        params={}
    )


def _route_reset(data, additional_options, action, response):
    build_directive(
        response=response,
        path='clear_route',
        params={}
    )


def _search(data, additional_options, action, response):
    build_directive(
        response=response,
        path='map_search',
        params={'text': data['query']}
    )


class NotFoundDataInPayloadError(Exception):
    pass


class NaviResponse(object):
    _HANDLERS = {
        'dont_understand': _search,
        'do_nothing': _dont_change_action,
        'switch_layers': _switch_layers,
        'bug_report': _add_point,
        'add_point': _add_point,
        'add_talk': _add_point,
        'cancel': _confirmation,
        'confirmation': _confirmation,
        'parking_route': _parking_route,
        'show_me': _show_me,
        'route': _route,
        'route_view': _route_view,
        'route_reset': _route_reset,
        'search': _search,
    }

    def __init__(self, name, payload):
        self._name = name
        self._payload = payload

    def build_commands(self, response, action, req_info):
        if self._name not in self._HANDLERS:
            logger.error('Action %s not implemented yet' % self._name)
            raise NotImplementedError
        if 'data' not in self._payload:
            logger.error('Not found data block in payload %s' % self._payload)
            raise NotFoundDataInPayloadError

        self._HANDLERS[self._name](self._payload['data'], req_info.additional_options, action, response)


class NaviSKConnector(SpeechKitConnector):
    def handle_request(self, req_info, **kwargs):
        if req_info.utterance is None:
            response = VinsResponse()
        else:
            req_info = self._wrap_request_info(req_info, **kwargs)

            response = super(NaviSKConnector, self).handle_request(req_info, **kwargs)

            old_directives = response.directives
            response.directives = []
            for c in old_directives:
                if isinstance(c, ClientActionDirective):
                    NaviResponse(c.name, c.payload).build_commands(response, c, req_info)
                else:
                    response.directives.append(c)

        response.voice_text = None
        response.should_listen = False

        return response

    @staticmethod
    def _wrap_request_info(req_info, **kwargs):
        if isinstance(req_info.event, VoiceInputEvent):
            utterance_text = req_info.event.asr_utterance()
            event = VoiceInputEvent.from_utterance(utterance_text)
            req_info = req_info.replace(utterance=event.utterance, event=event)
        navigator_state = req_info.device_state.get('navigator')
        if navigator_state is not None:
            new_additional_options = wrap_new_request(navigator_state, req_info.location, req_info.lang)
            req_info.additional_options.update(new_additional_options)
        return req_info
