import os

from alice.uniproxy.library.backends_memcached.client_provider import ClientProvider


def test_get_geo_empty():
    os.environ.pop('BSCONFIG_ITAGS', None)
    client_provider = ClientProvider()
    geo = client_provider.get_geo()
    assert geo == 'sas'


def test_get_geo_VLA():
    os.environ['BSCONFIG_ITAGS'] = 'a_geo_VLA'
    client_provider = ClientProvider()
    geo = client_provider.get_geo()
    assert geo == 'vla'


def test_get_geo_trash():
    os.environ['BSCONFIG_ITAGS'] = 'a_zero a_one'
    client_provider = ClientProvider()
    geo = client_provider.get_geo()
    assert geo == 'sas'


def test_get_geo_one_of():
    os.environ['BSCONFIG_ITAGS'] = 'a_smth a_geo_mAn'
    client_provider = ClientProvider()
    geo = client_provider.get_geo()
    assert geo == 'man'
