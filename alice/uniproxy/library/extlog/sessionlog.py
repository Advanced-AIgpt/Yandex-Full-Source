import json
import weakref

from datetime import datetime
from alice.uniproxy.library.utils.deepcopyx import deepcopy
from rtlog import AsJson
from tornado.queues import Queue
from tornado import gen
from tornado.ioloop import IOLoop
from io import BytesIO

from alice.uniproxy.library.backends_common.storage import MdsStorage
from alice.uniproxy.library.backends_tts.ttsutils import generate_wav_header, channels_from_mime
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.utils.tree import dict_at_path
from alice.uniproxy.library.utils.tree import replace_tag_value
from alice.uniproxy.library.utils.tree import value_by_path
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config, DO_NOT_LOG_EVENTS
from .obscure import obscure_oauth_token_and_cookies


# only for qloud mssngr installation
is_qloud = config['is_qloud']
use_mds = config.get('mds', {}).get('enable', True)
do_not_log_events = [w for w in DO_NOT_LOG_EVENTS.split(':')]


def get_mds_filename(sound_format, session_id, message_id, stream_id, spotter):
    if "wav" in sound_format or "pcm" in sound_format:
        ext = "wav"
    elif "opus" in sound_format:
        ext = "opus"
    elif "spx" in sound_format or "speex" in sound_format:
        ext = "spx"
    else:
        ext = "ogg"

    return "%s_%s_%s%s.%s" % (
        session_id,
        message_id,
        stream_id,
        "_spotter" if spotter else '',
        ext
    )


@gen.coroutine
def _log_dump(queue):
    logger = Logger.get('.')
    while True:
        try:
            item = yield queue.get()
        except Exception:
            logger.warning("Timeout on event waiting for session log")
            return
        try:
            item.save()
            if isinstance(item, StopEvent):
                return
        except Exception as exc:
            logger.error("Exception on log processing: %s" % (exc,))
        finally:
            queue.task_done()


def _hide_half(value):
    value_part_size = len(value) // 2
    return value[:value_part_size] + "*" * (len(value) - value_part_size)


def _hide_authorization(headers):
    if headers:
        for k in headers:
            if k.lower() == 'authorization':
                val = headers[k]
                if isinstance(val, str):
                    headers[k] = _hide_half(val)


class LogEvent(object):
    def __init__(self, session_id, rt_log, ipaddr='', uuid='', app_type='', app_id='', uid='', do_not_use_user_logs=False, write_log_to_uniproxy2_cb=None):
        self.write_log_to_uniproxy2 = write_log_to_uniproxy2_cb
        self.timestamp = datetime.now()
        self.session_id = session_id
        self.rt_log = rt_log
        self.ipaddr = ipaddr
        self.uuid = uuid
        self.app_type = app_type
        self.uid = uid
        self.do_not_use_user_logs = do_not_use_user_logs
        self.app_id = app_id

    def _save(self, action, data={}, message_id=None):
        def obscure_http_authorization(d):
            _hide_authorization(dict_at_path(d, ('Event', 'event', 'payload', 'music_request', 'headers')))
            _hide_authorization(dict_at_path(d, ('Event', 'event', 'payload', 'music_request2', 'headers')))

        def obscure_mssngr_texts(d):
            if 'Messenger' == value_by_path(d, ('Directive', 'directive', 'header', 'namespace')) or \
               'Messenger' == value_by_path(d, ('Event', 'event', 'header', 'namespace')):
                replace_tag_value(d, 'MessageText', '***')

        def obscure_userinfo(d):
            if 'Messenger' == value_by_path(d, ('Directive', 'directive', 'header', 'namespace')) and \
               'WhoamiResponse' == value_by_path(d, ('Directive', 'directive', 'header', 'name')):
                replace_tag_value(d, 'UserInfo', '***')

        obscure_oauth_token_and_cookies(data)
        obscure_http_authorization(data)
        obscure_mssngr_texts(data)
        obscure_userinfo(data)
        session = None

        if is_qloud:
            if value_by_path(data, ('Event', 'event', 'header', 'name')) in do_not_log_events:
                return
            session = {
                "Timestamp":        self.timestamp.isoformat(),
                "Action":           action,
                "SessionId":        self.session_id,
                "IpAddr":           self.ipaddr,
                "Uuid":             self.uuid,
            }
        else:
            session = {
                "Timestamp":        self.timestamp.isoformat(),
                "Action":           action,
                "SessionId":        self.session_id,
                "IpAddr":           self.ipaddr,
                "Uuid":             self.uuid,
                "AppType":          self.app_type,
                "Uid":              self.uid,
                "DoNotUseUserLogs": self.do_not_use_user_logs,
                "AppId":            self.app_id
            }

        message = {}
        message.update(data)
        message.update({"Session": session})
        if self.write_log_to_uniproxy2:
            self.write_log_to_uniproxy2(message, message_id)
        Logger.session(AsJson(json.dumps(message, ensure_ascii=False, sort_keys=True)), rt_log=self.rt_log)


