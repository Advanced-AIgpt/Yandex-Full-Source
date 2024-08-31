# coding: utf-8
from __future__ import unicode_literals

from collections import namedtuple

from urlparse import urlparse
from urlparse import parse_qs

import logging

from vins_core.dm.response import ClientActionDirective

logger = logging.getLogger(__name__)

# Intent name prefixes
APP_INTENT_PREFIX = 'personal_assistant.'
MICROINTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'handcrafted.'
AUTOAPP_MICROINTENT_PREFIX = MICROINTENT_NAME_PREFIX + 'autoapp.'
SCENARIOS_NAME_PREFIX = APP_INTENT_PREFIX + 'scenarios.'
NAVIGATOR_INTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'navi.'

CONFIRMATION_YES = AUTOAPP_MICROINTENT_PREFIX + 'confirmation_yes'
CONFIRMATION_NO = AUTOAPP_MICROINTENT_PREFIX + 'confirmation_no'

AUTOMOTIVE_INTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'automotive.'
AUTOMOTIVE_GREETING = AUTOMOTIVE_INTENT_NAME_PREFIX + 'greeting'
TEACH_ME_INTENT = SCENARIOS_NAME_PREFIX + 'teach_me'
DO_NOT_UNDERSTAND_INTENT = SCENARIOS_NAME_PREFIX + 'do_not_understand'

APPS = {'navi', 'yandexradio', 'yandexbrowser', 'yandexmusic', 'calls', 'contacts', 'application'}
WIDGETS = {'way', 'clock', 'radio', 'music', 'weather', 'auto', 'parking'}


def _form_to_params(form):
    return {s.name: s.value for s in form.slots if s.value is not None}


def _build_directive(app, intent, form=None, params=None):
    payload = {
        'application': app,
        'intent': intent,
        'params': {}
    }
    if form is not None:
        payload['params'].update(_form_to_params(form))
    if params is not None:
        payload['params'].update(params)
    return ClientActionDirective(
        name=app,
        sub_name=app + '_' + intent,
        payload=payload
    )


def _build_conf(confirmed):
    return _build_directive(
        'yandexnavi', 'external_confirmation',
        params={'app': 'yandexnavi', 'confirmed': '1' if confirmed else '0'})


FLOAT_KEYS = {'lat_to', 'lon_to'}
INT_KEYS = {'traffic_on'}

OpenUri = namedtuple('OpenUri', 'scheme intent params')


def _parse_param(key, val):
    if key in INT_KEYS:
        try:
            return int(val)
        except ValueError:
            logger.warning('failed to convert {} to int, val: {}'.format(key, val))
    if key in FLOAT_KEYS:
        try:
            return float(val)
        except ValueError:
            logger.warning('failed to convert {} to float, val: {}'.format(key, val))

    if isinstance(val, unicode):
        return val

    return unicode(val, 'utf-8')


def _parse_open_uri(directive, intent):
    uri_str = directive.payload['uri']
    # Uri is unicode and unquoting urlencoded unicode turns it to garbage.
    # So, if string can be converted to ascii we consider it to be urlencoded
    # and make params unicode after.
    if all(ord(char) < 128 for char in uri_str):
        uri_str = uri_str.encode('ascii')

    uri = urlparse(uri_str)
    query = parse_qs(uri.query)
    intent = uri.netloc
    params = dict()
    for k, v in query.items():
        params[k] = _parse_param(k, v[0])
    return OpenUri(uri.scheme, intent, params)


DEFAULT_STATION = 'Монте-Карло'


def _conv_radio(directives, intent, additional_options):
    def _conv(open_uri):
        intent = open_uri.intent
        params = open_uri.params
        radiostations = []
        if 'radiostations' in additional_options:
            radiostations = additional_options['radiostations']
        if intent == 'fm_radio':
            if 'name' in params:
                radio = params['name']
                if radio not in radiostations:
                    return _teach_me()
                new_intent = 'media_select'
                new_params = {'radio': radio}
            else:
                new_intent = 'launch'
                new_params = {'widget': 'radio'}
        elif intent == 'media_control':
            if DEFAULT_STATION in radiostations:
                new_intent = 'media_select'
                new_params = {'radio': DEFAULT_STATION}
            else:
                new_intent = 'launch'
                new_params = {'widget': 'radio'}
        else:
            new_intent = 'launch'
            new_params = {'app': 'yandexradio'}
        return _build_directive('car', new_intent, params=new_params)

    new_directives = list()
    for d in directives:
        if d.name == 'open_uri':
            open_uri = _parse_open_uri(d, intent)
            new_d = _conv(open_uri)
            if isinstance(new_d, tuple):
                return new_d  # change_form case
            if new_d is not None:
                new_directives.append(new_d)

    return None, new_directives


