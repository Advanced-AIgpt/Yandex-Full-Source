import tornado.ioloop
import tornado.gen

from alice.uniproxy.library.backends_memcached.client_provider import ClientProvider

from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


MEMCACHED_DEFAULT_EXPIRE_TIME = 43200   # 43200 seconds == 12 hours
MEMCACHED_DEFAULT_POOL_SIZE = 16


MEMCACHED_TTS_SERVICE = config['tts'].get('memcached_service')
MEMCACHED_ACTIVATION_SERVICE = config['activation'].get('memcached_service')


g_client_provider = ClientProvider()
g_fake_client_provider = ClientProvider(fake=True)


# --------------------------------------------------------------------------------------------------------------------
def memcached_init(ignore=[], fake=False):
    global g_client_provider, g_fake_client_provider

    memcached_configs = config.get('memcached', {})
    for name, conf in memcached_configs.items():
        if not conf.get('enabled'):
            continue
        if name in ignore:
            continue
        if fake:
            g_fake_client_provider.init_sync(name, conf)
        else:
            g_client_provider.init_sync(name, conf)


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def memcached_init_async(ignore=[], fake=False):
    global g_client_provider, g_fake_client_provider

    memcached_configs = config.get('memcached', {})
    for name, conf in memcached_configs.items():
        if not conf.get('enabled'):
            continue
        if name in ignore:
            continue
        if fake:
            yield g_fake_client_provider.init(name, conf)
        else:
            yield g_client_provider.init(name, conf)


# --------------------------------------------------------------------------------------------------------------------
def memcached_client(name, fake=False):
    global g_client_provider, g_fake_client_provider
    return g_client_provider.get(name) if not fake else g_fake_client_provider.get(name)


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def memcached_save_data(name, value, ttl):
    client = memcached_client('tts')

    if not client:
        return False

    if name is None:
        return False

    try:
        with UnistatTiming('memcached_set_time'):
            yield client.set(name, value, exptime=ttl)
    except Exception as exc:
        GlobalCounter.MEMCACHED_SET_ERRORS_SUMM.increment()
        Logger.get('.backends.memcached').exception(exc)
        return False

    return True


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def memcached_get_data(name):
    client = memcached_client('tts')

    if not client:
        return None

    data = None

    try:
        with UnistatTiming('memcached_get_time'):
            data = yield client.get(name, binary=True)

        GlobalCounter.MEMCACHED_GETS_SUMM.increment()
    except Exception as exc:
        GlobalCounter.MEMCACHED_GET_ERRORS_SUMM.increment()
        Logger.get('.backends.memcached').exception(exc)

    return data
