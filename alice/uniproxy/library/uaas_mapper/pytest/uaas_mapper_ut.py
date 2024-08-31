import base64
import json

from alice.uniproxy.library.uaas_mapper.uaas_mapper import GetUaasAppHeader
from alice.uniproxy.library.uaas_mapper.uaas_mapper import GetUaasAppInfo


def test_get_uaas_info():
    a, b, c = GetUaasAppInfo('ru.yandex.quasar.app', '')

    assert a == 'OtherApplications'
    assert b == 'android'
    assert c == 'station'


def test_get_uaas_header():
    hdr_encoded = GetUaasAppHeader('ru.yandex.quasar.app', '')
    hdr = base64.b64decode(hdr_encoded)
    data = json.loads(hdr)

    assert 'browserName' in data
    assert 'deviceType' in data
    assert 'mobilePlatform' in data
    assert data['browserName'] == 'OtherApplications'
    assert data['deviceType'] == 'station'
    assert data['mobilePlatform'] == 'android'


def test_aliced_header():
    hdr_encoded = GetUaasAppHeader('aliced', 'Linux')
    hdr = base64.b64decode(hdr_encoded)
    data = json.loads(hdr)

    assert 'browserName' in data
    assert 'deviceType' in data
    assert 'mobilePlatform' in data
    assert data['browserName'] == 'OtherApplications'
    assert data['deviceType'] == 'station_mini'
    assert data['mobilePlatform'] == 'android'


def test_beta_bro_header():
    hdr_encoded = GetUaasAppHeader('ru.yandex.mobile.navigator.inhouse', '')
    hdr = base64.b64decode(hdr_encoded)
    data = json.loads(hdr)

    assert 'browserName' in data
    assert 'mobilePlatform' in data
    assert data['browserName'] == 'YandexNavigatorBeta'
    assert data['mobilePlatform'] == 'iphone'
