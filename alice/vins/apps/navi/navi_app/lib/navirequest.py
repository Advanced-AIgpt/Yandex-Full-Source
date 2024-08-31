# coding: utf-8
from __future__ import unicode_literals

import attr
import logging

from localization import FAV_PLACES

logger = logging.getLogger(__name__)

ALLOWED_LANGS = {'ru', 'uk', 'en', 'tr', 'fr'}


def normalize_lang(lang):
    lang = lang.split('-')[0].split('_')[0].lower()
    if lang not in ALLOWED_LANGS:
        lang = 'en'
    return lang


@attr.s
class Favourite(object):
    title = attr.ib()
    lat = attr.ib()
    lon = attr.ib()
    subtitle = attr.ib()


@attr.s
class Context(object):
    lang = attr.ib(default=None)
    min_lat = attr.ib(default=None)
    min_lon = attr.ib(default=None)
    max_lat = attr.ib(default=None)
    max_lon = attr.ib(default=None)
    lat = attr.ib(default=None)
    lon = attr.ib(default=None)
    favourites = attr.ib(default=None)
    broken = attr.ib(default=False)
    commands = attr.ib(default=attr.Factory(set))
    route = attr.ib(default=None)

    def to_dict(self):
        return attr.asdict(self)


def _rearrange_favourites(favourites):
    def favourite_priority(favourite):
        priority_keywords = {'дом', 'дача', 'дачу', 'home', 'домой', 'işyeri', 'iş', 'работа', 'работу', 'work'}
        return any(x in priority_keywords for x in favourite.title.lower().split(' '))

    return sorted(favourites, key=favourite_priority)


def _add_favourite(context, favourite, title):
    lat = float(favourite['lat'])
    lon = float(favourite['lon'])

    context.favourites.append(Favourite(title=title, lat=lat, lon=lon, subtitle=''))


def wrap_new_request(navigator_state, location, lang):
    context = Context()
    context.favourites = []

    normalized_lang = normalize_lang(lang)
    try:
        fav = navigator_state.get('home', None)
        if fav is not None:
            _add_favourite(context, fav, FAV_PLACES.get(normalized_lang, {}).get('home', 'home'))

        fav = navigator_state.get('work', None)
        if fav is not None:
            _add_favourite(context, fav, FAV_PLACES.get(normalized_lang, {}).get('work', 'work'))

        user_favorites = navigator_state.get('user_favorites', [])
        if user_favorites is not None:
            for fav in user_favorites:
                _add_favourite(context, fav, fav.get('name'))

        context.favourites = _rearrange_favourites(context.favourites)

        current_route = navigator_state.get('current_route', {})
        if 'points' in current_route:
            context.route = [{'lat': float(loc['lat']), 'lon': float(loc['lon'])} for loc in
                             current_route['points']]

        context.lat = float(location['lat'])
        context.lon = float(location['lon'])

        map_view = navigator_state.get('map_view', {})
        lat1 = float(map_view.get('tl_lat', 0))
        lon1 = float(map_view.get('tl_lon', 0))
        lat2 = float(map_view.get('br_lat', 0))
        lon2 = float(map_view.get('br_lon', 0))

        context.min_lat = min(lat1, lat2)
        context.min_lon = min(lon1, lon2)
        context.max_lat = max(lat1, lat2)
        context.max_lon = max(lon1, lon2)

        if context.min_lat == 0 and context.min_lon == 0 and context.max_lat == 0 and context.max_lon == 0:
            context.min_lat = context.lat - 0.01
            context.min_lon = context.lon - 0.01
            context.max_lat = context.lat + 0.01
            context.max_lon = context.lon + 0.01

    except Exception:
        logger.warn('Failed to parse favourites')
        context.broken = True

    return context.to_dict()
