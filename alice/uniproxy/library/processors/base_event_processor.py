from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.uaas import FlagsJsonClient
from alice.uniproxy.library.utils import deepupdate
from alice.uniproxy.library.events import EventException, EventExceptionEx
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils.experiments import conducting_experiment

from yweb.webdaemons.icookiedaemon.icookie_lib.utils_py import GetDecryptedIcookie, GenerateIcookieFromUuid
from rtlog import null_logger
from enum import Enum, unique

import time
import base64


class EventProcessor:
    @unique
    class StreamControlAction(Enum):
        Close = 0
        Ignore = 1
        Flush = 2
        NextChunk = 3

    def __init__(self, system, rt_log, init_message_id):
        self._log = Logger.get('.event.processor')
        self.system = system
        self.session_id = self.system.session_id
        self.event = None
        self.payload_with_session_data = {}
        self.init_message_id = "" if (init_message_id is None) else init_message_id.lower()
        self._closed = False
        self.create_ts = time.time()
        self.rt_log = rt_log or null_logger()
        self.flags_json_client = None
        self.flags_json_client_class = FlagsJsonClient
        self.flags_json_url = None
        self.flags_json_rtlog_token = None
        self._processed_external_experiments = False
        GlobalCounter.UNIPRX_EV_CREATED_SUMM.increment()
        GlobalCounter.UNIPRX_EV_RUNNING_AMMX.increment()

    @property
    def proc_id(self):
        return f"{self.event_type}-{self.init_message_id}"

    # ----------------------------------------------------------------------------------------------------------------
    def add_data(self, data):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        self.event = event
        self.payload_with_session_data = deepupdate(self.system.session_data, self.event.payload)

    # ----------------------------------------------------------------------------------------------------------------
    def process_extra(self, event):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def process_streamcontrol(self, _) -> StreamControlAction:
        self.dispatch_error("Do not want!")
        return self.StreamControlAction.Close

    # ----------------------------------------------------------------------------------------------------------------
    def dispatch_error(self, err, close=True):
        e = EventException(err, self.event.message_id if self.event else None)
        self._do_dispatch_error(e, close)

    # ----------------------------------------------------------------------------------------------------------------
    def dispatch_error_ex(self, message, details=None, close=True, response={}):
        e = EventExceptionEx(message, details, self.event.message_id if self.event else None, response=response)
        self._do_dispatch_error(e, close)

    # ----------------------------------------------------------------------------------------------------------------
    def _do_dispatch_error(self, event_exception, close):
        self.system.write_directive(event_exception)
        GlobalCounter.EVENT_EXCEPTIONS_SUMM.increment()
        if self.rt_log:
            self.rt_log.error(
                'exception_sent_to_client',
                exc_info=event_exception,
                message=event_exception.message
            )
        if event_exception.initial_exception:
            if self.rt_log:
                self.rt_log.exception_info(event_exception.initial_exception, 'initial_exception')
        if close:
            self.close()

    # ----------------------------------------------------------------------------------------------------------------
    def close(self):
        if self._closed:
            return
        self._closed = True
        try:
            self.system.on_close_event_processor(self)
        except ReferenceError:
            pass
        try:
            if self.rt_log:
                self.rt_log.end_request()
        except Exception:
            self.EXC('EventProcessor.close()')
        GlobalCounter.UNIPRX_EV_CLOSED_SUMM.increment()
        GlobalCounter.UNIPRX_EV_RUNNING_AMMX.decrement()

    # ----------------------------------------------------------------------------------------------------------------
    def _try_use_experiment(self, puid=None):
        # Note: this remove 'cookie' 'icookie' 'user_agent' from payload
        if not self._try_use_flags_json(self.event, puid=puid, call_source="sync_state"):
            self._try_process_external_experiments()

    # ----------------------------------------------------------------------------------------------------------------
    def _process_external_experiments(self):
        if conducting_experiment('disable_balancing_hint', self.payload_with_session_data):
            self.system.use_balancing_hint = False
        if conducting_experiment('enable_spotter_glue', self.payload_with_session_data):
            self.system.use_spotter_glue = True

    def _try_process_external_experiments(self, *args, **kwargs):
        if not self._processed_external_experiments:
            self._processed_external_experiments = True
            self._process_external_experiments(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    def _try_use_vins_url(self, payload, flag, force=False):
        if not force and payload.get('uaasVinsUrl'):
            return
        if len(flag) <= 12:
            return
        try:
            vins_url_encoded = flag[12:]
            if len(vins_url_encoded) > 0:
                uaas_vins_url = base64.b64decode(vins_url_encoded.encode('utf-8'))
                uaas_vins_url = uaas_vins_url.decode('utf-8')
                if uaas_vins_url:
                    payload['uaasVinsUrl'] = uaas_vins_url.strip()
            else:
                self.WARN('UaasVinsUrl has empty url: {}'.format(flag))
        except Exception:
            self.WARN('UaasVinsUrl failed to parse: {}'.format(flag))

    # ----------------------------------------------------------------------------------------------------------------
    def _on_flags_json_response_wrap(self, *args, **kwargs):
        self.on_flags_json_response(*args, **kwargs)
        self._try_process_external_experiments()

    def _try_use_flags_json(self, event, puid=None, call_source=None):
        # VOICESERV-3972
        no_tests = False
        if conducting_experiment("only_100_percent_flags", self.payload_with_session_data):
            no_tests = True
        else:
            if conducting_experiment("disregard_uaas", self.payload_with_session_data):
                return False

        cookie = event.payload.get("cookie", None)

        user_agent = event.payload.get("user_agent", None)
        uaas_tests = event.payload.get("uaas_tests", None)
        if uaas_tests is None:
            uaas_tests = []

        uuid = self.system.session_data.get("uuid")
        if uuid is None:
            return False

        if not self.system.icookie_for_uaas:
            try:
                icookie = event.payload.get("icookie", None)
                if icookie is not None:
                    self.system.icookie_for_uaas = GetDecryptedIcookie(icookie).decode("ascii")
                else:
                    self.system.icookie_for_uaas = GenerateIcookieFromUuid(uuid).decode("ascii")
            except:
                self.EXC('Error ocured while dealing with icookie')

        exps_cfg = config.get("experiments", {})
        if not exps_cfg:
            return False

        flags_json_cfg = exps_cfg.get("flags_json", {})
        if not flags_json_cfg:
            return False

        if not flags_json_cfg.get("enabled", False):
            return False

        self.flags_json_url = self.system.srcrwr["FLAGS_JSON"] or flags_json_cfg.get("url")
        if not self.flags_json_url:
            return False

        def flags_json_request_logger(data):
            self.system.logger.log_directive({
                "type": "FlagsJsonRequest",
                "ForEvent": self.event.message_id,
                "data": data
            })

        def _on_error(err):
            try:
                flags_json_request_logger("FlagsJsonError: {}".format(err))
                # unanswer from FlagsJson is not fatal error, simple log it and continue
                self.WARN(f"request FlagsJson url={self.flags_json_url} cause error: {err}")
            except ReferenceError:
                pass

            self._try_process_external_experiments()

        try:
            self.DLOG('run request to FlagsJson url={}'.format(self.flags_json_url))
            self.flags_json_client = self.flags_json_client_class(
                self.flags_json_url,
                on_result=self._on_flags_json_response_wrap,
                on_error=_on_error,
                ip=self.system.client_ip,
                uuid=uuid,
                cookie=cookie,
                icookie=self.system.icookie_for_uaas,
                user_agent=user_agent,
                tests_ids=uaas_tests + self.system.test_ids,
                rt_log=self.rt_log,
                app_info=self.system.x_yandex_appinfo,
                device_id=self.system.device_id,
                connect_timeout=flags_json_cfg.get("connect_timeout", 0.1),
                request_timeout=flags_json_cfg.get("request_timeout", 0.2),
                request_logger=flags_json_request_logger,
                puid=puid,
                staff_login=self.system.staff_login,
                no_tests=no_tests,
                call_source=call_source,
            )
        except Exception as exc:
            self.WARN("can not create FlagsJson client: {}".format(exc))
            self.EXC(exc)
            return False

        return True

    # ----------------------------------------------------------------------------------------------------------------
    def is_closed(self):
        return self._closed

    def reply_timedout(self):
        return False  # by default EventProcessor has not final response

    def DLOG(self, *args):
        self._log.debug(self.session_id, *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self.session_id, *args, rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self.session_id, *args, rt_log=self.rt_log)

    def ERR(self, *args):
        self._log.error(self.session_id, *args, rt_log=self.rt_log)

    def EXC(self, exception):
        self._log.exception(exception, rt_log=self.rt_log)
