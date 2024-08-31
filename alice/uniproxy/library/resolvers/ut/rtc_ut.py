import json
import os
import tornado.gen
import tornado.ioloop
import tornado.concurrent

os.environ['UNIPROXY_CUSTOM_ENVIRONMENT_TYPE'] = 'rtc_alpha'

from alice.uniproxy.library.resolvers.rtc import RTCResolver

import alice.uniproxy.library.testing

PODSET_IDS = {}
DC = ['man', 'sas', 'vla']
for dc in DC:
    PODSET_IDS['uniproxy_' + dc] = dc

EXISTING_PODSET = list(PODSET_IDS.keys())[0]


class FakeResponse:
    def __init__(self, hostnames):
        res = {"instance": []}
        for hostname in hostnames:
            res["instance"].append({"spec": {"hostname": hostname}})
        self.body = json.dumps(res).encode('utf-8')


class FakeClient:
    def __init__(self, client_body):
        self.body = client_body

    @tornado.gen.coroutine
    def fetch(self, request):
        yield tornado.gen.sleep(0)
        resp = self.body(request)
        return resp


class ResolverMock(object):

    def __init__(self, client=None, services=None):
        self.resolver = RTCResolver(custom_period=0.2)
        if client:
            self.resolver._client = client
        if services:
            self.resolver._services = services
        self.resolver.start()
        self.res = None

    @tornado.gen.coroutine
    def resolve(self, host):
        self.res = yield self.resolver.resolve_uniproxy(host)

    def tear_down(self):
        self.resolver.stop()


def get_service_from_req(request):
    service = json.loads(request.body.decode('utf-8'))['filter']['serviceId']
    return service


@alice.uniproxy.library.testing.ioloop_run
def test_existing_host():

    def simple_body(request):
        service = get_service_from_req(request)
        response = FakeResponse(['my_host_1_' + service, 'my_host_2_' + service])
        return response
    test_client = FakeClient(simple_body)
    resolver_mock = ResolverMock(client=test_client)
    yield resolver_mock.resolve('my_host_1_uniproxy_sas')
    resolver_mock.tear_down()
    assert resolver_mock.res == 'my_host_1_uniproxy_sas'


@alice.uniproxy.library.testing.ioloop_run
def test_nonexisting_host():

    def simple_body(request):
        service = get_service_from_req(request)
        response = FakeResponse(['my_host_1_' + service, 'my_host_2_' + service])
        return response
    test_client = FakeClient(simple_body)
    resolver_mock = ResolverMock(client=test_client)
    yield resolver_mock.resolve('my_host_0_uniproxy_sas')
    resolver_mock.tear_down()
    assert resolver_mock.res is None

counter = 0


@alice.uniproxy.library.testing.ioloop_run
def test_with_ex():

    def body_with_exception(request):
        global counter
        if counter < 10:
            counter += 1
            raise Exception('ex')
        service = get_service_from_req(request)
        response = FakeResponse(['my_host_1_' + service, 'my_host_2_' + service])
        return response
    test_client = FakeClient(body_with_exception)
    resolver_mock = ResolverMock(client=test_client)
    yield resolver_mock.resolve('my_host_1_uniproxy_sas')
    resolver_mock.tear_down()
    assert resolver_mock.res == 'my_host_1_uniproxy_sas'
