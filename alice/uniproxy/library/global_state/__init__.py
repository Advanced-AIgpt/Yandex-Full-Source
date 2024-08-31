import multiprocessing
import os
import time


class CachedMultiprocessingVariable:
    def __init__(self, ctype="i", initval=0, ttl=1.0):
        self._local = initval
        self._mp = multiprocessing.Value(ctype, initval)
        self._expiration = time.monotonic() + ttl
        self._ttl = ttl

    def set(self, value):
        with self._mp.get_lock():
            self._mp.value = value
        self._local = value
        self._expiration = time.monotonic() + self._ttl

    def incr(self, amount: int = 1):
        with self._mp.get_lock():
            self._mp.value += amount
            self._local = self._mp.value
        self._expiration = time.monotonic() + self._ttl

    def value(self):
        if time.monotonic() > self._expiration:
            with self._mp.get_lock():
                self._local = self._mp.value
            self._expiration = time.monotonic() + self._ttl
        return self._local


class GlobalTags:
    geo = None
    balancing_hint_header = None
    geo_valid = False

    def __init__(self):
        pass

    @classmethod
    def init(cls):
        cls.balancing_hint_header = None
        cls.geo = os.environ.get('UNIPROXY_CUSTOM_GEO', os.environ.get('a_geo', os.environ.get('a_dc', ''))).lower()
        cls.ctype = os.environ.get('a_ctype', '').lower()
        if cls.geo in ['sas', 'man', 'vla', 'myt', 'iva'] and cls.ctype in ['prod', 'prestable']:
            cls.geo_valid = True

    @classmethod
    def geo(cls):
        return cls.geo

    @classmethod
    def get_balancing_hint_header_val(cls, mode):
        if not cls.geo_valid:
            return None
        if cls.ctype == 'prod':
            if mode == 'pre_prod' or mode == 'prod':
                return cls.geo
            elif mode == 'fullmesh':
                return None
        elif cls.ctype == 'prestable':
            if mode == 'pre_prod':
                return '%s-pre' % (cls.geo)
            elif mode == 'prod' or mode == 'fullmesh':
                return None
        return None

    @classmethod
    def get_balancing_hint_header(cls, mode):
        val = cls.get_balancing_hint_header_val(mode)
        if val is not None:
            return 'X-Yandex-Balancing-Hint: %s\r\n' % (val)
        return None


class GlobalState(object):
    def __init__(self, ctype="i", initval=0):
        self.val = multiprocessing.Value(ctype, initval)

    def set(self, value):
        with self.val.get_lock():
            self.val.value = value

    def incr(self, amount: int = 1):
        with self.val.get_lock():
            self.val.value += amount

    def value(self):
        with self.val.get_lock():
            return self.val.value

    @classmethod
    def init(cls, nproc: int = 1):
        GlobalTags.init()
        cls.nproc = nproc
        cls._is_ready = False
        cls.READY = cls()
        cls.LISTENING = CachedMultiprocessingVariable("i", 0, ttl=1.0)

    @classmethod
    def set_listening(cls):
        cls.LISTENING.set(1)

    @classmethod
    def set_stopping(cls):
        cls.LISTENING.set(0)

    @classmethod
    def set_ready(cls):
        cls.READY.incr()

    @classmethod
    def is_listening(cls) -> bool:
        return cls.LISTENING.value() >= 1

    @classmethod
    def is_stopping(cls) -> bool:
        return cls.LISTENING.value() == 0

    @classmethod
    def is_ready(cls) -> bool:
        if not cls._is_ready:
            cls._is_ready = (cls.READY.value() >= cls.nproc)
        return cls._is_ready

    @classmethod
    def is_online(cls) -> bool:
        return cls.is_listening() and cls.is_ready()

    @classmethod
    def is_offline(cls) -> bool:
        return cls.is_stopping() or not cls.is_ready()
