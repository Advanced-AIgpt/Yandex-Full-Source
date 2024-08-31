import os

import alice.uniproxy.library.backends_common.ydb as uniydb
from alice.uniproxy.library.settings import config, environment

driver = None
ydb_config = None
_session_pool = None


def get_ydb_config():
    global ydb_config
    if not ydb_config:
        ydb_config = config['notificator']['ydb']
    return ydb_config


def get_session_pool(driver, pool_size):
    global _session_pool
    if not _session_pool:
        _session_pool = uniydb.make_session_pool(driver, pool_size, min_pool_size=5)
    return _session_pool


async def get_driver():
    global driver
    if not driver:

        ydb_config = get_ydb_config()
        ydb_token = os.environ.get('YDB_TOKEN_DEVICE')
        if not ydb_token and environment == 'development':
            raise Exception('YDB_TOKEN_DEVICE must be set')

        driver = await uniydb.do_connect_ydb(ydb_config, ydb_token, 'device_delivery')

    return driver


from .device_locator import DeviceLocator, ClientEntry

__all__ = ['DeviceLocator', 'ClientEntry']

DeviceLocator.init_counters()