class InitEvent(LogEvent):
    def __init__(self, session_id, rt_log, ipaddr='', **kwargs):
        super(InitEvent, self).__init__(session_id, rt_log, ipaddr=ipaddr, **kwargs)

    def save(self):
        self._save("init", {})


class StopEvent(LogEvent):
    def __init__(self, session_id, rt_log, **kwargs):
        super(StopEvent, self).__init__(session_id, rt_log, **kwargs)

    def save(self):
        self._save("close", {})
        if self.rt_log:
            self.rt_log.end_request()
            self.rt_log = None


class ServerEvent(LogEvent):
    # ATTENTION: no deepcopy here!!!
    def __init__(self, session_id, rt_log, directive, ipaddr='', uuid='', prepare=None, app_type='', **kwargs):
        self.directive = directive
        self.prepare = prepare
        super(ServerEvent, self).__init__(session_id, rt_log, ipaddr=ipaddr, uuid=uuid, app_type=app_type, **kwargs)

    def save(self):
        if self.prepare is not None:
            self.prepare(self.directive)
        self._save("response", {"Directive": self.directive})


class ExperimentEvent(LogEvent):
    def __init__(self, session_id, rt_log, descr, **kwargs):
        self.descr = deepcopy(descr)
        super(ExperimentEvent, self).__init__(session_id, rt_log, **kwargs)

    def save(self):
        self._save("experiment", {"Description": self.descr})


class ExpBoxesEvent(LogEvent):
    def __init__(self, session_id, rt_log, exp_boxes, **kwargs):
        self.exp_boxes = exp_boxes
        super(ExpBoxesEvent, self).__init__(session_id, rt_log, **kwargs)

    def save(self):
        self._save("exp_boxes", {"Value": self.exp_boxes})


class ClientEvent(LogEvent):
    # ATTENTION: no deepcopy here!!!
    def __init__(self, session_id, rt_log, msg, ipaddr='', uuid='', app_type='', **kwargs):
        self.message = msg
        super(ClientEvent, self).__init__(session_id, rt_log, ipaddr=ipaddr, uuid=uuid, app_type=app_type, **kwargs)

    def save(self):
        self._save("request", {"Event": self.message})


