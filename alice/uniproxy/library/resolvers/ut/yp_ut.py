import os
import tornado.gen
import tornado.ioloop
import tornado.concurrent

os.environ['UNIPROXY_CUSTOM_ENVIRONMENT_TYPE'] = 'rtc_alpha'

from alice.uniproxy.library.async_http_client import HTTPError
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.resolvers.yp import YpResolver

import alice.uniproxy.library.testing

PODSET_IDS = {}
DC = ['man', 'sas', 'vla']
for dc in DC:
    PODSET_IDS['alice-memcached-yp-test-' + dc] = dc

EXISTING_PODSET = list(PODSET_IDS.keys())[0]
GlobalCounter.init()


class FakeResponse:
    def __init__(self):
        self.endpoint_set = {'endpoint_set': {'endpoints': []}}

    def add_endpoint(self, ip6, fqdn, port):
        self.endpoint_set['endpoint_set']['endpoints'].append({'ip6_address': ip6, 'fqdn': fqdn, 'port': port})

    def get_endpoint_set(self):
        return self.endpoint_set


class FakeResolver:
    def __init__(self, resolver_body):
        self.body = resolver_body

    @tornado.gen.coroutine
    def resolve_endpoints(self, podset_id):
        yield tornado.gen.sleep(0)
        return self.body(podset_id)


class UnisystemMock(object):
    def __init__(self, custom_resolver=None):
        self.resolver = YpResolver(custom_resolver)
        self.resolver.start()
        self.res = {}

    @tornado.gen.coroutine
    def resolve(self, podset_id):
        self.res = yield self.resolver.resolve_podset(podset_id)

    def tear_down(self):
        self.resolver.stop()


@alice.uniproxy.library.testing.ioloop_run
def test_simple():

    def simple_body(podset_id):
        test_response = FakeResponse()
        test_response.add_endpoint('ip6', 'fqdn', 'port')
        test_response.add_endpoint('1', '2', '3')
        return test_response.get_endpoint_set()
    test_resolver = FakeResolver(simple_body)
    unisystem = UnisystemMock(test_resolver)
    yield unisystem.resolve(EXISTING_PODSET)
    unisystem.tear_down()
    assert unisystem.res.get('fqdn') is not None
    assert unisystem.res['fqdn'] == 'ip6:port'
    assert unisystem.res.get('2') is not None
    assert unisystem.res['2'] == '1:3'


@alice.uniproxy.library.testing.ioloop_run
def test_with_unknown_podset():

    def simple_body(podset_id):
        test_response = FakeResponse()
        test_response.add_endpoint('ip6', 'fqdn', 'port')
        test_response.add_endpoint('1', '2', '3')
        return test_response.get_endpoint_set()
    test_resolver = FakeResolver(simple_body)
    unisystem = UnisystemMock(test_resolver)
    unknown_podset = 'unknown_podset'
    try:
        yield unisystem.resolve(unknown_podset)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'Unknown podset_id: {}'.format(unknown_podset)


@alice.uniproxy.library.testing.ioloop_run
def test_with_598():

    def body_with_exception(podset_id):
        raise HTTPError(598, 'body', 'req')
    test_resolver = FakeResolver(body_with_exception)
    unisystem = UnisystemMock(test_resolver)
    try:
        yield unisystem.resolve(EXISTING_PODSET)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'Got 598 error from resolver'


@alice.uniproxy.library.testing.ioloop_run
def test_with_599():

    def body_with_exception(podset_id):
        raise HTTPError(599, 'body', 'req')
    test_resolver = FakeResolver(body_with_exception)
    unisystem = UnisystemMock(test_resolver)
    try:
        yield unisystem.resolve(EXISTING_PODSET)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'Got 599 error from resolver'


@alice.uniproxy.library.testing.ioloop_run
def test_with_404():

    def body_with_exception(podset_id):
        raise HTTPError(404, 'body', 'req')
    test_resolver = FakeResolver(body_with_exception)
    unisystem = UnisystemMock(test_resolver)
    try:
        yield unisystem.resolve(EXISTING_PODSET)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'Got 404 error from resolver'


@alice.uniproxy.library.testing.ioloop_run
def test_with_timeout():

    def body_with_exception(podset_id):
        raise Exception('ex')
    test_resolver = FakeResolver(body_with_exception)
    unisystem = UnisystemMock(test_resolver)
    try:
        yield unisystem.resolve(EXISTING_PODSET)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'ex'


@alice.uniproxy.library.testing.ioloop_run
def test_with_empty_response():

    def body_with_empty_response(podset_id):
        return None
    test_resolver = FakeResolver(body_with_empty_response)
    unisystem = UnisystemMock(test_resolver)
    try:
        yield unisystem.resolve(EXISTING_PODSET)
        unisystem.tear_down()
        assert False
    except Exception as ex:
        assert str(ex) == 'Empty response from resolver'
