# coding: utf-8
from __future__ import unicode_literals

import math
import logging

from requests import RequestException

from navi_app.lib.tvm_client import TvmClientFactory, tvm_client_factory

from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting
from vins_core.utils.decorators import time_info
from vins_core.utils.strings import smart_unicode

from ticket_parser2.api.v1.exceptions import TvmException

from yandex.maps.proto.common2.response_pb2 import Response
from yandex.maps.proto.search.business_pb2 import GEO_OBJECT_METADATA as BUSINESS_METADATA
from yandex.maps.proto.search.geocoder_pb2 import GEO_OBJECT_METADATA as GEO_METADATA
from yandex.maps.proto.search.kind_pb2 import Kind

logger = logging.getLogger(__name__)

EARTH_RADIUS = 6371.0


class GeosearchAPIError(Exception):
    pass


class GeosearchAPI(BaseHTTPAPI):
    DEFAULT_URL = get_setting(
        'GEOSEARCH_URL', default='http://addrs-testing.search.yandex.net/search/stable/yandsearch'
    )

    _TEXT_REQUEST_PARAM = 'text'
    _SPN_REQUEST_PARAM = 'spn'
    _UID_REQUEST_PARAM = 'yandex_uid'
    _LONLAT_REQUEST_PARAM = 'll'

    _LANG_REQUEST_PARAM = 'lang'
    _MODEL_REQUEST_PARAM = 'model'
    _AUTOSCALE_REQUEST_PARAM = 'autoscale'
    _RESULTS_REQUEST_PARAM = 'results'
    _TYPE_REQUEST_PARAM = 'type'
    _ORIGIN_REQUEST_PARAM = 'origin'
    _KEY_REQUEST_PARAM = 'key'
    _FORMAT_REQUEST_PARAM = 'ms'
    _TIMEOUT_CONFIG_PARAM = 'timeout'
    _URL_CONFIG_PARAM = 'url'

    _TVM2_CLIENT_NAME = 'tvm2_client_name'

    def __init__(self, **kwargs):
        self._config = GeosearchAPI._get_default_config()
        self._config.update(kwargs)
        GeosearchAPI._validate_config(self._config)

        self._tvm_client_name = self._config.get(GeosearchAPI._TVM2_CLIENT_NAME, '')

        kwargs['timeout'] = self._config[self._TIMEOUT_CONFIG_PARAM]
        super(GeosearchAPI, self).__init__(**kwargs)

    @time_info('geo_search')
    def find(self, text, lat, lon, lang=None, uid=None, spn=None, results=None, object_types=None):
        logger.debug('GeosearchAPI: find() called with query "%s"', text)

        assert object_types is None or all(object_type in ['geo', 'biz'] for object_type in object_types)

        params = self._construct_request({
            GeosearchAPI._TEXT_REQUEST_PARAM: text,
            GeosearchAPI._LONLAT_REQUEST_PARAM: '%s,%s' % (str(lon), str(lat)),
        })
        if uid is not None:
            params[GeosearchAPI._UID_REQUEST_PARAM] = uid
        if spn is not None:
            params[GeosearchAPI._SPN_REQUEST_PARAM] = spn
        if object_types is not None:
            params[GeosearchAPI._TYPE_REQUEST_PARAM] = ','.join(object_types)
        if results is not None:
            params[GeosearchAPI._RESULTS_REQUEST_PARAM] = results
        if lang is not None:
            params[GeosearchAPI._LANG_REQUEST_PARAM] = lang

        try:
            headers = tvm_client_factory.get_headers(self._tvm_client_name, 'geo')
        except TvmException as e:
            raise GeosearchAPIError(e)

        try:
            response = self.get(
                headers=headers,
                url=self._config[GeosearchAPI._URL_CONFIG_PARAM],
                params=params,
                request_label='geo:{0}'.format(text),
            )
        except RequestException as e:
            raise GeosearchAPIError(e)
        if response.status_code != 200:
            raise GeosearchAPIError('%s %s' % (response.status_code, response.reason))

        r = Response()
        r.ParseFromString(response.content)

        result = []

        geo_objects = r.reply.geo_object
        if geo_objects:
            for geo_object in geo_objects:
                geo = GeosearchAPI._parse_geosearch_result(geo_object)
                result.append(geo)

        return result

    def _construct_request(self, params):
        add_params = {
            GeosearchAPI._LANG_REQUEST_PARAM,
            GeosearchAPI._AUTOSCALE_REQUEST_PARAM,
            GeosearchAPI._RESULTS_REQUEST_PARAM,
            GeosearchAPI._TYPE_REQUEST_PARAM,
            GeosearchAPI._ORIGIN_REQUEST_PARAM,
            GeosearchAPI._KEY_REQUEST_PARAM,
            GeosearchAPI._FORMAT_REQUEST_PARAM
        }
        request_config = {param: self._config[param] for param in add_params}
        request_config.update(params)
        return request_config

    @classmethod
    def _parse_geosearch_result(cls, geo_object):
        result = {
            'name': smart_unicode(geo_object.name),
            'description': smart_unicode(geo_object.description if geo_object.description else geo_object.name),
        }

        for metadata in geo_object.metadata:
            if metadata.HasExtension(BUSINESS_METADATA):
                result['type'] = 'biz'
            elif metadata.HasExtension(GEO_METADATA):
                geo_data = metadata.Extensions[GEO_METADATA]
                result.update(cls._parse_geo(geo_data))

        if geo_object.geometry:
            result['lat'], result['lon'] = GeosearchAPI._parse_geometry(geo_object.geometry[0])

        return result

    @classmethod
    def _parse_geo(cls, meta_data):
        result = {
            'type': 'geo',
        }

        if meta_data.address and meta_data.address.component:
            result['kind'] = Kind.Name(meta_data.address.component[-1].kind[0]).lower()
        else:
            result['kind'] = 'unknown'

        return result

    @classmethod
    def _parse_geometry(cls, geometry):
        if geometry.point:
            point = geometry.point
            return point.lat, point.lon

        from google.protobuf import text_format
        logger.error("Couldn't recognize format of protobuf: '%s' ", text_format.MessageToString(geometry))
        return 0, 0

    @staticmethod
    def _get_default_config():
        return {
            GeosearchAPI._LANG_REQUEST_PARAM: 'ru_RU',
            GeosearchAPI._AUTOSCALE_REQUEST_PARAM: '1',
            GeosearchAPI._RESULTS_REQUEST_PARAM: 1,
            GeosearchAPI._TYPE_REQUEST_PARAM: 'geo,biz',
            GeosearchAPI._ORIGIN_REQUEST_PARAM: 'mobile-yari-search-text',
            GeosearchAPI._KEY_REQUEST_PARAM: '1',
            GeosearchAPI._FORMAT_REQUEST_PARAM: 'pb',
            GeosearchAPI._SPN_REQUEST_PARAM: '0.15,0.15',
            GeosearchAPI._TIMEOUT_CONFIG_PARAM: 1,
            GeosearchAPI._URL_CONFIG_PARAM: GeosearchAPI.DEFAULT_URL
        }

    @staticmethod
    def _validate_config(cfg):
        assert isinstance(cfg[GeosearchAPI._LANG_REQUEST_PARAM], basestring)
        assert isinstance(cfg[GeosearchAPI._AUTOSCALE_REQUEST_PARAM], basestring)
        assert isinstance(cfg[GeosearchAPI._RESULTS_REQUEST_PARAM], int)
        assert isinstance(cfg[GeosearchAPI._TYPE_REQUEST_PARAM], basestring)
        assert isinstance(cfg[GeosearchAPI._ORIGIN_REQUEST_PARAM], basestring)
        assert isinstance(cfg[GeosearchAPI._KEY_REQUEST_PARAM], basestring)
        assert isinstance(cfg[GeosearchAPI._FORMAT_REQUEST_PARAM], basestring)


