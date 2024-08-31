# coding: utf-8
from __future__ import unicode_literals

import logging

from functools import partial
from multiprocessing.dummy import Pool

from vins_core.dm.response import ClientActionDirective

from navi_app.lib.geosearch import gps_distance, GeosearchAPIError
from navi_app.lib.navirequest import normalize_lang
from navi_app.lib.localization import COMMANDS_TRANSLATIONS
from navi_app.lib.util import find_geo_targets

logger = logging.getLogger(__name__)
callbacks_map = {}

POINT_TYPES = {
    'accident': 0,
    'roadworks': 1,
    'camera': 2,
    'other': 3,
    'closed': 4,
    'bridges': 5,
    'talks': 6,
    'feedback': 7,
}
NEIGHBORHOOD_DISTANCE = 30.


def _too_far_from(target, req_info):
    return gps_distance(
        target['lat'], target['lon'],
        req_info.location['lat'], req_info.location['lon']
    ) > NEIGHBORHOOD_DISTANCE


def _append_geo_description(description, geo, name, req_info):
    if geo is None:
        return description

    description = description + ' ' if description else ''  # no separator for empty descr
    if normalize_lang(req_info.lang) == "tr":
        return description + "'%s' %s" % (geo['name'], COMMANDS_TRANSLATIONS[normalize_lang(req_info.lang)][name])
    elif _too_far_from(geo, req_info) and geo['description'] != 'Россия' and not geo['is_favourite']:
        return description + "%s '%s (%s)'" % (
            COMMANDS_TRANSLATIONS[normalize_lang(req_info.lang)][name],
            geo['name'],
            geo['description']
        )
    else:
        return description + "%s '%s'" % (COMMANDS_TRANSLATIONS[normalize_lang(req_info.lang)][name], geo['name'])


def _find_any_geo(req_info, text):
    if text is None or not text:
        return None
    try:
        items = find_geo_targets(text, req_info)
        return items[0] if len(items) > 0 else None
    except GeosearchAPIError:
        logger.warning('Failed to find any geo results for query "%s"', text, exc_info=True)
        return None


def _search_response(search_description, query):
    description = "%s '%s'" % (search_description, query)
    return ClientActionDirective(name='search', sub_name='navi_search', payload={
        'data': {
            'query': query,
            'description': description
        },
        'text': description
    })


def _route_response(geo_targets, description):
    return ClientActionDirective(name='route', sub_name='navi_route', payload={
        'data': {
            'from': geo_targets[0],
            'to': geo_targets[1],
            'via': geo_targets[2],
            'description': description
        },
        'text': description
    })


def _fallback_to_search(lang, geo_searches, utterance):
    for geo in geo_searches:
        if geo is not None:
            return _search_response(COMMANDS_TRANSLATIONS[lang]['search'], geo)
    return _search_response(COMMANDS_TRANSLATIONS[lang]['search'], utterance)


def _should_fallback_to_route(geo, req_info):
    if len(geo) == 0:
        return False

    # change search to route if we have route info and the route is empty and user searched address
    # https://st.yandex-team.ru/DIALOG-549
    if req_info.additional_options.get('route', None) is not None:
        if len(req_info.additional_options['route']) == 0 and (geo[0]['is_address'] or geo[0]['is_favourite']):
            return True
    else:
        # change search to route if query is one of user's favourites
        if geo[0]['is_favourite']:
            return True
    return False


def map_search(form, req_info, sample, **kwargs):
    """
    Process commands that show results on the map. If only one result given, then it falls back to route callback.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    if sample.text in ['', ' ']:
        logger.warning('Empty text in sample, falling back to dont_understand')
        return simple_directive(req_info, 'dont_understand')

    lang = normalize_lang(req_info.lang)

    if 'query' not in form or not form.query.value:
        text = sample.text
    else:
        text = form.query.value

    try:
        geo = find_geo_targets(text, req_info)
    except GeosearchAPIError:
        logger.warning('Failed to find any geo results for query "%s"', text, exc_info=True)
        geo = []

    if _should_fallback_to_route(geo, req_info):
        description = "%s" % (COMMANDS_TRANSLATIONS[lang]['go'])
        description = _append_geo_description(description, geo[0], 'to', req_info)
        return _route_response([None, geo[0], None], description)
    else:
        return _search_response(COMMANDS_TRANSLATIONS[lang]['search'], text)


def route(form, req_info, sample, **kwargs):
    """
    Create or modify route.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    lang = normalize_lang(req_info.lang)
    geo_searches = [form.location_from.value, form.location_to.value, form.location_via.value]

    if all(geo is None for geo in geo_searches):
        logger.warning('All location slots are empty. Falling back to search with query "%s"', sample.text)
        return _fallback_to_search(lang, geo_searches, sample.text)

    pool = Pool(processes=3)
    geo_targets = pool.map(
        partial(_find_any_geo, req_info),
        geo_searches
    )
    pool.close()

    if all(t is None for t in geo_targets):
        logger.warning('Falling back to search with query "%s"', sample.text)
        return _fallback_to_search(lang, geo_searches, sample.text)

    description = COMMANDS_TRANSLATIONS[lang]['go']
    description = _append_geo_description(description, geo_targets[0], 'from', req_info)
    description = _append_geo_description(description, geo_targets[1], 'to', req_info)
    description = _append_geo_description(description, geo_targets[2], 'via', req_info)
    return _route_response(geo_targets, description)


