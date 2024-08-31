import collections.abc
import json
import sys
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from tornado import gen
from tornado.httpclient import HTTPRequest, HTTPError
from tornado.ioloop import IOLoop
from tornado.websocket import websocket_connect

if getattr(sys, "is_standalone_binary", False):
    from alice.library.python.decoder import Decoder
else:
    sys.path.append('/usr/lib/yandex/voice/python-decoder')  # noqa
    from decoder import Decoder  # pylint: disable=no-name-in-module

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.auth.blackbox import blackbox_client
from alice.uniproxy.library.auth.tvm2 import service_ticket_for_music


MAX_WS_MESSAGE_SIZE = 32768


def safeGet(d, path):
    if d is None:
        return None
    if not isinstance(d, collections.abc.Mapping):
        return None
    if len(path) == 1:
        return d.get(path[0])
    return safeGet(d.get(path[0]), path[1:])


def update(d, u):
    for k, v in u.items():
        if isinstance(v, collections.abc.Mapping):
            r = update(d.get(k, {}), v)
            d[k] = r
        else:
            d[k] = u[k]
    return d


class MusicStream2:
    connect_timeout_fmt = 'Connect timeout (to music recognition server websocket {})'

    def __init__(self, on_result, on_error, params, uuid=None, api_key=None,
                 oauth_token=None, client_ip=None, client_port=None, rt_log=None, rt_log_label=None):
        GlobalCounter.MUSIC2_REQUEST_SUMM.increment()
        self.params = {
            'music_request2': {
                'headers': dict(config['music']['headers'])
            }
        }
        update(self.params, params)
        self._log = Logger.get('.musicstream')
        self.on_result_callback = on_result
        self.on_error = on_error
        self.finished = False
        self.decoder = None
        self.time_limit = self.params.get('music_request2').get('time_limit', 19)
        self.timeout = None
        self.request_finalized = False
        music_request = self.params['music_request2']
        self.headers = music_request['headers']
        self.oauth_token = oauth_token
        self.client_ip = client_ip
        self.client_port = client_port
        self.rt_log = rt_log
        if oauth_token and config['music'].get('proxy_oauth', False):
            has_authorization = False
            for k in self.headers:
                if k.lower() == 'authorization':
                    has_authorization = True
                    break
            if not has_authorization:
                self.headers['Authorization'] = 'OAuth ' + oauth_token
        if uuid:
            self.headers['Uniproxy-Uuid'] = uuid
        if api_key:
            self.headers['Uniproxy-Client-Key'] = api_key
        if self.headers.get('Content-Type', '') != 'audio/pcm-data':
            # decode unknown format to preffered pcm-data (16bit/8kHz)
            self._log.debug('MusicStream2: create decoder', rt_log=self.rt_log)
            self.decoder = Decoder(8000)
            self.headers['Content-Type'] = 'audio/pcm-data'
        self.socket = None
        self.url = config['music']['websocket_url']
        if self.rt_log and rt_log_label:
            self.rt_log_token = self.rt_log.log_child_activation_started(rt_log_label)
        else:
            self.rt_log_token = None
        self.failed = False
        IOLoop.current().spawn_callback(self.get_tickets_and_connect)

    @gen.coroutine
    def get_tickets_and_connect(self):
        try:
            try:
                ticket = yield service_ticket_for_music(rt_log=self.rt_log)
                self.headers['X-Ya-Service-Ticket'] = ticket
            except Exception as exc:
                self._log.exception('fail getting service ticket: ' + str(exc), rt_log=self.rt_log)
            if self.oauth_token and self.client_ip:
                try:
                    self.headers['X-Ya-User-Ticket'] = yield blackbox_client().ticket4oauth(
                        self.oauth_token,
                        self.client_ip,
                        self.client_port,
                        rt_log=self.rt_log,
                    )
                except Exception as exc:
                    self._log.exception('music2: can not get user-ticket: ' + str(exc), rt_log=self.rt_log)
            if self.client_ip:
                self.headers['X-Forwarded-For'] = self.client_ip
            if self.request_finalized:  # music request canceled
                return
            logged_headers = self.headers.copy()
            for k in logged_headers:
                if k.lower() in ['authorization', 'x-ya-user-ticket', 'x-ya-service-ticket']:
                    val = logged_headers[k]
                    if isinstance(val, str):
                        logged_headers[k] = '<censored>'
            self._log.info('MusicStream2 request headers={}'.format(logged_headers), rt_log=self.rt_log)
            socket = yield self.connect()
            if self.request_finalized:  # music request canceled
                socket.close()
                return

            self.socket = socket  # web socket connected
            self.timeout = IOLoop.current().call_later(self.time_limit, self.on_timeout)
        except HTTPError as exc:
            self.fail(str(exc))
        except Exception as exc:
            self._log.exception('music2', rt_log=self.rt_log)
            self.fail(str(exc))

    def connect(self):
        return websocket_connect(
            HTTPRequest(
                self.url,
                headers=self.headers,
                validate_cert=False,
                connect_timeout=config.get('music', {}).get('connect_timeout', 0.5),
                request_timeout=config.get('music', {}).get('request_timeout', 0.5),
            ),
            on_message_callback=self.on_message,
        )

    def fail(self, err):
        if self.finished:
            return
        self.request_finalized = True
        self.failed = True
        GlobalCounter.MUSIC2_OTHER_ERR_SUMM.increment()
        self._log.warning(
            'MusicStream2.fail url={} headers={} error={}'.format(self.url, str(self.headers), str(err)),
            rt_log=self.rt_log
        )
        self.on_error(err)
        self.close()

    def on_timeout(self):
        self._log.debug('MusicStream2.on_timeout', rt_log=self.rt_log)
        GlobalCounter.MUSIC2_RESPONSE_TIMEOUT_SUMM.increment()
        self.on_result({"result": "response-timeout"})

    def on_message(self, msg):
        """
            handle here messages from music recognition server
        """
        self._log.debug('MusicStream2.on_message' + ('(EOF)' if msg is None else ''), rt_log=self.rt_log)
        if msg is None:
            if self.socket is None:
                return
            self.fail("Music API WebSocket closed unexpectedly (before return final response)")
            return

        if isinstance(msg, bytes):
            self.fail("Music API WebSocket returned some binary data")
            return

        try:
            js = json.loads(msg)
            dr = safeGet(js, ("directive", "header", "name"))
            if dr is None:
                self._log.warning('receive from Music API WebSocket event without directive/header/name: ' + str(msg))
                self.fail('receive from Music API WebSocket event without directive/header/name')
                return

            if dr == "classifying":
                music_detected = safeGet(js, ("directive", "payload", "musicDetected"))
                if music_detected is None:
                    self._log.warning('receive from Music API WebSocket classifying event without'
                                      ' directive/payload/musicDectected: ' + str(msg))
                    self.fail('receive from Music API WebSocket classifying event without'
                              ' directive/payload/musicDectected')
                elif music_detected:
                    # audio classified as music, but yet not recognize composition, so wait next message
                    self._log.debug('not finish music event', rt_log=self.rt_log)
                    GlobalCounter.MUSIC2_CLASSIFY_MUSIC_SUMM.increment()
                    self.on_result({"result": "music"}, finish=False)
                else:
                    GlobalCounter.MUSIC2_CLASSIFY_NOT_MUSIC_SUMM.increment()
                    self.on_result({"result": "not-music"})
                return

            result = safeGet(js, ("directive", "payload", "result"))
            if result is None:
                self._log.warning('bad music websocket api event: ' + str(msg))
                self.fail('receive from Music API WebSocket event without directive/payload/result')
            elif isinstance(result, collections.abc.Mapping):
                GlobalCounter.MUSIC2_SUCCESS_SUMM.increment()
                self.on_result({"result": "success", "data": result})
            else:
                GlobalCounter.MUSIC2_NO_MATCHES_SUMM.increment()
                self.on_result({"result": result})
        except Exception as err:
            self.fail(err)

    def add_chunk(self, data=b""):
        """
            decode input audio and write PCM to websocket
            (can accumulate some data inside Decoder on connecting duration)
        """
        if self.finished:
            return

        if self.decoder is not None:
            try:
                if not data:
                    # empy data == EOF marker
                    self.close()
                else:
                    self.decoder.write(data)
                    if self.socket:
                        pcm = b""
                        while True:
                            pcm_data = self.decoder.read()
                            if not pcm_data:
                                break
                            pcm += pcm_data
                        while pcm and self.socket and len(pcm):
                            sended_pcm = pcm
                            if len(pcm) > MAX_WS_MESSAGE_SIZE:
                                sended_pcm = pcm[:MAX_WS_MESSAGE_SIZE]
                                pcm = pcm[MAX_WS_MESSAGE_SIZE:]
                            else:
                                pcm = None
                            self._log.debug('send to music_asr2 {} bytes'.format(len(sended_pcm)), rt_log=self.rt_log)
                            self.socket.write_message(sended_pcm, binary=True)

            except Exception as err:
                self._log.warning("MusicStream2 decoding error: {}".format(repr(err)))
                self.fail(err)
        elif self.socket:
            try:
                self.socket.write_message(data, binary=True)
            except Exception as err:
                self.fail(err)

    def on_result(self, msg, finish=True):
        if self.finished:
            return
        self.request_finalized |= finish
        self.on_result_callback(msg, finish)
        if finish:
            self.close()

    def close(self):
        if self.finished:
            return

        self._log.debug('MusicStream2.close', rt_log=self.rt_log)
        if not self.request_finalized:
            self.request_finalized = True
            GlobalCounter.MUSIC2_FORCE_FINISH_SUMM.increment()
        if self.timeout:
            IOLoop.current().remove_timeout(self.timeout)
            self.timeout = None
        if self.decoder:
            self.decoder.close()
            self.decoder = None
        if self.socket:
            self.socket.close()
            self.socket = None
        if self.rt_log and self.rt_log_token:
            self.rt_log.log_child_activation_finished(self.rt_log_token, not self.failed)
            self.rt_log_token = None
        self.finished = True
