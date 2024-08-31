from tornado.websocket import websocket_connect
from tornado.ioloop import IOLoop
from tornado import gen
import tornado.httpserver
import functools
from alice.uniproxy.library.settings import config
from rtlog_grip import init as init_rtlog
from main import application
from alice.uniproxy.library.unisystem.uniwebsocket import UniWebSocket

import pytest
import json
import struct
import time
from subway.client import subway_init as init_subway
from alice.uniproxy.library.backends_memcached import memcached_init as init_umc
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.logging import Logger
from functools import wraps
import code, traceback, signal

from subway.server.subway_server import SubwayServer


class Session(object):
    _error = None
    _socket = None
    _timeout = None

    @classmethod
    def clear(cls):
        cls._error = None
        if cls._timeout:
            IOLoop.current().remove_timeout(cls._timeout)
            cls._timeout = None

    @classmethod
    def error(cls):
        return cls._error

    @classmethod
    def set_error(cls, err):
        cls._error = err

    @classmethod
    def fuckit(cls, msg):
        Logger.get().debug('got timeout ({})'.format(cls._timeout))
        cls._error = msg
        IOLoop.current().stop()

    @classmethod
    def set_socket(cls, socket):
        cls._socket = socket

    @classmethod
    def socket(cls):
        return cls._socket

    @classmethod
    def set_timeout(cls, timeout):
        Logger.get().debug('set timeout={}'.format(timeout))
        cls._timeout = IOLoop.current().call_later(timeout, functools.partial(cls.fuckit, "Session timeout"))


def connect(server, on_connect, on_message, timeout=3, connect_timeout=10):
    connect_start_time = time.time()

    def on_connect_save_socket(res):
        try:
            if res.exception():
                if time.time() - connect_start_time >= connect_timeout:
                    Session.fuckit(res.exception())
                    return
                else:
                    websocket_connect(
                        'ws://localhost:%s/uni.ws' % (config['port'], ),
                        callback=on_connect_save_socket,
                        on_message_callback=on_message
                    )
                    return
            Session.set_socket(res.result())
        except:
            IOLoop.current().stop()
            return
        on_connect(Session.socket())

    Session.clear()
    websocket_connect(
        "ws://localhost:%s/uni.ws" % (config["port"],),
        callback=on_connect_save_socket,
        on_message_callback=on_message)
    Session.set_timeout(timeout)
    IOLoop.current().start()
    Logger.get().info('ioloop stopped')
    assert (Session.error() is None), str(Session.error())
    # Session._socket.close()


def exit_ioloop():
    IOLoop.current().stop()


SERVER_INSTANCE = None
SUBWAY_INSTANCE = None


def debug(sig, frame):
    """Interrupt running process, and provide a python prompt for
    interactive debugging."""
    d = {'_frame': frame}         # Allow access to frame object.
    d.update(frame.f_globals)  # Unless shadowed by global
    d.update(frame.f_locals)

    i = code.InteractiveConsole(d)
    message = "Signal received : entering python shell.\nTraceback:\n"
    message += ''.join(traceback.format_stack(frame))
    i.interact(message)


def listen():
    signal.signal(signal.SIGUSR1, debug)  # Register handler


def _server(subway=True):
    NPROCS = 1
    listen()
    print('listen for debug on SIGUSR1')

    global SERVER_INSTANCE
    global SUBWAY_INSTANCE

    if SERVER_INSTANCE:
        return SERVER_INSTANCE

    Logger.init("test", True)
    init_rtlog()
    GlobalState.init(NPROCS)

    SUBWAY_INSTANCE = SubwayServer(port=7777)
    SUBWAY_INSTANCE.start()

    SERVER_INSTANCE = tornado.httpserver.HTTPServer(application, xheaders=True)
    SERVER_INSTANCE.bind(config["port"])
    SERVER_INSTANCE.start(NPROCS)

    if subway:
        init_subway(False)
    GlobalCounter.init()
    GlobalState.set_listening()
    GlobalState.set_ready()

    return SERVER_INSTANCE

@pytest.fixture()
def server():
    return _server()

@pytest.fixture()
def no_subway_server():
    return _server(subway=False)

def spawn_all(*callbacks):
    for c in callbacks:
        IOLoop.current().spawn_callback(c)


def write_json(socket, js):
    socket.write_message(json.dumps(js))


def write_data(socket, stream_id, data):
    socket.write_message(struct.pack(">I", stream_id) + data, binary=True)


@gen.coroutine
def read_message(socket):
    msg = yield socket.read_message()
    if isinstance(msg, bytes):
        raise gen.Return(msg)
    else:
        raise gen.Return(json.loads(msg))


@gen.coroutine
def read_stream(socket):
    stream_len = 0
    stream_control = None
    while True:
        message = yield read_message(socket)
        if isinstance(message, bytes):
            stream_len += len(message)
        else:
            stream_control = message
            break
    raise gen.Return((stream_len, stream_control))


def write_streamcontrol(socket, stream_id, message_id, action=0):
    write_json(socket, {
        "streamcontrol": {
            "streamId": stream_id,
            "action": action,
            "reason": 0,
            "messageId": message_id
        }
    })


@gen.coroutine
def write_stream(socket, file_name, stream_id, message_id, stop_callback=None, send_streamcontrol=True, chunk_size=9600, delay=0.2):
    with open(file_name, "rb") as wav_data:
        while True:
            chunk = wav_data.read(chunk_size)
            should_stop = stop_callback and stop_callback()
            if chunk and not should_stop:
                write_data(socket, stream_id, chunk)
            elif send_streamcontrol:
                write_streamcontrol(socket, stream_id, message_id)
                break
            elif should_stop:
                break
            elif not chunk:
                break
            yield gen.sleep(delay)


def async_test(func):
    @wraps(func)
    def _decorator(**kwargs):
        IOLoop.current().run_sync(lambda: func(**kwargs))
    return _decorator


def recursive_merge_dict(destination, source):
    for key, value in source.items():
        if isinstance(value, dict):
            # get node or create one
            node = destination.setdefault(key, {})
            recursive_merge_dict(node, value)
        else:
            destination[key] = value

