from alice.cuttlefish.library.python.test_utils import fix_timeout_for_sanitizer

import time
import asyncio
import tornado.httpclient


async def wait_ping(url, timeout=None, delay=0.3):
    deadline = None if (timeout is None) else (time.monotonic() + fix_timeout_for_sanitizer(timeout))

    while True:
        try:
            resp = await tornado.httpclient.AsyncHTTPClient().fetch(url)
            if resp.code == 200:
                return True
        except:
            if (deadline is not None) and (time.monotonic() > deadline):
                raise
            await asyncio.sleep(delay)