def confirmation(form, req_info, **kwargs):
    """
    Process confirmation in any situation.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    return ClientActionDirective(name='confirmation', sub_name='navi_confirmation', payload={
        'data': {'type': 'confirmation'},
        'text': COMMANDS_TRANSLATIONS[normalize_lang(req_info.lang)]['confirmation']
    })


def cancel(form, req_info, **kwargs):
    """
    Process canceling action.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    return ClientActionDirective(name='cancel', sub_name='navi_cancel', payload={
        'data': {'type': 'cancel'},
        'text': COMMANDS_TRANSLATIONS[normalize_lang(req_info.lang)]['cancel']
    })


def add_point(form, req_info, sub_intent=None, **kwargs):
    """
    Process adding point or event on the map.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    :return:
    """
    lang = normalize_lang(req_info.lang)
    lane = None
    comment = None
    sub_intent = sub_intent or form.name.split('_')[-1]

    for slot in form.slots:
        if slot.value and slot.name == 'comment':
            comment = slot.value
        elif slot.value and slot.name == 'lane':
            if slot.value in COMMANDS_TRANSLATIONS[lang]:
                lane = COMMANDS_TRANSLATIONS[lang][slot.value]
            else:
                lane = slot.value

    description = "%s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['report_point'], COMMANDS_TRANSLATIONS[lang][sub_intent])

    if comment is None:
        comment = ''
    else:
        description += ", %s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['with_comment'], comment)

    if lane:
        description += ", '%s'" % (lane)

    return ClientActionDirective(name='add_point', sub_name='navi_add_point', payload={
        'data': {
            'description': description,
            'comment': comment,
            'category': str(POINT_TYPES[sub_intent]),
            'lane': lane
        },
        'text': description
    })


def add_talk(form, req_info, **kwargs):
    """
    Process adding talks on the map.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    lang = normalize_lang(req_info.lang)
    description = "%s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['report_point'], COMMANDS_TRANSLATIONS[lang]['talks'])
    if form.talk_text.value:
        comment = form.talk_text.value or ''
        description += ", %s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['with_comment'], comment)
    else:
        comment = ''

    return ClientActionDirective(name='add_talk', sub_name='navi_add_talk', payload={
        'data': {
            'description': description,
            'comment': comment,
            'category': str(POINT_TYPES['talks'])
        },
        'text': description
    })


def bug_report(form, req_info, sub_intent=None, **kwargs):
    """
    Process adding bug reports about errors on the map.
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    lang = normalize_lang(req_info.lang)
    sub_intent = sub_intent or form.name.split('_')[-1]
    description = "%s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['report_point'], COMMANDS_TRANSLATIONS[lang]['feedback'])
    comment = COMMANDS_TRANSLATIONS[lang].get(sub_intent, kwargs['sample'].text)
    description += ", %s: '%s'" % (COMMANDS_TRANSLATIONS[lang]['with_comment'], comment)

    return ClientActionDirective(name='add_point', sub_name='navi_add_point', payload={
        'data': {
            'description': description,
            'comment': comment,
            'category': str(POINT_TYPES['feedback']),
        },
        'text': description
    })


def switch_layers(form, req_info, action=None, **kwargs):
    """
    Process showing map layers (e.g. traffic, cameras, accidents, e.t.c)
    :param form: vins_core.dm.form_filler.models.From
    :param req_info: vins_core.dm.request.ReqInfo
    :return: ClientActionDirective
    """
    action = action or form.name.split('_')[-1]
    lang = normalize_lang(req_info.lang)
    query = kwargs['sample'].text
    fallback = _search_response(COMMANDS_TRANSLATIONS[lang]['search'], query)

    layers = set()
    slots = {}
    for slot in form.slots:
        if slot.value:
            if slot.name in COMMANDS_TRANSLATIONS[lang]:
                slots[slot.name] = COMMANDS_TRANSLATIONS[lang][action] + ' ' + COMMANDS_TRANSLATIONS[lang][slot.name]
                layers.add(COMMANDS_TRANSLATIONS[lang][slot.name])
            else:
                layers.add(slot.name)

    if len(slots) == 0:
        return fallback

    return ClientActionDirective(name='switch_layers', sub_name='navi_switch_layers', payload={
        'data': {
            'slots': slots,
            'command': 'on' if action == 'show' else 'off',
            'query': fallback.payload['data']['query'],
            'description': fallback.payload['data']['description'],
        },
        'text': "%s '%s'" % (COMMANDS_TRANSLATIONS[lang][action], ', '.join(layers)),
    })


def simple_directive(req_info, action):
    lang = normalize_lang(req_info.lang)
    return ClientActionDirective(name=action, sub_name='navi_' + action, payload={
        'data': {
            'query': req_info.utterance.text,
            'description': "%s '%s'" % (COMMANDS_TRANSLATIONS[lang]['search'], req_info.utterance.text),
        },
        'text': "%s" % (COMMANDS_TRANSLATIONS[lang][action]),
    })


def simple_action(form, req_info, **kwargs):
    return simple_directive(req_info, form.name.split('.')[-1])


def register_callback(name, func):
    def act_and_say(response=None, **kwargs):
        action = func(response=response, **kwargs)
        response.directives.append(action)
        response.say(action.payload['text'])

    callbacks_map[name] = act_and_say


register_callback('map_search', map_search)
register_callback('route', route)
register_callback('confirmation', confirmation)
register_callback('cancel', cancel)
register_callback('add_point', add_point)
register_callback('add_talk', add_talk)
register_callback('bug_report', bug_report)
register_callback('switch_layers', switch_layers)
register_callback('simple_action', simple_action)
