import collections
import time

import tornado.gen

from .client import Client
from .connection import Connection


class FakeValue(object):
    def __init__(self):
        super().__init__()
        self.data = None
        self.ts = 0
        self._cas = int(time.time() * 1000000000) * 1000

    def set(self, value: bytes, expire=0) -> bool:
        self.data = value
        self.ts = int((time.time() + expire) * 1000) if expire else 0
        self._cas = int(time.time() * 1000000000) * 1000
        return True

    def cas(self, value: bytes, cas: str, expire=0) -> (bool, bool):
        if str(self._cas) != cas:
            return True, True, False
        return True, True, self.set(value, expire)

    def get(self) -> bytes:
        if self.data is None:
            return None

        if self.ts == 0:
            return self.data

        if self.ts >= int(time.time() * 1000):
            return self.data

        return None

    def gets(self) -> (bytes, str):
        value = self.get()
        if value:
            return value, str(self._cas)
        return None, ''

    def incr(self, value: int) -> int:
        if self.data is None:
            return None

        try:
            x = int(self.data.decode('utf-8'))
            x = x + value
            self.data = str(x).encode('utf-8')
        except Exception:
            return None

        return x

    def decr(self, value: int) -> int:
        if self.data is None:
            return None

        try:
            x = int(self.data.decode('utf-8'))
            x = x - value
            self.data = str(x).encode('utf-8')
        except Exception:
            return None

        return x


class FakeServer(object):
    def __init__(self):
        super().__init__()
        self.data = collections.defaultdict(FakeValue)

    def add(self, key: str, value: bytes, expire=0):
        if key in self.data:
            return False
        return self.data[key].set(value, expire)

    def set(self, key: str, value: bytes, expire=0):
        return self.data[key].set(value, expire)

    def delete(self, key: str):
        if key in self.data:
            del self.data[key]
        return True

    def cas(self, key: str, value: bytes, cas: str, expire: int):
        if key not in self.data:
            return True, False, False
        return self.data[key].cas(value, cas, expire)

    def get(self, key: str):
        return self.data[key].get()

    def gets(self, key: str):
        if key in self.data:
            return self.data[key].gets()
        return None, None

    def incr(self, key: str, value: int):
        return self.data[key].incr(value)

    def decr(self, key: str, value: int):
        return self.data[key].decr(value)


g_fake_servers = collections.defaultdict(FakeServer)


class ConnectionMock(Connection):
    def __init__(self, host, *args, **kwargs):
        global g_fake_servers
        super().__init__(host, *args, **kwargs)
        self.server = g_fake_servers[host]

    def is_active(self):
        return True

    @tornado.gen.coroutine
    def connect(self):
        return True

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def stats(self):
        return {
            'time': int(time.time()),
            'uptime': 0,
        }

    @tornado.gen.coroutine
    def delete(self, key):
        return self.server.delete(key)

    @tornado.gen.coroutine
    def incr(self, key, value):
        return self.server.incr(key, value)

    @tornado.gen.coroutine
    def decr(self, key, value):
        return self.server.decr(key, value)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def add(self, key, value, exptime=0):
        if isinstance(value, str):
            value = value.encode('utf-8')
        elif not isinstance(value, bytes):
            value = str(value).encode('utf-8')

        return self.server.add(key, value, exptime)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def get(self, *keys, as_dict=False, binary=False):
        result = {}

        for key in keys:
            value = self.server.get(key)
            if value is not None:
                if binary:
                    result[key] = value
                else:
                    result[key] = value.decode('utf-8')

        return result

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def gets(self, key: str, binary=False):
        value, cas = self.server.gets(key)

        if value and cas:
            if binary:
                return value, cas
            return value.decode('utf-8'), cas

        return None, None

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def cas(self, key, value, cas_unique, exptime=0):
        if isinstance(value, str):
            value = value.encode('utf-8')
        return self.server.cas(key, value, cas_unique, exptime)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def set(self, key, value, exptime=0):
        if isinstance(value, str):
            value = value.encode('utf-8')
        return self.server.set(key, value, exptime)


# ====================================================================================================================
class ClientMock(Client):
    def __init__(self, *args, **kwargs):
        kwargs['connection_class'] = ConnectionMock
        super().__init__(*args, **kwargs)

    def enumerate_server_mocks(self):
        global g_fake_servers
        yield from g_fake_servers.items()
