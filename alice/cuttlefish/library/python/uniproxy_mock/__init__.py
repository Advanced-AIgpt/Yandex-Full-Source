import tornado.websocket
import tornado.queues
import tornado.web
import tornado.concurrent
import logging
import datetime
from alice.cuttlefish.library.python.test_utils_with_tornado import wait_ping


logger = logging.getLogger("UniproxyMock")


# -------------------------------------------------------------------------------------------------
class PingHandler(tornado.web.RequestHandler):
    def get(self):
        self.write("pong")


# -------------------------------------------------------------------------------------------------
class UniproxyMockHandler(tornado.websocket.WebSocketHandler):
    def accept(self):
        self._on_open_fut.set_result(lambda x: None)

    def reject(self, code=400, reason=None):
        self._on_open_fut.set_result(lambda x: x.send_error(status_code=code, reason=reason))

    def __str__(self):
        try:
            socket = self.request.connection.stream.socket
            return f"WebSocket({socket.getpeername()}->{socket.getsockname()})"
        except AttributeError:
            return "WebSocket(unconnected)"

    async def read_message(self):
        logger.debug(f"{self} wait for message in queue...")
        msg = await self._msg_queue.get()
        logger.debug(f"{self}  got message from queue: {msg}")
        self._msg_queue.task_done()
        return msg

    async def write_message(self, msg, binary=False):
        await super().write_message(msg, binary=binary)
        if not binary:
            logger.debug(f"{self} sent WS message: {msg}")
        else:
            logger.debug(f"{self} sent binary message ({len(msg)} bytes)")

    def initialize(self, queue):
        self._on_open_fut = tornado.concurrent.Future()
        self._msg_queue = tornado.queues.Queue()
        queue.put_nowait(self)

    async def prepare(self):
        logger.debug("Got request for a new WS connection...")
        (await self._on_open_fut)(self)

    async def open(self):
        logger.debug("New WS connection has been established")
        self.set_nodelay(True)

    def on_message(self, message):
        logger.debug(f"{self} got new WS message: {message}")
        self._msg_queue.put_nowait(message)

    def on_close(self):
        logger.debug("{self} connection is closed by peer")
        self._msg_queue.put_nowait(None)


# -------------------------------------------------------------------------------------------------
class UniproxyMock:
    async def __aenter__(self):
        await self.start()
        return self

    async def __aexit__(self, *args, **kwargs):
        await self.stop()

    def __init__(self, port):
        self._conn_queue = tornado.queues.Queue()
        self._port = port

    @property
    def host(self):
        return "localhost"

    @property
    def port(self):
        return self._port

    @property
    def endpoint(self):
        return (self.host, self.port)

    async def get_connection(self, timeout=None):
        if timeout is not None:
            timeout = datetime.timedelta(seconds=timeout)
        conn = await self._conn_queue.get(timeout)
        self._conn_queue.task_done()
        logger.debug("Got new WS connection request")
        return conn

    async def accept(self, timeout=5):
        conn = await self.get_connection(timeout)
        conn.accept()
        return conn

    async def reject(self, timeout=5):
        conn = await self.get_connection(timeout)
        conn.reject()

    async def start(self):
        logger.info(f"Listen on port {self._port}")

        application = tornado.web.Application(
            [(r"/uni.ws", UniproxyMockHandler, dict(queue=self._conn_queue)), (r"/ping", PingHandler)]
        )

        self.server = application.listen(self._port)

        await wait_ping(f"http://localhost:{self._port}/ping", timeout=3)
        logger.info("Ready")

    async def stop(self):
        logger.info("Stopping...")