class GeosearchAPIMocker(object):
    def __init__(self, request_mock, patcher):
        self._request_mock = request_mock
        self._patcher = patcher

    def __enter__(self):
        self._request_mock.start()
        self._patcher.start()
        return self

    def __exit__(self, type, value, traceback):
        self._request_mock.stop()
        self._patcher.stop()


def geosearch_tvm_mock(return_value='unittest'):
    import mock
    return mock.patch.object(TvmClientFactory, 'get_headers', return_value={
        'X-Ya-Service-Ticket': return_value
    })


def geosearch_fail_mock(url):
    import requests_mock

    req_mock = requests_mock.Mocker(real_http=True)
    req_mock.get(url, status_code=500)

    return GeosearchAPIMocker(request_mock=req_mock, patcher=geosearch_tvm_mock(return_value='unittest'))


def geosearch_no_results_mock(url):
    import requests_mock

    response = Response()
    req_mock = requests_mock.Mocker(real_http=True)
    req_mock.get(url, content=response.SerializeToString())

    return GeosearchAPIMocker(request_mock=req_mock, patcher=geosearch_tvm_mock(return_value='unittest'))


def geosearch_mock(url, response, req_mock=None):
    import requests_mock

    if req_mock is None:
        req_mock = requests_mock.Mocker(real_http=True)

    if isinstance(response, basestring):
        req_mock.get(url, content=response)
    else:
        req_mock.get(url, content=response.SerializeToString())

    return GeosearchAPIMocker(request_mock=req_mock, patcher=geosearch_tvm_mock(return_value='unittest'))


