# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import numpy as np
import pytest

from vins_core.utils.config import get_setting

from navi_app.lib.geosearch import (
    GeosearchAPI,
    GeosearchAPIError,
    geosearch_fail_mock,
    geosearch_biz_mock,
    geosearch_geo_mock,
    geosearch_mock,
    gps_distance
)

from library.python import resource

_URL = get_setting('GEOSEARCH_URL', default=GeosearchAPI.DEFAULT_URL)


def test_geosearch_biz():
    resource_name = 'response_geosearch_biz.data'
    response_data = resource.find(resource_name)
    if not response_data:
        raise RuntimeError('Resource {} is not found'.format(resource_name))

    with geosearch_mock(url=_URL, response=response_data):
        gs = GeosearchAPI(url=_URL, object_types=['biz'])
        result = gs.find('самый лучший ресторан', 55.732616, 37.585889, 'ru_RU', results=5)

    assert result == [
        {
            'name': 'Макдоналдс',
            'lat': 55.747404,
            'lon': 37.584499,
            'type': 'biz',
            'description': 'Троилинский пер., 1, Москва, Россия'
        },
        {
            'name': 'Сейджи',
            'lat': 55.731897,
            'lon': 37.592976,
            'type': 'biz',
            'description': 'Комсомольский просп., 5/2, Москва, Россия'
        },
        {
            'name': 'White Rabbit',
            'lat': 55.747608,
            'lon': 37.581377,
            'type': 'biz',
            'description': 'Смоленская площадь, 3, Москва, Россия'
        },
        {
            'name': 'Scrocchiarella',
            'lat': 55.755028,
            'lon': 37.600743,
            'type': 'biz',
            'description': 'Никитский бул., 12, Москва, Россия'
        },
        {
            'name': 'Сыроварня',
            'lat': 55.750803,
            'lon': 37.559173,
            'type': 'biz',
            'description': 'Кутузовский просп., 12, стр. 1, Москва, Россия'
        }
    ]


def test_geosearch_geo():
    resource_name = 'response_geosearch_geo.data'
    response_data = resource.find(resource_name)
    if not response_data:
        raise RuntimeError('Resource {} is not found'.format(resource_name))

    with geosearch_mock(url=_URL, response=response_data):
        gso = GeosearchAPI(url=_URL, object_types=['geo'])
        result = gso.find('александровский сад', 55.732616, 37.585889, 'ru_RU', results=5)

    assert result == [
        {
            'kind': 'metro_station',
            'name': 'метро Александровский сад',
            'lon': 37.609461,
            'lat': 55.752349,
            'type': 'geo',
            'description': 'Филёвская линия, Москва, Россия'
        },
        {
            'kind': 'vegetation',
            'name': 'Александровский сад',
            'lon': 37.613611,
            'lat': 55.752075,
            'type': 'geo',
            'description': 'Москва, Россия'
        },
        {
            'kind': 'locality',
            'name': 'коттеджный поселок Александровский сад',
            'lon': 38.542181,
            'lat': 56.366585,
            'type': 'geo',
            'description': 'Александровский район, Владимирская область, Россия'
        }
    ]


def test_geosearch_geo_mock():
    with geosearch_geo_mock(
            country='Россия', city='Москва', street='Кутузовский проспект', house='10', lat=10, lon=20, url=_URL
    ):
        geosearch = GeosearchAPI(url=_URL)

        assert geosearch.find('кутуза 10', 55.732616, 37.585889, 'ru_RU', results=1) == [
            {
                'name': 'Москва, Кутузовский проспект, 10',
                'description': 'Москва, Кутузовский проспект, 10',
                'type': 'geo',
                'lat': 10,
                'lon': 20,
                'kind': 'house',
            }
        ]


def test_geosearch_biz_mock():
    with geosearch_biz_mock(
        name='Пеши', country='Россия', city='Москва', street='Кутузовский проспект', house='10', lat=10, lon=20,
        url=_URL,
    ):
        geosearch = GeosearchAPI(url=_URL)

        assert geosearch.find('ресторан пеши', 55.732616, 37.585889, 'ru_RU', results=1) == [
            {
                'name': 'Пеши',
                'type': 'biz',
                'lat': 10,
                'lon': 20,
                'description': 'Москва, Кутузовский проспект, 10'
            }
        ]


def test_geosearch_fail_mock():
    geosearch = GeosearchAPI(url=_URL)
    with geosearch_fail_mock(_URL):
        with pytest.raises(GeosearchAPIError):
            geosearch.find('александровский сад', 55.732616, 37.585889, 'ru_RU', results=5)


@pytest.mark.parametrize("a_lat, a_lon, b_lat, b_lon, dist", [
    (55.732616, 37.585889, 55.163688, 61.400699, 1495.79),  # Москва Челябинск
    (55.755919, 37.617589, 41.897397, 12.498911, 2375.66),  # Москва Рим
])
def test_gps_distance(a_lat, a_lon, b_lat, b_lon, dist):
    assert np.isclose(gps_distance(a_lat, a_lon, b_lat, b_lon), dist)
