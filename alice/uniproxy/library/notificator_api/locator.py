import tornado
import time

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, RTLogHTTPRequest
from alice.uniproxy.library.global_counter import GlobalTimings, GlobalCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.protos.notificator_pb2 import TDeviceLocator
from alice.uniproxy.library.utils.hostname import current_hostname
from alice.uniproxy.library.settings import config

settings = config['notificator']['uniproxy']
DEFAULT_TIMEOUT = settings.get('timeouts', {}).get('default', 1.5)

LOCATOR = '/locator'


def get_client(url):
    return QueuedHTTPClient.get_client_by_url(
        url,
        pool_size=settings.get('pool_size', 1),
        queue_size=10,
        wait_if_queue_is_full=False
    )


@tornado.gen.coroutine
def register_device(host, puid, device_id, device_model, supported_features):
    host = host or settings['url']

    msg = TDeviceLocator()
    msg.Puid = puid
    msg.DeviceId = device_id
    msg.Host = current_hostname()
    msg.Timestamp = int(time.time() * 1000000)
    model = device_model.lower() if device_model else ''
    if model == 'station':
        model = 'yandexstation'
    msg.DeviceModel = model

    if supported_features:
        msg.Config.SupportedFeatures.extend(supported_features)

    request = RTLogHTTPRequest(
        host + LOCATOR,
        method='POST',
        body=msg.SerializeToString(),
        request_timeout=settings.get('timeouts', {}).get(LOCATOR, DEFAULT_TIMEOUT),
        need_str=True,
    )

    start = time.monotonic()
    try:
        client = get_client(host)
        yield client.fetch(request)
        GlobalCounter.REGISTER_DEVICE_OK_SUMM.increment()

    except Exception as e:
        GlobalCounter.REGISTER_DEVICE_FAIL_SUMM.increment()
        Logger.get('.register_device').error("can't register device at notificator: {}".format(str(e)))

    finally:
        GlobalTimings.store('register_device', time.monotonic() - start)


@tornado.gen.coroutine
def unregister_device(host, puid, device_id):
    host = host or settings['url']

    msg = TDeviceLocator()
    msg.Puid = puid
    msg.DeviceId = device_id
    msg.Host = current_hostname()
    msg.Timestamp = int(time.time() * 1000000)

    request = RTLogHTTPRequest(
        host + LOCATOR,
        method='DELETE',
        body=msg.SerializeToString(),
        request_timeout=settings.get('timeouts', {}).get(LOCATOR, DEFAULT_TIMEOUT),
        need_str=True,
    )

    start = time.monotonic()
    try:
        client = get_client(host)
        yield client.fetch(request)
        GlobalCounter.UNREGISTER_DEVICE_OK_SUMM.increment()

    except Exception as e:
        GlobalCounter.UNREGISTER_DEVICE_FAIL_SUMM.increment()
        Logger.get('.unregister_device').error("can't unregister device at notificator: {}".format(str(e)))

    finally:
        GlobalTimings.store('unregister_device', time.monotonic() - start)
