# coding: utf-8

from builtins import str
import json
from .utils import get_slots


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_geo')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


def get_action_route_string(address, company, home_work, point_type):
    point_to_str = {
        'from': _('от точки'),
        'to': _('до точки'),
        'via': _('через точку')
    }
    action = ''
    if 'what_{}_home'.format(point_type) in home_work:
        action += _('{} "Дом", указанной в настройках пользователя').format(point_to_str[point_type]) + '\n'
    elif 'what_{}_work'.format(point_type) in home_work:
        action += _('{} "Работа", указанной в настройках пользователя').format(point_to_str[point_type]) + '\n'
    else:
        action += '{} "{}"'.format(point_to_str[point_type], address)
        if company:
            action += ' ({})'.format(company)
        action += '\n'
    return action


def get_address_and_company(slot):
    address = None
    company = None
    if slot.get('typed_value') and slot['typed_value'].get('string') and slot['typed_value']['string'] != 'null':
        s = json.loads(slot['typed_value']['string'])
        if s.get('address_line'):
            address = s['address_line'].encode('utf-8')
        if s.get('company_name'):
            company = s['company_name'].encode('utf-8')
            if s.get('geo') and s['geo'].get('address_line'):
                address = s['geo']['address_line'].encode('utf-8')
    return address, company


def get_route_action(slots):
    home_work = []
    address_from, company_from, address_to, company_to, address_via, company_via, address, company = None, None, None, None, None, None, None, None
    for slot in slots:
        if slot.get('name') in ('what_from', 'what_to', 'what_via', 'what'):
            if slot.get('typed_value') and slot['typed_value'].get('string'):
                slot_value = slot['typed_value']['string']
                if slot_value in (u'"home"', u'"work"'):
                    home_work.append(slot['name'] + '_' + str(slot['typed_value']['string']).strip('\"'))
        if slot.get('name') == 'resolved_location_from':
            address_from, company_from = get_address_and_company(slot)
        if slot.get('name') == 'resolved_location_to':
            address_to, company_to = get_address_and_company(slot)
        if slot.get('name') == 'resolved_location_via':
            address_via, company_via = get_address_and_company(slot)
        if slot.get('name') == 'last_found_poi':
            address, company = get_address_and_company(slot)

    if address_from or address_to or address_via:
        action = _('Строится маршрут') + '\n'
        if address_from:
            action += get_action_route_string(address_from, company_from, home_work, point_type='from')
        if address_to:
            action += get_action_route_string(address_to, company_to, home_work, point_type='to')
        if address_via:
            action += get_action_route_string(address_via, company_via, home_work, point_type='via')
        return action.strip()

    if address:
        action = _('Открывается точка на карте ')
        if 'what_home' in home_work:
            action += _('"Дом", указанная в настройках пользователя')
        elif 'what_work' in home_work:
            action += _('"Работа", указанная в настройках пользователя')
        else:
            action += '"{}"'.format(address)
            if company:
                action += ' ({})'.format(company)
        return action


def get_route_for_state(state):
    slots = get_slots(state)
    if slots:
        route_action = get_route_action(slots)
        if route_action:
            return route_action
