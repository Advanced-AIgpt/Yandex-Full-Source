import time
import urllib.error
import urllib.request


def wait_ready(ping_url, check_interval=0.2, timeout=10):
    deadline = time.monotonic() + timeout
    while True:
        try:
            resp = urllib.request.urlopen(ping_url)
            assert resp.status == 200
            break
        except urllib.error.URLError:
            if time.monotonic() > deadline:
                raise
            time.sleep(check_interval)
