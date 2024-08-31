import time
import tornado.gen
import tornado.platform.asyncio

from enum import Enum, unique
from functools import partial
from tornado.ioloop import IOLoop

from alice.uniproxy.library.settings import config

from alice.uniproxy.library.backends_common.protostream import ProtoStream
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json, proto_to_json

from voicetech.library.proto_api.yabio_pb2 import YabioRequest, Method, AddData
from voicetech.library.proto_api.yabio_pb2 import YabioResponse, AddDataResponse

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import deepupdate
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import BIOMETRY_SCORING_CONFIDENCE_HGRAM


def get_by_path(dct, *keys):
    for k in keys:
        dct = dct.get(k, None)
        if dct is None:
            return None
    return dct


class YabioStream(ProtoStream):
    _NATIVE_FORMAT = "audio/x-pcm;bit=16;rate=16000"

    @unique
    class YabioStreamType(Enum):
        Score = "yabio-score"
        Classify = "yabio-classify"

    default_json = {
        "hostName": "",
        "sessionId": "",
        "uuid": "",
        "mime": "audio/opus",
    }

    def __init__(
        self, stream_type: YabioStreamType, callback, error_callback, params, session_id,
        host=None, spotter=False, rt_log=None, rt_log_label=None, system=None, message_id=None,
        stream_id=None, close_callback=None,
    ):
        # current inheritance from ProtoStream architecture can cause access to self._timeout
        # even if we throw exception here (method __del__), so init members as early as possible
        self._timeout = None
        self.context_future = None
        self.session_id = session_id + ' yabio:'
        self.params = deepupdate(
            YabioStream.default_json, {
                'sessionId': '{}_{}'.format(session_id, message_id)
            }
        )
        if system:
            self.params['clientHostname'] = system.hostname
            if system.uaas_bio_flags:
                self.params['experiments'] = system.uaas_bio_flags

        self.params.update(params)

        super(YabioStream, self).__init__(
            host or config["yabio"]["host"],
            config["yabio"]["port"],
            config["yabio"].get("uri", "/bio"),
            rt_log,
            rt_log_label
        )
        self._log = Logger.get('.yabiostream')

        self.orig_session_id = session_id
        self.stream_id = stream_id
        enrolling_ids = params.get('_test_request_ids', None)
        if enrolling_ids:
            self.message_id = enrolling_ids.pop(0)
        else:
            self.message_id = message_id

        self.stream_type = stream_type
        self.spotter = spotter
        self.params["method"] = Method.Value(stream_type.name)
        if "classification_tags" not in self.params:
            self.params["classification_tags"] = self.params.get("biometry_classify", "")
            if isinstance(self.params["classification_tags"], str):
                self.params["classification_tags"] = self.params["classification_tags"].split(",")
        self.group_id = self.get_group_id(params)
        self.params["group_id"] = self.group_id
        if stream_type != YabioStream.YabioStreamType.Classify and not self.group_id:
            raise ValueError("all yabio methods except Classify MUST have parameter: biometry_group (or substitution)")

        self.callback = callback
        self.error_callback = error_callback
        self.close_callback = close_callback

        self.init_sent = False
        self.last_chunk = False
        self.reading = False
        self.enqueued_chunks = 0
        self.sent_chunks = 0
        self.need_results = 0
        self.processed_results = 0
        self.processed_chunks = 0
        self.buffered = []
        self.has_finish_code = False

        self._system = system
        if self.stream_type == YabioStream.YabioStreamType.Score:
            # run separate coroutine for get context from ydb
            # (simultaneously connecting to yabio_server)
            self.context_storage = self._system.get_yabio_storage(self.group_id)
            self.context_future = self.context_storage.get_users_context()
        self.DLOG('connect to {}:{}'.format(self.host, self.port))
        self.connect()

    def on_connect(self, duration):
        GlobalTimings.store('yabio_connect_time', duration)

    @staticmethod
    def get_group_id(params):
        return params.get(
            "biometry_group",
            params.get(
                "advanced_options",
                {}
            ).get(
                "biometry_group",
                params.get("deviceUUID", "")
            )
        )

    def increment_unistat_counter(self, code):
        if self.has_finish_code:
            return

        self.has_finish_code = True
        GlobalCounter.increment_error_code("yabio", code)
        try:
            if self._system:
                self._system.increment_stats("yabio", code)
        except ReferenceError:
            pass

    def close(self):
        if self.close_callback:
            close_callback = self.close_callback
            self.close_callback = None
            try:
                close_callback()
            except Exception as exc:
                self.EXC('YabioStream got exception on close_calback: ' + str(exc))

        self.DLOG('close')
        if self._timeout is not None:
            IOLoop.current().remove_timeout(self._timeout)
            self._timeout = None
        if self.context_future:
            self.context_future.cancel()
            self.context_future = None
        super(YabioStream, self).close()

    def on_timeout(self):
        self.on_error("read yabio-server response timeout", code=600)

    def on_error(self, err, code=None):
        self.mark_as_failed()
        self.DLOG('error: code={} {}'.format(code, err))
        if code:
            self.increment_unistat_counter(code)
        if (not self.is_closed() and self.is_connected()) and self.error_callback is not None:
            err = 'yabio ' + str(err)
            self.error_callback(err)
        if self.context_future is not None:
            try:
                self.context_future.cancel()
                self.context_future = None
            except Exception as exc:
                self.EXC('fail cancel ' + str(exc))
        self.close()

    def on_data(self, *args):
        if not self.is_closed() and self.callback is not None:
            self.callback(*args)

    def last_result_is_actual(self):
        # return true if all received audio already processed
        self.DLOG('self.enqueued_chunks={} self.processed_chunks={} self._closed={}'.format(
            self.enqueued_chunks, self.processed_chunks, self._closed,
        ))
        return self.enqueued_chunks == self.processed_chunks or self._closed  # DONT use is_closed() !!

    @tornado.gen.coroutine
    def process(self):
        proto = None
        context = None
        if self.context_future is not None:
            try:
                context = yield self.context_future
            except Exception as exc:
                self.EXC("Fail get context: {}".format(exc))
                self.on_error("can't get yabio context: {}".format(exc))
                return
            finally:
                self.context_future = None
        else:
            if self.stream_type == YabioStream.YabioStreamType.Score:
                self.INFO('yabio process not found context_future (like have race with close())')
                self.on_error('yabio process not found context_future (like have race with close())')
                return

        try:
            proto = proto_from_json(YabioRequest, self.params)
            if context:
                proto.context.CopyFrom(context)
                if len(context.users) > 0:
                    GlobalCounter.YABIO_REGISTERED_USERS_SUMM.increment()
            proto.mime = self.params.get("format", YabioStream._NATIVE_FORMAT)
            if self.spotter:
                proto.spotter = True
        except Exception as exc:
            self.EXC("Can't create YabioRequest request: {} from {}".format(exc, self.params))
            self.on_error("Can't create YabioRequest")
            return
        self.send_protobuf(proto, self.read_init_response)

    def add_chunk(self, data=None, need_result=False, last_spotter_chunk=False, last_chunk=False, text=None):
        if self._closed:
            return None

        if self.last_chunk:
            # last chunk already enqueued
            if not need_result:
                return None

            if self.last_result_is_actual():
                # already got all responses
                return None

            # expect receiving response for last_chunk, so pretend we send request need_result
            self.INFO('simulate requesting need_result after last_chunk ??')
            return self.enqueued_chunks

        log_line = "got new chunk[{}] size={}".format(self.enqueued_chunks, 0 if data is None else len(data))
        if need_result:
            log_line += ' need_result'
        if last_spotter_chunk:
            log_line += ' end_spotter'
        if last_chunk:
            log_line += ' last_chunk'
        if text:
            log_line += ' text({})'.format(text)

        self.DLOG(log_line)

        if last_chunk:
            self.last_chunk = True
            data = b""
            # TODO: restore timeout here: ?? (now it moved to send_chunk())
            # if self.stream_type == YabioStream.YabioStreamType.Score:
            #     self._timeout = IOLoop.current().call_later(config["yabio"].get("timeout", 3.15), self.on_timeout)

        if (last_chunk or last_spotter_chunk) and not text:
            self.WARN('text should be present for last {} chunk'.format('' if last_chunk else 'spotter'))

        if self.stream_type == YabioStream.YabioStreamType.Score and text:
            if last_chunk:
                self.context_storage.add_text(self.message_id, 'request', text)
            if last_spotter_chunk:
                self.context_storage.add_text(self.message_id, 'spotter', text)

        need_result |= self.last_chunk
        req = AddData(audioData=data, lastChunk=self.last_chunk)
        if need_result:
            req.needResult = True
        if last_spotter_chunk:
            req.lastSpotterChunk = True
        self.enqueued_chunks += 1
        self.buffered.append(req)
        self.send_chunks()
        return self.enqueued_chunks

    def send_chunks(self):
        if self.init_sent:
            while self.buffered:
                data = self.buffered.pop(0)
                needResult = bool(data.needResult)
                needResult |= data.lastChunk
                if needResult:
                    self.need_results += 1
                if needResult and not data.audioData and self.sent_chunks == 0:
                    self.WARN("Need result and no receive any audio data at all")
                    self.on_data(None, None)
                    self.increment_unistat_counter(400)
                    self.close()
                    return

                self.sent_chunks += 1
                self.DLOG('send protobuf needResult={}'.format(needResult))
                self.send_protobuf(data, partial(self.read_add_data_response, needResult))
                if data.lastChunk:
                    self._timeout = IOLoop.current().call_later(config["yabio"].get("timeout", 3.15), self.on_timeout)

    def read_init_response(self, send_result):
        def process_init_response(res):
            if res is None or res.responseCode != 200:
                self.WARN("Bad request to yabio")
                self.WARN(res)
                self.on_error("Connection to yabio failed", code=res.responseCode if res else 600)
            else:
                self.INFO("Successfully inited with {}".format(res.hostname))
                self.init_sent = True
                self.send_chunks()

        if not send_result:
            self.on_error("Connection to yabio failed")
        else:
            self.read_protobuf(YabioResponse, process_init_response)

    def read_add_data_response(self, read_response, send_result):
        def process_add_data_response(res):
            if self.is_closed():
                return

            if not res:
                self.on_error("Bad AddData response from yabio", code=600)
                return

            self.processed_results += 1
            self.processed_chunks += res.messagesCount
            last_result = self.last_chunk and self.need_results == self.processed_results
            user_matched = False

            result = {}
            if res.responseCode != 200:
                self.WARN('bad yabio responseCode={}'.format(res.responseCode))
                result = {
                    "status": "failed",
                    "error": "bad yabio responseCode={}".format(res.responseCode),
                }
            else:
                result = {
                    "status": "ok"
                }
                if self.stream_type == YabioStream.YabioStreamType.Score:
                    max_score = -1.0
                    scores_with_mode = []
                    for x in res.scores_with_mode:
                        scores = []
                        for s in x.scores:
                            scores.append({
                                'user_id': s.user_id,
                                'score': s.score,
                            })
                            max_score = max(max_score, s.score)
                        scores_with_mode.append({
                            'mode': x.mode,
                            'scores': scores,
                        })

                    if scores_with_mode:
                        result['scores_with_mode'] = scores_with_mode
                    else:
                        result['scores_with_mode'] = []

                    if max_score > 0:
                        GlobalTimings.store(BIOMETRY_SCORING_CONFIDENCE_HGRAM, max_score * 10)

                    if max_score > 0.5:
                        user_matched = True
                    result["group_id"] = self.group_id
                    for e in res.context.enrolling:
                        e.request_id = self.message_id
                        result["request_id"] = e.request_id
                        e.timestamp = int(time.time())
                        e.device_model = self._system.device_model or 'unknown'
                        e.device_id = get_by_path(self.params, 'request', 'device_state', 'device_id') or 'unknown'
                        e.device_manufacturer = get_by_path(
                            self.params, 'vins', 'application', 'device_manufacturer') or 'unknown'
                    # fork coroutine (can cause race if uniproxy client try create user from enrollings immediatly)
                    self.save_new_enrolling_coro(res.context.enrolling, res.supported_tags)
                elif self.stream_type == YabioStream.YabioStreamType.Classify:
                    jres = proto_to_json(res)
                    result['classification_results'] = jres['classificationResults']
                    result['bioResult'] = jres['classification']
            if result:
                try:
                    self.on_data(result, self.processed_chunks)
                except Exception as exc:
                    self.EXC('Exception on yabio callback: {}'.format(exc))

            self.DLOG(
                "yabio sent_chunks={} need_results={} processed_results={}"
                " processed_chunks={} read_next_response={}".format(
                    self.sent_chunks,
                    self.need_results,
                    self.processed_results,
                    self.processed_chunks,
                    self.need_results > self.processed_results,
                )
            )
            if last_result:
                self.increment_unistat_counter(res.responseCode)
                if user_matched:
                    GlobalCounter.YABIO_USER_MATCHED_SUMM.increment()
                self.close()
            elif self.need_results == self.processed_results:
                self.reading = False
            else:
                self.read_protobuf(AddDataResponse, process_add_data_response)

        if not send_result:
            self.on_error("AddData request failed")
        elif read_response and not self.reading:
            self.DLOG("waiting result")
            self.reading = True
            self.read_protobuf(AddDataResponse, process_add_data_response)

    @tornado.gen.coroutine
    def save_new_enrolling_coro(self, enrolling, supported_tags):
        try:
            yield self.context_storage.save_new_enrolling(enrolling, supported_tags)
        except Exception as exc:
            self.EXC('fail on save fresh enrollings: ' + str(exc))

    def DLOG(self, *args):
        self._log.debug(self.session_id, *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self.session_id, *args, rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self.session_id, *args, rt_log=self.rt_log)

    def ERR(self, *args):
        self._log.error(self.session_id, *args, rt_log=self.rt_log)

    def EXC(self, text):
        self._log.exception(self.session_id + ' ' + text, rt_log=self.rt_log)
