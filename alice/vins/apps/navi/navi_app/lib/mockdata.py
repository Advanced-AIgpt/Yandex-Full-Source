# coding: utf-8
from __future__ import unicode_literals

LOCATIONS = {
    'ru': {'lat': 55.732616, 'lon': 37.585889},  # Moscow
    'tr': {'lat': 41.008921, 'lon': 28.967111},  # Stambul
    'fr': {'lat': 48.858814, 'lon': 2.347060},  # Paris
    'en': {'lat': 51.506415, 'lon': -0.127201},  # London
}

FAVOURITES_MOCK = [
    ('Дом Бабушки', '55.3', '37.2', 'адрес дома бабушки'),
    ('На Дачу к Бабушке', '55.1', '37.2', 'адрес дачи бабушки'),
    ('Дача Димы', '55.12', '37.2', 'адрес дачи димы'),
    ('Дом', '55.7', '37.2', 'адрес дома'),
    ('Дом Орел', '55.7', '37.21', 'адрес дома в орле'),
    ('Библиотека Ленина', '55.711', '37.211', 'адрес библиотеки'),
    ('Любимый Дом', '55.74', '37.21 ', 'адрес любимого дома'),
    ('Офис', '55.2', '37.1', 'адрес офиса'),
    ('Папа', '55.26', '37.12', 'адрес папы'),
    ('Надежда Николаевна', '55.1', '37.7', 'адрес надежды'),
    ('На Работу', '55.7', '37.5', 'адрес работы'),
    ('на дачу', '55.7', '37.5', 'адрес дачи'),
    ('самый лучший ресторан', '55.7', '37.5', 'Юбилейный проспект, 1к1'),
    ('Дима', '55.3', '37.4', 'адрес димы'),
    ('Ваня', '55.721397', '37.423384', 'адрес вани'),
    ('Недорогая столовая', '55.721397', '37.423384', 'адрес столовой'),
    ('Настя', '55.721397', '37.423384', 'адрес насти'),
    ('ev', '55.887', '37.99999', 'ev address'),
]
OLD_COMMANDS = 'select, map_search, route, add_point, traffic'


def choose_location(lang):
    if lang not in LOCATIONS:
        return LOCATIONS['ru']
    else:
        return LOCATIONS[lang]


def generate_options(lang, location):
    lat, lon = location['lat'], location['lon']
    d = 0.002
    return {
        'favourites': [
            {'title': f[0], 'lon': float(f[1]), 'lat': float(f[2]), 'subtitle': f[3]} for f in FAVOURITES_MOCK
        ],
        'lat': lat,
        'lon': lon,
        'min_lat': lat - d,
        'max_lat': lat + d,
        'min_lon': lon - d,
        'max_lon': lon + d,
        'lang': lang,
        'commands': OLD_COMMANDS
    }
