import logging
import pytest

import tornado.gen

import unisystem
from alice.uniproxy.library.global_counter import GlobalCounter
import alice.uniproxy.library.testing
from tests.mocks import tvm_client_mock, subway_client_mock, wait_for_yamb_mock, no_lass


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')
GlobalCounter.init()


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_real_clients_counter():
    counter = GlobalCounter.MSSNGR_CLIENTS_AMMX
    orig_counter_value = counter.value()

    yield wait_for_yamb_mock()

    us = unisystem.UniSystem()
    us.update_messenger_guid("oauth-valid-token")
    us.update_messenger_guid("oauth-valid-token")

    yield tornado.gen.sleep(0.25)
    assert counter.value() == orig_counter_value + 1

    us.close()
    assert counter.value() == orig_counter_value


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_anonymous_clients_counter():
    counter = GlobalCounter.MSSNGR_ANON_CLIENTS_AMMX
    orig_counter_value = counter.value()

    us = unisystem.UniSystem()
    us.set_messenger_guid(guid="anonymous-guid", ticket=None, anonymous=True)
    us.set_messenger_guid(guid="anonymous-guid", ticket=None, anonymous=True)

    yield tornado.gen.sleep(0.25)
    assert counter.value() == orig_counter_value + 1

    us.close()
    assert counter.value() == orig_counter_value


@pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
@alice.uniproxy.library.testing.ioloop_run
def test_fake_clients_counter():
    counter = GlobalCounter.MSSNGR_FAKE_CLIENTS_AMMX
    orig_counter_value = counter.value()

    us = unisystem.UniSystem()
    us.set_messenger_guid(guid="fake-guid", ticket="service-ticket")
    us.set_messenger_guid(guid="fake-guid", ticket="service-ticket")

    yield tornado.gen.sleep(0.25)
    assert counter.value() == orig_counter_value + 1

    us.close()
    assert counter.value() == orig_counter_value
