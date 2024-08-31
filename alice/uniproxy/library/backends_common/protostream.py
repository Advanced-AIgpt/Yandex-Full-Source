import datetime
import re
import time
from tornado import gen
from tornado.ioloop import IOLoop
from tornado.tcpclient import TCPClient
from alice.uniproxy.library.logging import Logger


class ProtoStream(object):
    @property
    def peername(self):
        return "{}:{}".format(*self.stream.socket.getpeername()) if (self.stream and self.stream.socket) else "no"

    @property
    def sockname(self):
        return "{}:{}".format(*self.stream.socket.getsockname()) if (self.stream and self.stream.socket) else "no"

    def __init__(self, host, port, uri, rt_log=None, rt_log_label=None):
        self._log = Logger.get('.backends_common.protostream')
        self.stream = None
        self.host = host
        self.port = port
        self.uri = uri
        self._closed = False
        self._connected = False
        self.rt_log = rt_log
        self.rt_log_token = None
        self.rt_log_label = rt_log_label
        self.failed = False
        self.message_id = None
        self.balancing_hint_header = None

    def __del__(self):
        self.close()

    def connect(self, timeout=0.3, retries=2, request_timeout=3):
        # spawn connect coroutine
        IOLoop.current().spawn_callback(self._connect_coro, timeout, retries, request_timeout)

    @gen.coroutine
    def _connect_coro(self, connect_timeout, connect_retries, request_timeout):
        start_ts = time.monotonic()
        if request_timeout is None:
            request_timeout = 24 * 60 * 60  # 1day
        if connect_timeout is None:
            connect_timeout = request_timeout
        connect_timeout = min(connect_timeout, request_timeout)
        stream = None
        err = 'unknown'
        while True:
            try:
                begin_connect_ts = time.monotonic()
                stream = yield gen.with_timeout(
                    datetime.timedelta(seconds=connect_timeout),
                    TCPClient().connect(self.host, self.port)
                )
                if self._closed:
                    self.on_error(self._format_error('connection canceled'), code=596)
                    return

                # calc connect duration and put to histogramm
                self.on_connect(time.monotonic() - begin_connect_ts)
            except (TimeoutError, gen.TimeoutError):
                err = 'connect timeout({}) to {}:{}'.format(connect_timeout, self.host, self.port)
            except Exception as exc:
                err = 'connection to {}:{} failed: {}'.format(self.host, self.port, str(exc))
            if stream:
                break  # SUCCESS
            if connect_retries:
                used_time = time.monotonic() - start_ts
                if request_timeout > used_time:
                    connect_timeout = min(connect_timeout, request_timeout - used_time)
                    connect_retries -= 1
                    continue

            self.on_error(self._format_error(err), code=599)
            return

        self.stream = stream
        self._connected = True
        self._log.debug("{}->{} connected".format(self.sockname, self.peername))

        request_timeout -= time.monotonic() - start_ts
        if request_timeout < 0:
            self.on_error(
                self._format_error('connect timeout({}) [2] to {}:{}'.format(connect_timeout, self.host, self.port)),
                code=599,
            )
            return

        yield self._make_handshake(request_timeout)

    def on_connect(self, duration):
        pass

    @gen.coroutine
    def _make_handshake(self, request_timeout):
        headers = ""
        if self.rt_log and self.rt_log_label:
            self.rt_log_token = self.rt_log.log_child_activation_started(self.rt_log_label)
            if self.rt_log_token is None:
                if self.message_id is not None:
                    fake_token = 'MSGID-{}'.format(self.message_id)
                    self._log.info('Proto.RtLogToken: {} {} {}'.format(self.host, self.uri, fake_token))
                    headers = "X-RTLog-Token: {0}\r\n".format(fake_token)
            else:
                self._log.info('Proto.RtLogToken {} {} (msg:{}): {}'.format(
                    self.host, self.uri, self.message_id, self.rt_log_token))
                headers = "X-RTLog-Token: {0}\r\n".format(
                    self.rt_log_token.decode('ascii') if isinstance(self.rt_log_token, bytes) else self.rt_log_token)

        if self.balancing_hint_header:
            headers += self.balancing_hint_header

        self.stream.write(
            (
                "GET %s HTTP/1.1\r\n" % (self.uri)
                + "User-Agent:KeepAliveClient\r\n"
                + "Host: %s\r\n" % (self.host)
                + "Upgrade: protobuf\r\n"
                + "Connection: Upgrade\r\n"
                + headers
                + "\r\n"
            ).encode("utf-8")  # it may be uncaught exception here
        )
        try:
            response = yield gen.with_timeout(
                datetime.timedelta(seconds=request_timeout),
                self.stream.read_until(b'\r\n\r\n')
            )
        except (TimeoutError, gen.TimeoutError):
            self.on_error(
                self._format_error('handshake request timeout {}'.format(request_timeout)),
                code=599,
            )
            return
        except Exception as exc:
            self.on_error(
                self._format_error('fail read response: {}'.format(str(exc))),
                code=599,
            )
            return

        # check upgrade response
        if b"HTTP/1.1 101" not in response:
            self.on_fail_http_upgrade(response)
            self.on_error(self._format_error('bad upgrade response'), code=self.extract_code(response))
            self._log.error(response, rt_log=self.rt_log)
            return

        self.process()

    def is_closed(self):
        if self._closed:
            return True
        if not self.stream or self.stream.closed():
            self.close()
            return True
        return False

    def is_connected(self):
        return self._connected

    def close(self):
        self._closed = True
        if self.stream and not self.stream.closed():
            self.stream.close()
        self.stream = None
        if self.rt_log and self.rt_log_token:
            self.rt_log.log_child_activation_finished(self.rt_log_token, not self.failed)
            self.rt_log_token = None

    def on_error(self, err, code):
        self.mark_as_failed()
        self.close()
        self._log.error("error_callback: code={} {}".format(code, err), rt_log=self.rt_log)

    @gen.coroutine
    def _read_protobuf_ex_message_size(self):
        if self.is_closed():
            return None

        size = None

        try:
            data = yield self.stream.read_until(b"\r\n")
            size = int(data[:-2], 16)
        except:
            size = None

        return size

    @gen.coroutine
    def _read_protobuf_ex_message_body(self, size, protos):
        if size is None:
            return None

        body = yield self.stream.read_bytes(size)

        for ProtoType in protos:
            data = ProtoType()
            try:
                data.ParseFromString(body)
                return data
            except Exception as e:
                self._log.debug("Can't parse %s" % (e), rt_log=self.rt_log)

        return None

    @gen.coroutine
    def _read_protobuf_ex_impl(self, protos):
        size = yield self._read_protobuf_ex_message_size()

        body = yield self._read_protobuf_ex_message_body(size, protos)
        return body

    def read_protobuf_ex(self, protos):
        protos = protos if isinstance(protos, (list, tuple)) else (protos,)
        return self._read_protobuf_ex_impl(protos)

    def read_protobuf(self, protos, callback):
        ProtoTypes = protos if isinstance(protos, (list, tuple)) else (protos,)

        def read_message(future):
            if future.exception():
                self.close()
                self._log.error("read_message: %s" % (future.exception()), rt_log=self.rt_log)
                callback(None)
            else:
                res = None
                message = future.result()
                for ProtoType in ProtoTypes:
                    res = ProtoType()
                    try:
                        res.ParseFromString(message)
                        break
                    except Exception as e:
                        self._log.debug("Can't parse %s" % (e), rt_log=self.rt_log)
                try:
                    callback(res)
                except Exception as e:
                    self._log.exception('ProtoStream.callback throw exception: {}'.format(e), rt_log=self.rt_log)

        def read_message_size(future):
            if future.exception():
                self.close()
                self._log.error("read_message_size: %s" % (future.exception()), rt_log=self.rt_log)
                callback(None)
            elif self.is_closed():
                callback(None)
            else:
                data = future.result()
                size = int(data[:-2], 16)
                self._log.debug("Message size: %s" % (size), rt_log=self.rt_log)
                if self.stream:
                    self.stream.read_bytes(size).add_done_callback(read_message)
                else:
                    callback(None)
        if self.is_closed():
            callback(None)
        else:
            self.stream.read_until(b"\r\n").add_done_callback(read_message_size)

    @gen.coroutine
    def send_protobuf_ex(self, proto):
        message = None

        try:
            message = proto.SerializeToString()

            if self.is_closed():
                return False

            yield self.stream.write(
                hex(len(message)).encode("utf-8") + b"\r\n" + message
            )
        except Exception as ex:
            self.close()
            self._log.error("send_protobuf: %s" % (ex), rt_log=self.rt_log)
            return False

        return True

    def send_protobuf(self, proto, callback):
        def on_send(future):
            if future.exception():
                self.close()
                self._log.error("on_send: %s" % (future.exception()), rt_log=self.rt_log)
                callback(False)
            else:
                callback(True)

        message = None
        try:
            message = proto.SerializeToString()
        except Exception as e:
            self.close()
            self._log.error("send_protobuf: %s" % (e), rt_log=self.rt_log)
            callback(False)
            return
        if self.is_closed():
            callback(False)
        else:
            self.stream.write(
                hex(len(message)).encode("utf-8") + b"\r\n" + message
            ).add_done_callback(on_send)

    def mark_as_failed(self):
        self.failed = True

    def process(self):
        pass

    def on_fail_http_upgrade(self, http_resp):
        pass

    def extract_code(self, response):
        code = re.findall(rb"HTTP[/\.0-9]* (\d*) ", response)
        if code:
            try:
                code = int(code[0])
            except Exception:
                code = 599
        else:
            code = 599
        return code

    def _format_error(self, err):
        if not self.rt_log_label:
            return err

        return '{}: {}'.format(self.rt_log_label, err)
