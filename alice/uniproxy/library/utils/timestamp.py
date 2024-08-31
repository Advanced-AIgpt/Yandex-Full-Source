import datetime
import time

from alice.uniproxy.library.logging import Logger


class PerformanceCounter:
    def __init__(self, name=None, logger=None):
        self._name = name
        self._logger = logger if logger else (Logger.get('utils.perfcnt') if name else None)
        self._ts = 0

    def __enter__(self):
        return self.start()

    def __exit__(self, *args, **kwargs):
        self.stop()

    def start(self):
        self._ts = time.time()
        return self

    def stop(self):
        duration = time.time() - self._ts
        if self._name and self._logger:
            self._logger.info('OP "%s" TOOK %.6f ms' % (self._name, duration * 1000))
        return duration


def current_timestamp():
    return datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%f')


class TimestampLagStorage:
    def __init__(self, lags, reverse_postponed_lags, unistat):
        self._lags = lags  # t1-name -> [(t0-name, lag-name)]
        self._reverse_postponed_lags = reverse_postponed_lags  # t0-name -> [(t1-name, lag-name-for-t0_to_t1)]
        self._unistat = unistat
        self._t0 = {}  # times are stored here

    def store_lags(self, name, t):
        for lag in self._lags.get(name, []):
            t0 = self._t0.get(lag[0])
            if t0 is not None:
                self._unistat.store2(lag[1], t - t0)
        # for postponed lags
        t0 = t
        for lag in self._reverse_postponed_lags.get(name, []):
            t1 = self._t0.get(lag[0])
            if t1 is not None and t1 > t0:
                self._unistat.store2(lag[1], t1 - t0)

    def store(self, name, t):
        self._t0[name] = t
        self.store_lags(name, t)

    @property
    def timings(self):
        return self._t0
