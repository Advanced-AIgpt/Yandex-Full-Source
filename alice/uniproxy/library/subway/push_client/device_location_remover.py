import logging
import time

import tornado.httpclient
import tornado.gen

from tornado.ioloop import IOLoop

from alice.uniproxy.library.async_http_client import HTTPResponse
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayResponse
from alice.uniproxy.library.ydbs.device_locator import DeviceLocator

max_removing = 50


class LocationsRemover:
    KEEPALIVE_LAST_REMOVED_LOCATIONS_INFO = 10  # seconds

    def __init__(self):
        self._log = logging.getLogger('subway.locations_remover')
        self._removing_locations = {}
        self._last_removed_locations = {}
        self._last_removed_purger = None
        self._counter_run_remove = 0
        self._counter_skip_remove = 0
        self._test_mode = False

    def counter_run_remove(self):
        return self._counter_run_remove

    def counter_skip_remove(self):
        return self._counter_skip_remove

    def test_mode(self, mode=None):
        if mode is not None:
            self._test_mode = mode
        return self._test_mode

    def remove_entry(self, device_id, hostname, timestamp):
        # check last removed cache
        if (device_id, hostname) in self._last_removed_locations:
            return False

        if (device_id, hostname) in self._removing_locations:
            return False

        removings_cnt = len(self._removing_locations)
        if removings_cnt >= max_removing:
            self._counter_skip_remove += 1
            self._log.warning('to many({}) removings in process, skip removing device_id/hostname/ts={}/{}/{}'.format(
                removings_cnt, device_id, hostname, timestamp
            ))
            return True

        self._removing_locations[(device_id, hostname)] = time.monotonic()
        # async run coroutine
        self._run_remove(device_id, hostname, timestamp)
        return False

    @tornado.gen.coroutine
    def _run_remove(self, device_id, hostname, timestamp):
        self._counter_run_remove += 1
        try:
            if self._test_mode:
                yield tornado.gen.sleep(2)

            GlobalCounter.DELIVERY_LOCATION_REMOVE_SUMM.increment()
            yield DeviceLocator.remove(device_id, hostname, timestamp)
        finally:
            self._last_removed_locations[(device_id, hostname)] = time.monotonic()
            del self._removing_locations[(device_id, hostname)]
            if self._last_removed_purger is None:
                self._last_removed_purger = IOLoop.current().call_later(
                    self.KEEPALIVE_LAST_REMOVED_LOCATIONS_INFO,
                    self._purge_last_removed,
                )

    def _purge_last_removed(self):
        self._last_removed_purger = None
        now = time.monotonic()
        remove_recs = [
            rec for rec, ts in self._last_removed_locations.items()
            if ts + self.KEEPALIVE_LAST_REMOVED_LOCATIONS_INFO < now
        ]
        for rec in remove_recs:
            del self._last_removed_locations[rec]
        if self._last_removed_locations:
            self._last_removed_purger = IOLoop.current().call_later(
                self.KEEPALIVE_LAST_REMOVED_LOCATIONS_INFO,
                self._purge_last_removed,
            )


g_locations_remover = LocationsRemover()


def schedule_remove_devices(response: HTTPResponse, host: str):
    subway_response = TSubwayResponse()
    subway_response.ParseFromString(response.body)

    GlobalCounter.DELIVERY_PUSH_MISSING_COUNT_SUMM.increment(len(subway_response.MissingDevices))

    for d in subway_response.MissingDevices:
        if g_locations_remover.remove_entry(d, host, subway_response.Timestamp):
            break
