import time
from collections import deque

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


class ResponsesStorage(object):
    def __init__(self):
        self._log = Logger.get('.responses_storage')
        self._storages = {}
        self._storage_timings = deque()
        self._ttl = config.get('responses_storage', {}).get('ttl', 30.0)

    def _clear_storage(self):
        now = time.monotonic()
        while len(self._storage_timings) > 0:
            t, reqid = self._storage_timings[0]
            if now - t > self._ttl:
                self._storages.pop(reqid)
                self._storage_timings.popleft()
            else:
                break

    def _get_or_create_storage_by_reqid(self, reqid):
        s = self._storages.get(reqid, None)

        if s is None:
            s = {}
            self._storages[reqid] = s
            self._storage_timings.append((time.monotonic(), reqid))

            # lazy cleaning of old storages
            self._clear_storage()

        return s

    def store(self, reqid, hash, response):
        s = self._get_or_create_storage_by_reqid(reqid)
        s[hash] = response
        self._log.info('{} ResponsesStorage saved \'{}\''.format(reqid, hash))

    def load(self, reqid):
        return self._storages.get(reqid, None)