class StreamEvent(LogEvent):
    def __init__(self, session_id, rt_log, message_id, stream_id, format_, uuid='', save_to_mds=True, app_type='', vins_message_id=None, **kwargs):
        self.message_id = message_id
        self.stream_id = stream_id
        self.stream = BytesIO()
        self.stream_len = 0
        self.spotter = False
        self.format = format_.lower()
        self.vins_message_id = vins_message_id
        self.save_to_mds = save_to_mds
        self.yabio_storage = None
        if "pcm" in self.format:
            # just placeholder, will be overwritten by WAV header when the stream completed
            self.stream.write(b"0" * 44)
            self.stream_len = 44
        super().__init__(session_id, rt_log, uuid=uuid, app_type=app_type, **kwargs)

    def add_data(self, data):
        self.stream.write(data)
        self.stream_len += len(data)

    def close(self, spotter=False):
        self.stream.seek(0)
        self.spotter = spotter

    @gen.coroutine
    def save(self):
        data = {
            "messageId": self.message_id,
            "streamId": self.stream_id,
            "MDS": None,
            "isSpotter": self.spotter
        }
        if self.save_to_mds:
            mds_key = yield self._save_to_mds()
            data["MDS"] = mds_key
            data["format"] = self.format
            data["vins_message_id"] = self.vins_message_id
        self._save("stream", {"Stream": data}, self.message_id)

    @gen.coroutine
    def _save_to_mds(self):
        if not use_mds:
            return None

        if "pcm" in self.format:
            channels = channels_from_mime(self.format)
            if "48" in self.format:
                header = generate_wav_header(48000, channels, self.stream_len)
            elif "8" in self.format:
                header = generate_wav_header(8000, channels, self.stream_len)
            else:
                header = generate_wav_header(16000, channels, self.stream_len)
            self.stream.write(header)
            self.stream.seek(0)
            self.format = "audio/x-wav"

        mds_key = None
        if self.stream_len > 44:
            try:
                filename = get_mds_filename(
                    self.format,
                    self.session_id,
                    self.message_id,
                    self.stream_id,
                    self.spotter
                )
                mds_key = yield MdsStorage().save(filename, self.stream,
                                                  rt_log=self.rt_log, rt_log_label='save_client_stream_to_mds')

                # try to save biometry params. Fork coroutine
                if self.yabio_storage:
                    self.save_new_mds_url_coro(mds_key)

            except Exception as exc:
                logger = Logger.get('.streamevent')
                logger.error("Can't save stream with streamId=%s for messageId=%s, sessionId=%s: %s" % (
                    self.stream_id,
                    self.message_id,
                    self.session_id,
                    exc
                ), rt_log=self.rt_log)

        return mds_key

    @gen.coroutine
    def save_new_mds_url_coro(self, mds_url):
        try:
            source = 'spotter' if self.spotter else 'request'
            yield self.yabio_storage.update_mds_url(self.message_id, source, mds_url)
        except Exception as exc:
            Logger.get().exception('fail on save fresh enrollings: ' + str(exc))


