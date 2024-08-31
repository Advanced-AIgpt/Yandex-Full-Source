import tornado.ioloop
import tornado.gen
import tornado.queues

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.unimemcached.connection import Connection as DefaultConnection
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_POOL_SIZE
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_RECONNECT_PERIOD


# ====================================================================================================================
class ConnectionPool(object):
    def __init__(self, host, **kwargs):
        super(ConnectionPool, self).__init__()
        self.log = Logger.get('memcached.pool')
        self.host = host
        self.index = 0
        self.pool_size = kwargs.get('pool_size', UMEMCACHED_DEFAULT_POOL_SIZE)
        self.reconnect_period = kwargs.get('reconnect_period', UMEMCACHED_DEFAULT_RECONNECT_PERIOD)
        self.connections = tornado.queues.Queue(maxsize=self.pool_size)
        self.ConnectionClass = kwargs.get('connection_class', DefaultConnection)

        self._server_restarted_callback = kwargs.get(
            'server_restarted_callback',
            self._default_server_restarted_callback
        )

        self._server_started_timestamp = 0

    # ----------------------------------------------------------------------------------------------------------------
    def __str__(self):
        return 'pool/{}'.format(self.host)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _default_server_restarted_callback(self):
        self.log.info('default server_restarted_callback called, nothing to do')

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _on_server_restarted(self, prev_time, curr_time):
        if self._server_started_timestamp != curr_time:
            self._server_started_timestamp = curr_time
            yield self._server_restarted_callback(self.host, self.index)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def prepare_connections(self):
        futures_create = [self.create_connection() for i in range(0, self.pool_size)]
        connections = yield futures_create

        futures_connect = [conn.connect() for conn in connections]
        connected = yield futures_connect

        return connected

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def create_connection(self):
        self.log.debug('creating connection for host {}'.format(self.host))
        conn = self.ConnectionClass(
            self.host,
            index=self.index,
            reconnect_period=self.reconnect_period,
            server_restarted_callback=self._on_server_restarted
        )
        self.index += 1
        yield self.connections.put(conn)
        return conn

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def acquire(self):
        conn = yield self.connections.get()

        if not conn.is_active():
            conn.schedule_reconnect()
            self.connections.put_nowait(conn)
            return None

        return conn

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def release(self, conn):
        self.connections.put_nowait(conn)