def geosearch_geo_mock(
    street=None, house=None, country='Россия', city='Москва', name=None, description=None,
    lat=0.0, lon=0.0, url=GeosearchAPI.DEFAULT_URL, mock=None,
):
    response = Response()
    geo_object = response.reply.geo_object.add()
    _geosearch_geo_entry(geo_object=geo_object,
                         street=street, house=house, country=country, city=city,
                         name=name, description=description, lat=lat, lon=lon)

    return geosearch_mock(url, response, req_mock=mock)


def geosearch_geo_mock_many(results, url=GeosearchAPI.DEFAULT_URL, mock=None):
    response = Response()
    geo_object_root = response.reply

    for r in results:
        geo_object = geo_object_root.geo_object.add()
        _geosearch_geo_entry(geo_object=geo_object, **r)

    return geosearch_mock(url, response, req_mock=mock)


def geosearch_biz_mock(
    name, street, house, country='Россия', city='Москва',
    lat=0.0, lon=0.0, url=GeosearchAPI.DEFAULT_URL, mock=None,
):
    response = Response()
    geo_object = response.reply.geo_object.add()
    _geosearch_biz_entry(
        geo_object=geo_object, name=name, street=street, house=house, country=country, city=city, lat=lat, lon=lon
    )

    return geosearch_mock(url, response, req_mock=mock)


def geosearch_biz_mock_many(results, url=GeosearchAPI.DEFAULT_URL, mock=None):
    response = Response()
    geo_object_root = response.reply

    for r in results:
        geo_object = geo_object_root.geo_object.add()
        _geosearch_biz_entry(geo_object=geo_object, **r)

    return geosearch_mock(url, response, req_mock=mock)


def _geosearch_geo_entry(geo_object,
                         street=None, house=None, country='Россия', city='Москва',
                         name=None, description=None, lat=0.0, lon=0.0):
    address_line = city
    if street:
        address_line += ', %s' % street
    if house:
        address_line += ', %s' % house

    geo_meta_data = geo_object.metadata.add().Extensions[GEO_METADATA]

    _add_geosearch_address_entry(
        meta_data=geo_meta_data,
        address_line=address_line,
        country=country,
        city=city,
        street=street,
        house=house
    )

    geo_object.name = address_line if name is None else name
    geo_object.description = address_line if description is None else description
    geometry = geo_object.geometry.add()
    geometry.point.lat = lat
    geometry.point.lon = lon


def _geosearch_biz_entry(geo_object, name, street, house, country='Россия', city='Москва', lat=0.0, lon=0.0):
    from yandex.maps.proto.search.precision_pb2 import NUMBER

    address_line = '%s, %s, %s' % (city, street, house)

    biz_meta_data = geo_object.metadata.add().Extensions[BUSINESS_METADATA]
    biz_meta_data.id = name
    biz_meta_data.name = name
    biz_meta_data.geocode_result.house_precision = NUMBER

    _add_geosearch_address_entry(
        meta_data=biz_meta_data,
        address_line=address_line,
        country=country,
        city=city,
        street=street,
        house=house
    )

    geo_object.name = address_line if name is None else name
    geo_object.description = address_line
    geometry = geo_object.geometry.add()
    geometry.point.lat = lat
    geometry.point.lon = lon


def _add_geosearch_address_entry(meta_data, address_line, country, city, street, house):
    import yandex.maps.proto.search.kind_pb2 as kind_pb2

    address = meta_data.address
    address.formatted_address = address_line

    if country:
        component = address.component.add()
        component.name = country
        component.kind.append(kind_pb2.COUNTRY)

    if city:
        component = address.component.add()
        component.name = city
        component.kind.append(kind_pb2.PROVINCE)

    if street:
        component = address.component.add()
        component.name = street
        component.kind.append(kind_pb2.STREET)

    if house:
        component = address.component.add()
        component.name = unicode(house)
        component.kind.append(kind_pb2.HOUSE)

    return address


def gps_distance(a_lat, a_lon, b_lat, b_lon):
    cos_projection = (
        math.sin(math.radians(a_lat)) * math.sin(math.radians(b_lat)) +
        math.cos(math.radians(a_lat)) * math.cos(math.radians(b_lat)) * math.cos(math.radians(a_lon - b_lon))
    )
    if cos_projection > 1.:
        cos_projection = 1.
    return EARTH_RADIUS * math.acos(cos_projection)