class SessionLogger(object):
    def __init__(self, session_id, rt_log, ipaddr='', system=None):
        self.streams = {}
        self.session_id = session_id
        self.rt_log = rt_log
        self.ipaddr = ipaddr

        self.queue = Queue()

        self.queue.put_nowait(InitEvent(self.session_id, self.rt_log, ipaddr=self.ipaddr))
        IOLoop.current().add_callback(_log_dump, self.queue)

        self._system = weakref.proxy(system) if system else None

        self.logging_enabled = True
        self.logging_utterance_enabled = True

        self.uuid = ""
        self.app_type = ''
        self.uid = ''
        self.do_not_use_user_logs = False
        self.app_id = ''

        self._last_partial = ""  # filthy hack to disable partials overhead
        self._bio_group_ids = {}
        self._message_filter = None

    def set_bio_group_id(self, stream_id, value):
        self._bio_group_ids[stream_id] = value

    def disable(self):
        self.logging_enabled = False

    def disable_utterance(self):
        self.logging_utterance_enabled = False

    def close(self):
        self.queue.put_nowait(StopEvent(
            self.session_id,
            self.rt_log,
            app_id=self.app_id,
            uid=self.uid,
            do_not_use_user_logs=self.do_not_use_user_logs
        ))
        self.queue.join()

    def log_experiment(self, descr):
        if self.logging_enabled:
            self.queue.put_nowait(ExperimentEvent(
                self.session_id,
                self.rt_log,
                descr,
                app_id=self.app_id,
                uid=self.uid,
                do_not_use_user_logs=self.do_not_use_user_logs
            ))

    def log_exp_boxes(self, exp_boxes):
        if self.logging_enabled and exp_boxes:
            self.queue.put_nowait(ExpBoxesEvent(
                self.session_id,
                self.rt_log,
                exp_boxes,
                app_id=self.app_id,
                uid=self.uid,
                do_not_use_user_logs=self.do_not_use_user_logs
            ))

    def is_droppable(self, directive):
        """filthy hack to disable partials overhead"""
        header = directive.get("directive", {}).get("header", {})
        name = header.get("namespace", "") + header.get("name", "")

        if name == 'MessengerHistory':
            return True

        if name == 'MessengerEditHistoryResponse':
            return True

        if name == "ASRResult":
            payload = directive.get("directive", {}).get("payload", {}) or {}
            if not payload.get("endOfUtt", False):
                recognition = payload.get("e2e_recognition", payload.get("recognition", []))

                last_partial = recognition[0].get("normalized", "") if recognition else ""
                if last_partial == self._last_partial:
                    return True
                else:
                    self._last_partial = last_partial
            else:
                self._last_partial = ""
        return False

    def set_message_filter(self, f):
        self._message_filter = f

    def get_message_filter(self):
        return self._message_filter

    def log_directive(self, directive, prepare=None, rt_log=None):
        if not self.logging_enabled or self.is_droppable(directive):
            return

        if not self.logging_utterance_enabled:
            def do_prepare(d):
                recognition = d.get("directive", {}).get("payload", {}).get("recognition", [])
                for result in recognition:
                    result.pop("normalized")
                    result.pop("words")
                if prepare is not None:
                    prepare(d)
        else:
            do_prepare = prepare

        if self._message_filter is not None:
            directive = self._message_filter.filtered_copy(directive)
        else:
            directive = deepcopy(directive)

        self.queue.put_nowait(ServerEvent(
            self.session_id,
            rt_log if rt_log else self.rt_log,
            directive,
            ipaddr=self.ipaddr,
            uuid=self.uuid,
            prepare=do_prepare,
            app_type=self.app_type,
            app_id=self.app_id,
            do_not_use_user_logs=self.do_not_use_user_logs
        ))

    def write_log_to_uniproxy2(self, data, message_id):
        if self._system:
            self._system.write_directive(
                Directive(
                    'Uniproxy2',
                    'AnaLogInfo',
                    data,
                    event_id=message_id,
                ),
                log_message=False
            )

    def start_stream(self, message_id, stream_id, format_="pcm16", save_to_mds=True):
        if self.logging_enabled:
            self.streams[stream_id] = StreamEvent(
                self.session_id,
                self.rt_log,
                message_id,
                stream_id,
                format_,
                uuid=self.uuid,
                save_to_mds=save_to_mds,
                app_type=self.app_type,
                app_id=self.app_id,
                vins_message_id=self._system.log_spotter_vins_message_id if self._system else None,
                uid=self.uid,
                do_not_use_user_logs=self.do_not_use_user_logs,
                write_log_to_uniproxy2_cb=self.write_log_to_uniproxy2,
            )

    def log_data(self, stream_id, data):
        if self.logging_enabled and (stream_id % 2 == 1):  # store only incoming streams data
            stream = self.streams.get(stream_id)
            if stream is not None:
                stream.add_data(data)

    def close_stream(self, stream_id, spotter=False):
        if self.logging_enabled:
            stream = self.streams.get(stream_id)
            if stream is not None:
                bio_group_id = self._bio_group_ids.get(stream_id)
                try:
                    if bio_group_id and self._system:
                        stream.yabio_storage = self._system.get_yabio_storage(bio_group_id)
                except ReferenceError:
                    pass
                stream.close(spotter)
                self.queue.put_nowait(self.streams.pop(stream_id))

    def flush_stream(self, stream_id):
        if self.logging_enabled and stream_id in self.streams:
            message_id = self.streams[stream_id].message_id
            format_ = self.streams[stream_id].format
            save_to_mds = self.streams[stream_id].save_to_mds
            self.close_stream(stream_id, spotter=True)
            self.start_stream(message_id, stream_id, format_, save_to_mds=save_to_mds)

    def log_rawmessage(self, message):
        if self.logging_enabled:
            self.queue.put_nowait(ClientEvent(
                self.session_id,
                self.rt_log,
                deepcopy(message),
                ipaddr=self.ipaddr,
                uuid=self.uuid,
                app_type=self.app_type,
                app_id=self.app_id,
                uid=self.uid,
                do_not_use_user_logs=self.do_not_use_user_logs
            ))

    def log_event(self, event):
        if not self.logging_enabled:
            return

        msg = event.create_message()
        if self._message_filter is not None:
            msg = self._message_filter.filtered_copy(msg)
        else:
            msg = deepcopy(msg)

        self.queue.put_nowait(ClientEvent(
            self.session_id,
            self.rt_log,
            msg,
            ipaddr=self.ipaddr,
            uuid=self.uuid,
            app_type=self.app_type,
            app_id=self.app_id,
            uid=self.uid,
            do_not_use_user_logs=self.do_not_use_user_logs
        ))

    def set_uuid(self, uuid):
        self.uuid = uuid
