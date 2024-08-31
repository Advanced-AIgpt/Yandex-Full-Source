# coding: utf-8

import attr
import base64
import logging

from lxml import etree

logger = logging.getLogger(__name__)

HOME_PLACES = ['дом', 'домой', 'дача', 'дачу', 'home', 'maison']
WORK_PLACES = ['iş', 'работа', 'работу', 'work', 'travail']

ALLOWED_LANGS = {'ru', 'uk', 'en', 'tr', 'fr'}


def normalize_lang(lang):
    lang = lang.split('-')[0].split('_')[0].lower()
    if lang not in ALLOWED_LANGS:
        lang = 'en'
    return lang


@attr.s
class Favourite(object):
    name = attr.ib()
    lat = attr.ib()
    lon = attr.ib()


@attr.s
class Context(object):
    lang = attr.ib(default=None)
    lat = attr.ib(default=None)
    lon = attr.ib(default=None)
    tl_lat = attr.ib(default=None)
    tl_lon = attr.ib(default=None)
    br_lat = attr.ib(default=None)
    br_lon = attr.ib(default=None)
    favourites = attr.ib(default=None)
    home_favourites = attr.ib(default=None)
    work_favourites = attr.ib(default=None)
    broken = attr.ib(default=False)
    commands = attr.ib(default=attr.Factory(set))
    current_route = attr.ib(default=None)

    def to_dict(self):
        return attr.asdict(self)


def _is_wait_for_confirmation(xml):
    value = xml.find('speech').get('wait_for_confirmation')
    return value is not None and value.strip().lower() in ('true', '1')


def _normalize_text(text, lang):
    text = text.strip().lower().replace(',', '')
    if not text or text == '+' or text == '-':
        return None

    if lang == 'tr':
        text = text.replace(u'giderim', u'gidelim')
    elif lang == 'uk':
        text = text.replace(u'\u0069', '\u0456')  # replace wrong i i -> і
    return text


def _parse_xml(xml_string, context):
    parser = etree.XMLParser(resolve_entities=False, dtd_validation=False)
    xml = etree.fromstring(xml_string, parser=parser)

    cases = []
    for alt in xml.find('speech').findall('alternative'):
        if not alt.text:
            continue

        text = _normalize_text(alt.text, context.lang)
        if text:
            cases.append((text, float(alt.get('confidence'))))

    return xml, cases


def _get_commands(xml):
    command_section = xml.find('client/commands')
    if command_section is not None:
        return {c.strip() for c in command_section.text.split(',')}
    else:
        return {}


def _get_current_route(xml):
    locations = []

    if xml.find('context') is not None:
        route_section = xml.find('context/route')
        if route_section is not None:
            locations = [{'lat': location.get('lat'), 'lon': location.get('lon')}
                         for location in route_section.getchildren()]

    return {'points': locations} if len(locations) > 0 else {}


def _fill_favourites(xml, context):
    favourites = [Favourite(name=fav.get('name'), lat=float(fav.get('lat')), lon=float(fav.get('lon')))
                  for fav in xml.find('favourites').findall('fav')]

    context.favourites = favourites
    context.home_favourites = next(iter([x for x in favourites if x.name.lower().strip() in HOME_PLACES]), None)
    context.work_favourites = next(iter([x for x in favourites if x.name.lower().strip() in WORK_PLACES]), None)


def parse_request(lang, data, default_text=''):
    context = Context()

    try:
        context.favourites = []
        context.lang = normalize_lang(lang)
        context.lat, context.lon = (float(i.strip()) for i in data['ll'].split(','))
        context.tl_lat = float(data['tl_lat'])
        context.tl_lon = float(data['tl_lon'])
        context.br_lat = float(data['br_lat'])
        context.br_lon = float(data['br_lon'])

        xml, cases = _parse_xml(base64.b64decode(data['xml']), context)
        context.wait_for_confirmation = _is_wait_for_confirmation(xml)
        text = cases[0][0] if len(cases) > 0 else default_text
    except Exception:
        logger.exception('Failed to understand request')
        context.broken = True
        return default_text, context.to_dict()

    try:
        _fill_favourites(xml, context)
        context.commands = _get_commands(xml)
        context.current_route = _get_current_route(xml)
    except Exception:
        logger.exception('Failed to parse favourites')
        context.broken = True
        return text, context.to_dict()

    if len(cases) == 0:
        logger.warning('Utterance is empty, using "%s"' % default_text)

    return text, context.to_dict()