def _conv_music(directives, intent, *args):
    for d in directives:
        if d.name == 'open_uri':
            if u'radio.yandex.ru/user/onyourwave' in d.payload['uri']:
                return None, [_build_directive('car', 'launch', params={'app': 'yandexradio'})]
    return _teach_me()


NAVI_SUPPORTED_INTENTS = {
    'map_search',
    'build_route_on_map',
    'add_point',
    'traffic',
    'show_route_overview',
    'show_user_position',
    'set_place'
}


def _conv_navi(directives, intent, *args):
    def _conv(open_uri):
        if open_uri.scheme == 'yandexnavi' and open_uri.intent in NAVI_SUPPORTED_INTENTS:
            return _build_directive(open_uri.scheme, open_uri.intent, params=open_uri.params)
        return None

    new_directives = list()
    for d in directives:
        if d.name == 'open_uri':
            open_uri = _parse_open_uri(d, intent)
            new_d = _conv(open_uri)
            if new_d is not None:
                new_directives.append(new_d)

    return None, new_directives


def _conv_open_site_or_app(directives, intent, *args):
    def _conv(open_uri):
        if 'name' not in open_uri.params:
            return None
        app_or_widget = open_uri.params['name']
        if app_or_widget not in APPS and app_or_widget not in WIDGETS:
            return None
        params = {'app': app_or_widget} if app_or_widget in APPS else {'widget': app_or_widget}
        return _build_directive('car', 'launch', params=params)

    new_directives = list()
    for d in directives:
        if d.name == 'open_uri':
            open_uri = _parse_open_uri(d, intent)
            new_d = _conv(open_uri)
            if new_d is not None:
                new_directives.append(new_d)

    return None, new_directives


def _conv_player(directives, intent, *args):
    new_directives = list()
    for d in directives:
        if d.name == 'open_uri':
            open_uri = _parse_open_uri(d, intent)
            if 'action' in open_uri.params:
                action = open_uri.params['action']
                if action == 'play':
                    action = 'on'
                params = {'action': action}
                new_directives.append(_build_directive('car', 'media_control', params=params))

    return None, new_directives


def _conv_sound(directives, intent, *args):
    new_directives = list()
    for d in directives:
        if d.name == 'open_uri':
            open_uri = _parse_open_uri(d, intent)
            if 'action' in open_uri.params:
                action = open_uri.params['action']
                new_directives.append(_build_directive('car', action))

    return None, new_directives


# Not expecting any directives, just some text answer.
def _conv_none(*args):
    return None, []


def _conf_yes(*args):
    return None, [_build_conf(True)]


def _conf_no(*args):
    return None, [_build_conf(False)]


def _dont_understand(*args):
    return DO_NOT_UNDERSTAND_INTENT, list()


def _teach_me(*args):
    return TEACH_ME_INTENT, list()


def _greeting(*args):
    return AUTOMOTIVE_GREETING, list()


