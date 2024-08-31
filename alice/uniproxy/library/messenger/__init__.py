#!/usr/bin/env python

from alice.uniproxy.library.messenger.client import MessengerClient
from alice.uniproxy.library.messenger.client_locator import YdbClientLocator
from alice.uniproxy.library.settings import config


_g_settings = None
_g_client = None

YdbClientLocator.init_counters()


# ====================================================================================================================
def init_mssngr():

    global _g_client, _g_settings
    _g_settings = config.get('messenger', {})
    _g_client = MessengerClient(_g_settings)


# ====================================================================================================================
def mssngr_client_instance() -> MessengerClient:
    global _g_client
    return _g_client
