import tornado.gen
import tornado.ioloop
import tornado.iostream
import tornado.tcpclient


from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_PORT
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_RECONNECT_PERIOD


# ====================================================================================================================
class Connection(object):
    def __init__(self, host, **kwargs):
        super(Connection, self).__init__()
        self.host, self.port = self.parse_host(host)
        self.client = tornado.tcpclient.TCPClient()
        self.reconnect_period = kwargs.get('reconnect_period', UMEMCACHED_DEFAULT_RECONNECT_PERIOD)
        self.stream = None
        self.is_connecting = True
        self.active = False
        self.log = Logger.get('memcached.conn(%d)' % (kwargs.get('index', 0)))

        self._server_previous_started_at = 0
        self._server_started_at = 0
        self._server_uptime = 0

        self._server_restarted_callback = kwargs.get('server_restarted_callback')

    # ----------------------------------------------------------------------------------------------------------------
    def is_active(self):
        return self.active

    # ----------------------------------------------------------------------------------------------------------------
    def schedule_reconnect(self):
        if self.active or self.is_connecting:
            return

        self.log.warning('connection is not active, scheduling reconnect...')

        if self.is_connecting:
            tornado.ioloop.IOLoop().instance().call_later(self.reconnect_period, self.connect)
        else:
            tornado.ioloop.IOLoop().instance().spawn_callback(self.connect)

        self.is_connecting = True

    # ----------------------------------------------------------------------------------------------------------------
    def notify_server_restarted(self, prev, current):
        if self._server_restarted_callback:
            self.log.warning('server restart notification will be spawned')
            tornado.ioloop.IOLoop.instance().spawn_callback(self._server_restarted_callback, prev, current)
        else:
            self.log.warning('server restart notification should be sent, but no callback found')

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _update_server_uptime(self):
        stats = yield self.stats()

        if 'uptime' not in stats or 'time' not in stats:
            return False

        _uptime = int(stats['uptime'])
        _time = int(stats['time'])

        self._server_previous_started_at = self._server_started_at
        self._server_started_at = _time - _uptime

        if self._server_previous_started_at == 0:
            return True

        if self._server_started_at != self._server_previous_started_at:
            self.notify_server_restarted(self._server_previous_started_at, self._server_started_at)

        return True

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def connect(self):
        self.log.debug('connecting to server...')
        try:
            self.stream = yield self.client.connect(self.host, self.port)
            if self.stream and not self.stream.closed():
                self.active = True
                self.is_connecting = False

            self.log.debug('connected to {}'.format(self.host))

            yield self._update_server_uptime()
        except Exception as ex:
            self.active = False
            self.is_connecting = False
            self.log.error('failed to connect ({}:{}): {}'.format(self.host, self.port, ex))

        return self.stream

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _read_and_decode_line(self):
        resp = yield self.stream.read_until(b'\r\n')
        resp = resp[:-2].decode('ascii')
        return resp

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def stats(self):
        try:
            yield self.stream.write(b'stats\r\n')

            data = {}

            resp = yield self._read_and_decode_line()
            while resp != 'END':
                _, key, value = resp.split(None, 2)
                data[key] = value
                resp = yield self._read_and_decode_line()

            return data
        except Exception as ex:
            self.log.warning('failed to get server stats, marking connection as inactive {}'.format(ex))
            self.active = False

        return {}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def delete(self, key):
        try:
            cmd = b'delete ' + key.encode('utf-8') + b'\r\n'

            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            return resp == 'DELETED'
        except Exception as ex:
            self.log.warning('failed to set key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return False

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def incr(self, key, value):
        cmd = b'incr ' + key.encode('utf-8') + b' ' + str(value).encode('utf-8') + b'\r\n'
        try:
            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            if resp == 'NOT_FOUND':
                return None
            elif resp.startswith('CLIENT_ERROR'):
                return None

            return int(resp)
        except Exception as ex:
            self.log.warning('failed to increment key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return None

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def decr(self, key, value):
        cmd = b'decr ' + key.encode('utf-8') + b' ' + str(value).encode('utf-8') + b'\r\n'

        try:
            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            if resp == 'NOT_FOUND':
                return None
            elif resp.startswith('CLIENT_ERROR'):
                return None

            return int(resp)
        except Exception as ex:
            self.log.warning('failed to decrement key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return None

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def add(self, key, value, exptime=0):
        try:
            if isinstance(value, str):
                value = value.encode('utf-8')

            size = len(value)
            cmd = b'add '
            cmd = cmd + key.encode('utf-8') + b' 0 '
            cmd = cmd + str(exptime).encode('ascii') + b' '
            cmd = cmd + str(size).encode('ascii') + b'\r\n'
            cmd = cmd + value + b'\r\n'

            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            return resp == 'STORED'
        except Exception as ex:
            self.log.warning('failed to set key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return False

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _get_read_value(self, binary=False, cas=False):
        try:
            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            if resp.startswith('VALUE'):
                cas_unique = None
                if not cas:
                    key, flags, size = resp.split()[1:]
                else:
                    key, flags, size, cas_unique = resp.split()[1:]

                size = int(size)
                value = yield self.stream.read_bytes(size + 2)

                if binary:
                    value = value[:-2]
                else:
                    value = value[:-2].decode('utf-8')

                return (key, value, cas_unique)
        except Exception as ex:
            self.active = False
            self.is_connecting = False
            self.log.error('failed to _get_read_value: {}'.format(ex))

        return (None, None, None)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def get(self, *keys, as_dict=False, binary=False):
        cmd = 'get {}\r\n'.format(' '.join(keys)).encode('utf-8')

        result = None
        try:
            yield self.stream.write(cmd)

            if len(keys) == 1:
                _, result, _ = yield self._get_read_value(binary=binary)

                if result:
                    _, end, _ = yield self._get_read_value(binary=binary)

                if as_dict:
                    result = {keys[0]: result} if result else {}
            elif len(keys) > 1:
                result = {}
                key, value, _ = yield self._get_read_value(binary=binary)
                while key:
                    result[key] = value
                    key, value, _ = yield self._get_read_value(binary=binary)
            else:
                result = None
        except Exception as ex:
            self.log.warning('failed to get key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return result

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def gets(self, key: str, binary=False):
        value = None
        cas = None

        cmd = 'gets {}\r\n'.format(key).encode('utf-8')

        try:
            yield self.stream.write(cmd)

            _, value, cas = yield self._get_read_value(binary=binary, cas=True)

            if value or cas:
                _, end, _ = yield self._get_read_value(binary=binary)
        except Exception as ex:
            self.log.warning('failed to get key value with cas: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return (value, cas)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def cas(self, key, value, cas_unique, exptime=0):
        ok = False
        found = False
        cas_ok = False

        try:
            if isinstance(value, str):
                value = value.encode('utf-8')

            size = len(value)

            cmd = b'cas '
            cmd = cmd + key.encode('utf-8') + b' 0 '
            cmd = cmd + str(exptime).encode('ascii') + b' '
            cmd = cmd + str(size).encode('ascii') + b' '
            if cas_unique is not None:
                cmd = cmd + str(cas_unique).encode('ascii') + b'\r\n'
            else:
                cmd = cmd + b'0\r\n'
            cmd = cmd + value + b'\r\n'

            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')

            resp = resp[:-2].decode('ascii')

            if resp == 'STORED':
                ok = True
                found = True
                cas_ok = True
            elif resp == 'EXISTS':
                ok = True
                found = True
                cas_ok = False
            elif resp == 'NOT_FOUND':
                ok = True
                found = False
                cas_ok = False
            else:
                self.log.error('unexpected response {}, assuming it failed'.format(resp))
                ok = False
                found = False
                cas_ok = False

        except Exception as ex:
            self.log.warning('failed to get key value with cas: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return (ok, found, cas_ok)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def set(self, key, value, exptime=0):
        try:
            if isinstance(value, str):
                value = value.encode('utf-8')

            size = len(value)
            cmd = b'set '
            cmd = cmd + key.encode('utf-8') + b' 0 '
            cmd = cmd + str(exptime).encode('ascii') + b' '
            cmd = cmd + str(size).encode('ascii') + b'\r\n'
            cmd = cmd + value + b'\r\n'

            yield self.stream.write(cmd)

            resp = yield self.stream.read_until(b'\r\n')
            resp = resp[:-2].decode('ascii')

            return resp == 'STORED'
        except Exception as ex:
            self.log.warning('failed to set key value: "{}", marking connection as inactive'.format(ex))
            self.active = False

        return False

    # ----------------------------------------------------------------------------------------------------------------
    def parse_host(self, host):
        r = host.rsplit(':', 1)
        if len(r) == 2:
            try:
                return r[0], int(r[1])
            except:
                return r[0], UMEMCACHED_DEFAULT_PORT
        elif len(r) == 1:
            return r[0], UMEMCACHED_DEFAULT_PORT

    # ----------------------------------------------------------------------------------------------------------------
    def __str__(self):
        return 'conn/{}'.format(self.host)
