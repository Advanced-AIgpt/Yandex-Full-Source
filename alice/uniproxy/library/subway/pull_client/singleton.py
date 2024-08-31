from . import PullClient
from alice.uniproxy.library.settings import config


SUBWAY_WAIT_FOR_START = config['subway']['enabled']
SUBWAY_PULL_POOL_SIZE = config.get('subway', {}).get('pool_size', 26)


_g_client = PullClient(wait=SUBWAY_WAIT_FOR_START)


def subway_client() -> PullClient:
    global _g_client
    return _g_client


def subway_init(wait=True):
    global _g_client
    _g_client.start(pool_size=SUBWAY_PULL_POOL_SIZE)
    if wait:
        _g_client.wait_for_ready_sync()