_HANDLERS = {
    SCENARIOS_NAME_PREFIX + 'radio_play': _conv_radio,
    SCENARIOS_NAME_PREFIX + 'music_play': _conv_music,

    SCENARIOS_NAME_PREFIX + 'open_site_or_app': _conv_open_site_or_app,

    SCENARIOS_NAME_PREFIX + 'sound_louder': _conv_sound,
    SCENARIOS_NAME_PREFIX + 'sound_louder__ellipsis': _conv_sound,
    SCENARIOS_NAME_PREFIX + 'sound_quiter': _conv_sound,
    SCENARIOS_NAME_PREFIX + 'sound_quiter__ellipsis': _conv_sound,
    SCENARIOS_NAME_PREFIX + 'sound_mute': _teach_me,
    SCENARIOS_NAME_PREFIX + 'sound_unmute': _teach_me,

    SCENARIOS_NAME_PREFIX + 'player_next_track': _conv_player,
    SCENARIOS_NAME_PREFIX + 'player_previous_track': _conv_player,
    SCENARIOS_NAME_PREFIX + 'player_pause': _conv_player,
    SCENARIOS_NAME_PREFIX + 'player_continue': _conv_player,
    # Navi
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point': _conv_navi,
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point__cancel': _conv_navi,
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point__ellipsis': _conv_navi,
    NAVIGATOR_INTENT_NAME_PREFIX + 'change_voice': _conv_navi,
    NAVIGATOR_INTENT_NAME_PREFIX + 'change_voice__ellipsis': _conv_navi,
    NAVIGATOR_INTENT_NAME_PREFIX + 'hide_layer': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'how_long_to_drive': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'how_long_traffic_jam': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'parking_route': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'reset_route': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'show_layer': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'show_route_on_map': _teach_me,
    NAVIGATOR_INTENT_NAME_PREFIX + 'when_we_get_there': _teach_me,

    SCENARIOS_NAME_PREFIX + 'convert': _conv_none,
    SCENARIOS_NAME_PREFIX + 'convert__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'convert__get_info': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_date': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_date__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_my_location': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_my_location__details': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_news': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_news__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_news__more': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_time': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_time__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_weather': _conv_none,
    SCENARIOS_NAME_PREFIX + 'get_weather__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'music_sing_song': _conv_none,
    SCENARIOS_NAME_PREFIX + 'music_sing_song__next': _conv_none,
    SCENARIOS_NAME_PREFIX + 'random_num': _conv_none,

    SCENARIOS_NAME_PREFIX + 'get_my_location': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'search': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'poi_general': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__details': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__ellipsis': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__by_index': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__next': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__prev': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'find_poi__show_on_map': _conv_navi,

    SCENARIOS_NAME_PREFIX + 'show_route': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'show_route__ellipsis': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'show_route__show_route_on_map': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'show_traffic': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'show_traffic__details': _conv_navi,
    SCENARIOS_NAME_PREFIX + 'remember_named_location': _conv_none,
    SCENARIOS_NAME_PREFIX + 'remember_named_location__ellipsis': _conv_none,
    SCENARIOS_NAME_PREFIX + 'confirmation_yes': _conf_yes,
    SCENARIOS_NAME_PREFIX + 'confirmation_no': _conf_no,
    SCENARIOS_NAME_PREFIX + 'bluetooth_on': _teach_me,
    SCENARIOS_NAME_PREFIX + 'bluetooth_off': _teach_me,

    SCENARIOS_NAME_PREFIX + 'repeat': _conv_none,

    MICROINTENT_NAME_PREFIX + 'tell_about_yandex_auto': _conv_none,
    MICROINTENT_NAME_PREFIX + 'hello': _greeting,
    AUTOAPP_MICROINTENT_PREFIX + 'greeting': _conv_none,
    AUTOAPP_MICROINTENT_PREFIX + 'internet_appeared': _conv_none,
    AUTOAPP_MICROINTENT_PREFIX + 'tired': _conv_none,
    AUTOAPP_MICROINTENT_PREFIX + 'missed': _conv_none,

    CONFIRMATION_YES: _conf_yes,
    CONFIRMATION_NO: _conf_no,
    DO_NOT_UNDERSTAND_INTENT: _conv_none,
    TEACH_ME_INTENT: _conv_none,
    AUTOMOTIVE_GREETING: _conv_none,
    SCENARIOS_NAME_PREFIX + 'taxi_new_order': _conv_none,
    SCENARIOS_NAME_PREFIX + 'taxi_new_disabled': _conv_none,
    SCENARIOS_NAME_PREFIX + 'common.irrelevant': _conv_none,
}


def autoapp_allowed_intents():
    return frozenset(_HANDLERS.keys())


def is_autoapp_teach_me_intent(form):
    intent = form.name
    return intent in _HANDLERS and _HANDLERS[intent] == _teach_me


def to_autoapp_directives(directives, intent, req_info):
    logger.debug('Got into AutoApp translate with intent {}'.format(intent))

    if intent in _HANDLERS:
        conv = _HANDLERS[intent]
    else:
        return _dont_understand()

    new_intent, new_directives = conv(directives, intent, req_info.additional_options or {})

    # if there were some directives and we failed to convert any -  reply with "teach_me".
    if not new_intent and intent != TEACH_ME_INTENT and len(new_directives) == 0 and len(directives) > 0:
        return _teach_me()

    return new_intent, new_directives


def autoapp_add_navi_state(req_info):
    favs = list(req_info.additional_options.get('favourites') or [])
    favs = [fav for fav in favs if 'title' in fav]
    if len(favs) == 0:
        return req_info

    navi = dict()
    new_favs = list()
    for fav in favs:
        fav = fav.copy()
        name = fav.pop('title', None)
        fav['name'] = name
        if name.lower() == 'home':
            navi['home'] = fav
        elif name.lower() == 'work':
            navi['work'] = fav
        else:
            new_favs.append(fav)
    if len(new_favs) > 0:
        navi['user_favorites'] = new_favs
    return req_info.replace(device_state={'navigator': navi})
