import datetime
import json
import time
import weakref
from enum import IntEnum, unique
from urllib import parse

from tornado import gen
from tornado.concurrent import Future

from .fake_stream import FakeStream
from alice.uniproxy.library.backends_common.httpstream import AsyncHttpStream

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.perf_tester import events
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import deepupdate, experiment_value, conducting_experiment

from alice.uniproxy.library.async_http_client.http_request import HTTPResponse, HTTPError

from alice.uniproxy.library.global_counter.uniproxy import (
    VINS_REQUEST_HGRAM,
    VINS_REQUEST_SIZE_QUASAR_HGRAM,
    VINS_RESPONSE_SIZE_QUASAR_HGRAM,
    VINS_REQUEST_SIZE_OTHER_HGRAM,
    VINS_RESPONSE_SIZE_OTHER_HGRAM,
    VINS_APPLY_REQUEST_DURATION_HGRAM,
    # users only metrics (not include robots)
    USEFUL_VINS_APPLY_PREPEARE_REQUEST_DURATION,
    USEFUL_VINS_APPLY_REQUEST_DURATION_HGRAM,
)
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings

from alice.uniproxy.library.utils.futures import with_result_on_timeout
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.extlog.log_filter import VinsSensitiveDataFilter
from alice.uniproxy.library.global_state import GlobalTags


VINS_PRODUCTION_HOSTS = (
    'vins-int.voicetech.yandex.net',
    'vins.alice.yandex.net',
)


def _hide_sensitive_data(vins_response):
    return vins_response.get("contains_sensitive_data", False) and not vins_response.get("mm_black_sheep_wall", False)


def _apply_srcrwr(vins_url):
    def wrapper(self, *args, **kwargs):
        url = vins_url(self, *args, **kwargs)

        rwr_host = self._system.srcrwr['VINS_HOST']
        if rwr_host is None:
            return url

        parsed = parse.urlparse(url)
        if parsed.netloc in VINS_PRODUCTION_HOSTS:
            parsed = parsed._replace(netloc=rwr_host)
            return parse.urlunparse(parsed)
        return url

    return wrapper


class VinsRequestTimings:
    def __init__(self, start_ts=None):
        self.start_ts = start_ts
        if self.start_ts is None:
            self.start_ts = time.monotonic()
        self.prepare_request = {}  # indexed by VinsRequest.WaitRequestsForPartSensor timings
        self.begin_request_ts = None  # end prepare request
        self.end_request_ts = None

    def lag(self):
        return time.monotonic() - self.start_ts

    def request_lag(self):
        return time.monotonic() - self.begin_request_ts

    def set_personal_data_ts(self, ts):
        if ts is None:
            return

        lag = ts - self.start_ts
        if lag < 0:
            lag = 0
        self.prepare_request[events.EventUsefulVinsPrepareRequestPersonalData.NAME] = lag


class VinsRequest(object):
    @unique
    class RequestParts(IntEnum):
        ASR = 1
        MUSIC = 2
        YABIO = 4,
        SESSION = 8,
        CLASSIFY = 16
        NOTIFICATION_STATE = 32
        MEMENTO = 64
        SMART_HOME = 128
        CONTACTS = 256
        LAAS = 512
        CONTACTS_PROTO = 1024

    WaitRequestsForPartSensor = {
        RequestParts.ASR: events.EventUsefulVinsPrepareRequestAsr.NAME,
        RequestParts.MUSIC: events.EventUsefulVinsPrepareRequestMusic.NAME,
        RequestParts.YABIO: events.EventUsefulVinsPrepareRequestYabio.NAME,
        RequestParts.SESSION: events.EventUsefulVinsPrepareRequestSession.NAME,
        RequestParts.CLASSIFY: events.EventUsefulVinsPrepareRequestClassify.NAME,
        RequestParts.NOTIFICATION_STATE: events.EventUsefulVinsPrepareRequestNotificationState.NAME,
        RequestParts.MEMENTO: events.EventUsefulVinsPrepareRequestMemento.NAME,
        RequestParts.CONTACTS: events.EventUsefulVinsPrepareRequestContacts.NAME,
        RequestParts.LAAS: events.EventUsefulVinsPrepareRequestLaas.NAME,
        RequestParts.CONTACTS_PROTO: events.EventUsefulVinsPrepareRequestContacts.NAME,
    }

    WAIT_BIOMETRY_TIMELIMIT1 = 1.0
    WAIT_BIOMETRY_TIMELIMIT2 = 0.3  # timelimit for case when already has same spare/fallback data
    WAIT_NOTIFIER_TIMELIMIT = 1.0
    WAIT_CONTACTS_TIMELIMIT = 2.0
    WAIT_LAAS_TIMELIMIT = 0.05

    @unique
    class RequestType(IntEnum):
        Run = 0,
        Apply = 1

    def __init__(self, system, message_id, payload, on_result, on_error, on_cancel, rt_log, fake=False,
                 vins_timings=None, request_fallback_parts=None, start_ts=None, get_user_ticket=None):
        # system is already a weakref.proxy, no need for weak reference
        self._log = Logger.get('.vins.request')
        self._system = system
        self._message_id = message_id
        self._payload = payload
        self._on_result = weakref.WeakMethod(on_result)
        self._on_error = weakref.WeakMethod(on_error)
        self._on_cancel = weakref.WeakMethod(on_cancel)
        self._unistat = system.unistat
        self._request = {
            "header": self._payload.get("header", {}),
            "application": self._make_application(),
            "request": self._payload.get("request"),
            "iot_user_info_data": self._payload.get("iot_user_info_data"),
        }
        self._request_fallback_parts = request_fallback_parts
        self.rt_log = rt_log
        self._vins_timings = vins_timings  # all vins requests timings
        self._http_stream = None
        self._futures = {}
        self._access_logger = AccessLogger("vins", self._system, rt_log=self.rt_log)
        self.DLOG("Created VinsRequest")
        self._request_text = ""
        self._timings = VinsRequestTimings(start_ts)  # timings only for this request
        self.rtlog_token = None
        self.eou = True
        self.asr_result = None
        self.fake = fake
        self.asr_result_same_as_eou = False
        self.partial_numbers = {}
        self.asr_core_debug = None
        self.get_user_ticket = get_user_ticket
        self.user_ticket = None

        self.request_start_time_ts = None
        self.request_end_time_ts = None

        self.type = VinsRequest.RequestType.Run
        self.need_score_processed_chunks = None
        self.need_classify_processed_chunks = None
        self.vins_request = None  # << ApplyRequest keep here ref to Run request for re-use biometry parts
        self.scoring_request_part = None
        self.classify_request_part = None

        self.is_trash_partial = False

        self.balancing_hint_header = None
        balancer_mode = self._payload.get("settings_from_manager", {}).get("balancing_mode_megamind", None)
        if balancer_mode:
            self.balancing_hint_header = GlobalTags.get_balancing_hint_header_val(balancer_mode)

    def __on_vins_cancel__(self, *args, **kwargs):
        if self._on_cancel and self._on_cancel():
            callback = self._on_cancel()
            callback(*args, **kwargs)

    def start_request(self, request_parts=0):
        self._timings.start_ts = time.monotonic()
        request_parts |= VinsRequest.RequestParts.SESSION
        request_parts |= VinsRequest.RequestParts.LAAS
        if self._system.is_notification_supported():
            request_parts |= VinsRequest.RequestParts.NOTIFICATION_STATE
        if conducting_experiment('use_memento', self._payload):
            request_parts |= VinsRequest.RequestParts.MEMENTO
        if conducting_experiment('use_contacts', self._payload):
            request_parts |= VinsRequest.RequestParts.CONTACTS
        if conducting_experiment('contacts_as_proto', self._payload):
            request_parts |= VinsRequest.RequestParts.CONTACTS_PROTO

        self.DLOG("Started VinsRequest")

        for part in VinsRequest.RequestParts:
            if part & request_parts:
                if self.type == VinsRequest.RequestType.Apply:
                    if part & VinsRequest.RequestParts.YABIO:
                        self.scoring_request_part = self.vins_request.scoring_request_part
                        continue
                    elif part & VinsRequest.RequestParts.CLASSIFY:
                        self.classify_request_part = self.vins_request.classify_request_part
                        continue
                self._futures[part] = Future()
        # start coroutine
        self.start_request_coro()

    @gen.coroutine
    def start_request_coro(self):
        settings_from_manager = self._payload.get("settings_from_manager", {})
        wait_laas_timelimit = float(settings_from_manager.get("vinsreq_wait_laas_timelimit", self.WAIT_LAAS_TIMELIMIT))

        index_to_part = []
        f_list = []
        for part, fut in self._futures.items():
            if part & (VinsRequest.RequestParts.YABIO | VinsRequest.RequestParts.CLASSIFY):
                # not trust future from yabio, so set timeout for it, but use speedup hacks for limit waiting duration
                timelimit = self.WAIT_BIOMETRY_TIMELIMIT1
                fallback_data = {}
                data_wrapper = self.wrap_scoring_result if part & VinsRequest.RequestParts.YABIO else self.wrap_classify_result
                if self._request_fallback_parts:
                    fallback_data = self._request_fallback_parts.get(part, {})
                    if fallback_data:
                        fallback_data = data_wrapper(fallback_data)
                        timelimit = self.WAIT_BIOMETRY_TIMELIMIT2
                f_list.append(with_result_on_timeout(datetime.timedelta(seconds=timelimit), fut, fallback_data))
            elif part == VinsRequest.RequestParts.NOTIFICATION_STATE:
                f_list.append(with_result_on_timeout(datetime.timedelta(seconds=self.WAIT_NOTIFIER_TIMELIMIT), fut, {}))
            elif part == VinsRequest.RequestParts.CONTACTS or part == VinsRequest.RequestParts.CONTACTS_PROTO:
                f_list.append(with_result_on_timeout(datetime.timedelta(seconds=self.WAIT_CONTACTS_TIMELIMIT), fut, {}))
            elif part == VinsRequest.RequestParts.LAAS:
                f_list.append(with_result_on_timeout(datetime.timedelta(seconds=wait_laas_timelimit), fut, {}))
            else:
                f_list.append(fut)
            index_to_part.append(part)

        if self._request_fallback_parts:
            has_yabio_scoring = VinsRequest.RequestParts.YABIO in self._futures
            yabio_score_fallback_data = self._request_fallback_parts.get(VinsRequest.RequestParts.YABIO)
            if not has_yabio_scoring and yabio_score_fallback_data:
                # has race when got response from yabio & closed yabio backend before create VinsRequest (and so not has bio future)
                self._request = deepupdate(self._request, self.wrap_scoring_result(yabio_score_fallback_data))

        if self.type == VinsRequest.RequestType.Apply:
            if self.scoring_request_part:
                self._request = deepupdate(self._request, self.scoring_request_part)
            if self.classify_request_part:
                self._request = deepupdate(self._request, self.classify_request_part)

        wait_iterator = gen.WaitIterator(*f_list)
        while not wait_iterator.done():
            try:
                result = yield wait_iterator.next()
                if result is not None:
                    self._request = deepupdate(self._request, result)
            except Exception as exc:
                part_name = VinsRequest.WaitRequestsForPartSensor.get(index_to_part[wait_iterator.current_index])
                self.rt_log.exception(f"vins.request_wait_completed with fail on {part_name}")
                if self._on_result and self._on_result():
                    self.EXC(exc)
                self.__on_vins_cancel__(reason='Error: request wait failed')
                # on_vins_cancel
                return

            part = index_to_part[wait_iterator.current_index]
            if self.type == VinsRequest.RequestType.Run:
                if part == VinsRequest.RequestParts.YABIO:
                    self.scoring_request_part = result
                elif part == VinsRequest.RequestParts.CLASSIFY:
                    self.classify_request_part = result
            name_prepared_req = VinsRequest.WaitRequestsForPartSensor.get(part)
            self.rt_log('{} completed'.format(name_prepared_req))
            if name_prepared_req is not None:
                self._timings.prepare_request[name_prepared_req] = self._timings.lag()

        self.rt_log('vins.request_wait_completed')
        if self.type == VinsRequest.RequestType.Apply:
            self._unistat.store(USEFUL_VINS_APPLY_PREPEARE_REQUEST_DURATION, time.monotonic() - self._timings.start_ts)
        if self._vins_timings and self.type == VinsRequest.RequestType.Run:
            self._vins_timings.on_request_prepared(self._timings.start_ts, time.monotonic())

        try:
            self.user_ticket = yield self.get_user_ticket()
        except:
            pass

        try:
            if self._system.uaas_test_ids:
                self._request["request"].setdefault("test_ids", []).extend(self._system.uaas_test_ids)
            self.DLOG("Really started VinsRequest")
            self._start_stream()
        except ReferenceError:
            # on_vins_cancel
            self.__on_vins_cancel__(
                reason='Error: weakly-referenced object no longer exists (probably self._system died)')
            self.WARN("weakly-referenced object no longer exists: probably self._system died")

    @property
    def timings(self):
        return self._timings

    def set_music_result(self, result=None):
        music_future = self._futures.get(VinsRequest.RequestParts.MUSIC)
        if music_future is None or music_future.done():
            return

        music_future.set_result({
            "request": {
                "event": {
                    "music_result": result
                }
            }
        })

    def set_asr_result(self, result, asr_partial_number, end_of_utterance, is_whisper, request_text, core_debug):
        self.eou = end_of_utterance
        self._request_text = request_text
        if core_debug:
            self.asr_core_debug = core_debug
        self.asr_result = result
        self.partial_numbers['asr'] = asr_partial_number

        if VinsRequest.RequestParts.ASR not in self._futures:
            if result is not None:
                self.ERR("Attempt to set not empty ASR result for text-only input: result='{result}'")
            return

        res = {
            "request": {
                "event": {
                    "asr_result": result,
                    "end_of_utterance": end_of_utterance,
                    "asr_whisper": is_whisper,
                },
            },
        }

        if self.asr_core_debug:
            res["request"]["event"]["asr_core_debug"] = self.asr_core_debug

        if asr_partial_number is not None:
            res["request"]["event"]["hypothesis_number"] = asr_partial_number
        self._futures[VinsRequest.RequestParts.ASR].set_result(res)

    def wrap_scoring_result(self, result, end_of_utterance=False):
        return {
            "request": {
                "event": {
                    "biometry_scoring": result,
                    "end_of_utterance": end_of_utterance,
                }
            }
        }

    def set_yabio_result(self, result, processed_chunks=None, end_of_utterance=False):
        if not result:
            # got some error, ignore it (error already must be logged)
            result = {"status": "ok", "scores_with_mode": []}

        self.partial_numbers['scoring'] = result.get('partial_number')

        future = self._futures.get(VinsRequest.RequestParts.YABIO)
        if future is None or future.done():
            return

        if processed_chunks is not None and self.need_score_processed_chunks is not None:
            if processed_chunks < self.need_score_processed_chunks:
                return

        future.set_result(self.wrap_scoring_result(result, end_of_utterance))

    def wrap_classify_result(self, result):
        return {
            "request": {
                "event": {
                    "biometry_classification": result,
                }
            }
        }

    def set_classify_result(self, result, processed_chunks=None):
        if not result:
            # got some error, ignore it (error must already be logged)
            result = {"status": "ok", "scores": []}

        self.partial_numbers['classification'] = result.get('partial_number')

        future = self._futures.get(VinsRequest.RequestParts.CLASSIFY)
        if future is None or future.done():
            return

        if processed_chunks is not None and self.need_classify_processed_chunks is not None:
            if processed_chunks < self.need_classify_processed_chunks:
                return

        future.set_result(self.wrap_classify_result(result))

    def set_vins_session_result(self, result):
        if not self._futures[VinsRequest.RequestParts.SESSION].done():
            self._futures[VinsRequest.RequestParts.SESSION].set_result({
                "session": result
            })

    def set_notification_state_result(self, result):
        fut = self._futures.get(VinsRequest.RequestParts.NOTIFICATION_STATE)
        if not fut or fut.done():
            return
        fut.set_result({
            'request': {
                'notification_state': result,
            }
        })

    def set_memento_state_result(self, result):
        fut = self._futures.get(VinsRequest.RequestParts.MEMENTO)
        if not fut or fut.done():
            return
        fut.set_result({
            'memento': result
        })

    def set_contacts_state_result(self, result):
        fut = self._futures.get(VinsRequest.RequestParts.CONTACTS)
        if not fut or fut.done():
            return
        if result is None:
            fut.set_result({})
            return
        fut.set_result({
            'contacts': result
        })

    def set_contacts_proto_state_result(self, result):
        fut = self._futures.get(VinsRequest.RequestParts.CONTACTS_PROTO)
        if not fut or fut.done():
            return
        if result is None:
            fut.set_result({})
            return
        fut.set_result({
            'contacts_proto': result
        })

    def set_laas_result(self, result=None):
        laas_future = self._futures.get(VinsRequest.RequestParts.LAAS)
        if (laas_future is None) or laas_future.done():
            return

        if not result:
            laas_future.set_result({})
        else:
            GlobalCounter.SET_LAAS_FROM_CL_TO_VINS_SUMM.increment()
            laas_future.set_result({
                "request": {
                    "laas_region": result
                }
            })

    def set_smart_home_result(self, result):
        fut = self._futures.get(VinsRequest.RequestParts.SMART_HOME)
        if (not fut) or fut.done():
            return

        if result is None:
            fut.set_result({})
            return

        new_proto_api, smart_home = result[0], result[1]

        if smart_home:
            fut.set_result({
                "request": {
                    "smart_home": smart_home
                },
                "iot_user_info_data": new_proto_api
            })
        else:
            # hotfix for IOT unanswer
            fut.set_result({})

    def close(self):
        if self._http_stream:
            self._http_stream.close()
        self._http_stream = None
        self._on_result = None
        self._on_error = None
        for future in self._futures.values():
            if not future.done:
                future.set_exception(Exception("abort"))

    def _make_application(self):
        application = deepupdate(
            self._payload.get("vins", {}).get("application", {}),
            self._payload.get("application", {})
        )

        return application

    def _parse_qs(self, query):
        args = parse.parse_qs(query)

        if 'srcrwr' in args:
            rwrs = dict(
                (arg.split(':', 1) for arg in args.get('srcrwr', []))
            )
            del args['srcrwr']
        else:
            rwrs = {}

        return args, rwrs

    def _parse_vins_url(self, url, append_path=None):
        parsed = parse.urlparse(url)

        if append_path:
            path = parsed.path
            if path == '/':
                path = append_path
            else:
                path = '%s%s' % (path, append_path)
            parsed = parsed._replace(path=path)

        qs, rwrs = self._parse_qs(parsed.query)

        return parsed, qs, rwrs

    def _unparse_vins_url(self, parsed, qs, rwrs):
        qs['srcrwr'] = ['%s:%s' % (k, v) for k, v in rwrs.items()]
        query = parse.urlencode(qs, doseq=True, safe=':/', quote_via=parse.quote)
        parsed = parsed._replace(query=query)
        return parse.urlunparse(parsed)

    def _build_mapped_url(self, suffix=None):
        app_name = self._make_application().get("app_id", "")
        for pattern, url in config["vins"]["hosts_mapping"]:
            if pattern.match(app_name):
                self.INFO("vins_routing %s to %s" % (app_name, url))
                if suffix:
                    self.INFO("effective suffix: %s" % (suffix))
                    parsed = parse.urlparse(url)
                    parsed = parsed._replace(path=suffix)
                    return parse.urlunparse(parsed)
                else:
                    return url

        default_host = config["vins"]["hosts_mapping"][-1][1]

        self.INFO("vins_routing %s to %s" % (app_name, default_host))

        if suffix is None:
            return default_host

        parsed = parse.urlparse(default_host)
        parsed = parsed._replace(path=suffix)
        return parse.urlunparse(parsed)

    def _inject_graph_override(self, base_url, override_graph):
        if override_graph is not None:
            parsed, qs, rwrs = self._parse_vins_url(base_url)
            qs['graph'] = qs.get('graph', []) + [override_graph]
            return self._unparse_vins_url(parsed, qs, rwrs)
        else:
            return base_url

    @_apply_srcrwr
    def _build_vins_url(self, srcrwr_url, payload_url, uaas_url, suffix=None, override_graph=None):
        # Redirecting all requests with envvar UNIPROXY_VINS_BACKEND (for yappy)
        if not srcrwr_url and config["vins"]["url"]:
            return self._inject_graph_override(config["vins"]["url"], override_graph)

        if not payload_url:
            if not srcrwr_url and not uaas_url:
                return self._inject_graph_override(self._build_mapped_url(suffix), override_graph)

        rwrs = {}
        payload_rwrs = None
        uaas_rwrs = None
        srcrwr_rwrs = None

        if not payload_url:
            parsed, qs, rwrs = self._parse_vins_url(
                "http://vins.alice.yandex.net%s" % (config["vins"].get("default_run_url", "/speechkit/app/pa"))
            )
        else:
            parsed, qs, payload_rwrs = self._parse_vins_url(payload_url)

        if suffix is not None:
            parsed = parsed._replace(path=suffix)

        if uaas_url and (not parsed or (parsed and (parsed.netloc in VINS_PRODUCTION_HOSTS))):
            parsed, qs, uaas_rwrs = self._parse_vins_url(uaas_url, append_path=parsed.path)

        if srcrwr_url:
            parsed, qs, srcrwr_rwrs = self._parse_vins_url(srcrwr_url)
            if suffix is not None:
                parsed = parsed._replace(path=suffix)

        if uaas_rwrs:
            rwrs.update(uaas_rwrs)

        if payload_rwrs:
            rwrs.update(payload_rwrs)

        if srcrwr_rwrs:
            if 'VINS' in srcrwr_rwrs:
                del rwrs['VINS']
            rwrs.update(srcrwr_rwrs)

        if 'VINS_HOST' in rwrs:
            del rwrs['VINS_HOST']

        host_rwr = self._system.srcrwr["VINS_HOST"]
        if host_rwr and parsed.netloc in VINS_PRODUCTION_HOSTS:
            parsed = parsed._replace(netloc=host_rwr)

        if override_graph is not None:
            qs['graph'] = qs.get('graph', []) + [override_graph]

        url = self._unparse_vins_url(parsed, qs, rwrs)

        self.INFO("vins_routing to %s" % url)

        return url

    def _vins_url(self, suffix=None, override_graph=None):
        if override_graph is None:
            override_graph = self._system.graph_overrides.override_run
        return self._build_vins_url(
            self._system.srcrwr["VINS"],
            self._payload.get("vinsUrl"),
            self._payload.get("uaasVinsUrl"),
            suffix,
            override_graph
        )

    @staticmethod
    def safe_get_additional_options(payload):
        request = payload.setdefault("request", {})
        if not request.get("additional_options"):
            request["additional_options"] = {}
        return request["additional_options"]

    def _add_server_time_to_request(self, payload):
        additional_options = VinsRequest.safe_get_additional_options(payload)
        if "server_time_ms" in additional_options and experiment_value("uniproxy_use_server_time_from_client",
                                                                       self._payload):
            additional_options["server_time_ms"] = int(additional_options["server_time_ms"])
        else:
            additional_options["server_time_ms"] = int(1000 * time.time())

    def _add_client_ip_to_request(self, payload):
        # hack for VOICESERV-427 (enrich request with client_ip info)
        additional_options = VinsRequest.safe_get_additional_options(payload)
        additional_options.setdefault("bass_options", {}).setdefault("client_ip", self._system.websocket.client_ip)

    def _add_uaas_experiments_to_request(self, payload):
        uaas_headers = {}
        if self._system.uaas_exp_boxes:
            uaas_headers['expboxes'] = self._system.uaas_exp_boxes
        if uaas_headers:
            additional_options = VinsRequest.safe_get_additional_options(payload)
            additional_options.update(uaas_headers)

    def _store_request_timings(self, response_size=None):
        self.timings.end_request_ts = time.monotonic()
        request_duration = self.timings.request_lag()
        GlobalTimings.store(VINS_REQUEST_HGRAM, request_duration)
        if self.type == VinsRequest.RequestType.Apply:
            self._unistat.store(VINS_APPLY_REQUEST_DURATION_HGRAM, request_duration)
            self._unistat.store(USEFUL_VINS_APPLY_REQUEST_DURATION_HGRAM, request_duration)
        if response_size is not None:
            if self._is_quasar():
                GlobalTimings.store(VINS_RESPONSE_SIZE_QUASAR_HGRAM, response_size)
            else:
                GlobalTimings.store(VINS_RESPONSE_SIZE_OTHER_HGRAM, response_size)

    def _error_callback(self, response):
        self._store_request_timings()
        self.rt_log.log_child_activation_finished(self.rtlog_token, False)
        self.rt_log.error('vins.stream_result_received',
                          response_code=response.code,
                          response_body=response.body)
        if self._on_error is None:
            return

        if self._access_logger:
            self._access_logger.end(code=response.code, size=len(response.body or ""))
            self._access_logger = None

        GlobalCounter.increment_error_code("vins", response.code)
        try:
            self._system.increment_stats("vins", response.code)
        except ReferenceError:
            pass
        response_error = ''  # short response for user (but logging full response)
        if isinstance(response, HTTPResponse):
            if experiment_value('uniproxy_512_and_x_yandex_vins_ok_as_good_response', self._payload):
                if response.code == 512 and response.headers.get("x-yandex-vins-ok", "false") == "true":
                    self._result_callback(response)
                    return
                else:
                    response_error = 'Bad vins response: code={}, response={}'.format(response.code,
                                                                                      response.text())
            else:
                if response.code == 512 or response.headers.get("x-yandex-vins-ok", "false") == "true":
                    self._result_callback(response)
                    return
                else:
                    response_error = 'Bad vins response: code={}, response={}'.format(response.code,
                                                                                      response.text())
        elif isinstance(response, HTTPError):
            response_error = 'Bad vins response: code={}'.format(response.code)
        else:
            response_error = 'Unexpected VINS response type: {}'.format(response.__class__.__name__)
        try:
            self._system.logger.log_directive(
                {
                    "type": "VinsResponse",
                    "ForEvent": self._message_id,
                    "error": response_error + ': ' + str(response) + ': ' + str(response.text()),
                },
                rt_log=self.rt_log,
            )
        except ReferenceError:
            return
        finally:
            if self._on_error and self._on_error():
                self._on_error()(response_error)
        self.close()

    def _result_callback(self, response):
        if self._vins_timings:
            if self.type == VinsRequest.RequestType.Run:
                if self.eou:
                    self._vins_timings.on_event(events.EventFinishVinsRequestEOU)
        response_size = len(response.body or "")
        self._store_request_timings(response_size)
        self.rt_log.log_child_activation_finished(self.rtlog_token, True)
        self.rt_log('vins.stream_result_received',
                    code=response.code,
                    response_size=response_size)

        if response.code < 300:
            GlobalCounter.increment_error_code("vins", response.code)
            try:
                self._system.increment_stats("vins", response.code)
            except ReferenceError:
                pass
        if self._on_result is None or self._on_result() is None:
            return

        if self._access_logger:
            self._access_logger.end(code=response.code, size=len(response.body or ""))
            self._access_logger = None

        try:
            self._system.logger.log_directive(
                {
                    "type": "VinsResponse",
                    "ForEvent": self._message_id,
                    "time_info": response.time_info
                },
                rt_log=self.rt_log,
            )
        except ReferenceError:
            return

        try:
            vins_response = json.loads(response.body.decode("utf-8"))
            if self.partial_numbers is not None:
                if not vins_response.get('header'):
                    vins_response['header'] = {}
                for back, value in self.partial_numbers.items():
                    vins_response['header']['{}_partial_number'.format(back)] = value

            self.is_trash_partial = vins_response.get('voice_response', {}).get('is_trash_partial', False)
        except Exception as exc:
            self._error_callback("vins response can't be decoded")
            self.EXC(exc)
        else:
            if _hide_sensitive_data(vins_response):
                self._system.logger.set_message_filter(VinsSensitiveDataFilter)
            if self._on_result and self._on_result():
                self._on_result()(
                    vins_request=self,
                    vins_response=vins_response,
                    request_text=self._request_text,
                    asr_result=self.asr_result,
                    eou=self.eou,
                    force_eou=(response.code == 205))
        self.close()

    def _start_stream(self):
        self._access_logger.start(self._message_id)

        payload = {
            "type": "VinsRequest",
            "Body": self._request,
            "EffectiveVinsUrl": self._vins_url(),
            "ForEvent": self._message_id
        }

        self._system.logger.log_directive(
            payload,
            lambda d: d['Body'].pop('session', None),
            rt_log=self.rt_log,
        )

        # send this to uniproxy2
        self._system.write_directive(
            Directive(
                'Uniproxy2',
                'AnaLogInfo',
                payload,
                event_id=self._message_id
            ),
            log_message=False
        )

        self._add_server_time_to_request(self._request)
        self._add_client_ip_to_request(self._request)
        self._add_uaas_experiments_to_request(self._request)

        additional_options = VinsRequest.safe_get_additional_options(self._request)
        # set passpord uid for logging and later GDPR search
        if self._system.uid:
            additional_options["yandex_uid"] = self._system.yuid
            additional_options["puid"] = self._system.puid
        additional_options["do_not_use_user_logs"] = self._system.do_not_use_user_logs

        if self._system.icookie_for_uaas:
            additional_options["icookie"] = self._system.icookie_for_uaas

        headers = {
            'Content-type': 'application/json',
            'X-Ya-User-Ticket': self.user_ticket,
        }
        if self.balancing_hint_header:
            headers['X-Yandex-Balancing-Hint'] = self.balancing_hint_header
        req_id = self._request.get("header", {}).get("request_id")
        if req_id:
            headers.update({"x-alice-client-reqid": req_id})

        if self._system:
            if self._system._app_type:
                headers.update({
                    'X-Alice-AppType': self._system._app_type,
                })
            if self._system._app_id:
                headers.update({
                    'X-Alice-AppId': self._system._app_id,
                })

        headers.update(self._system.srcrwr.header)

        # self.DLOG('VINS_REQUEST={}'.format(json.dumps(
        #    self._request, indent=4, ensure_ascii=False, sort_keys=True)))

        if self._request_text:
            _rtlog_label = 'vins:%s' % (self._request_text or self._request['request']['event'].get('text', 'vins'), )
        else:
            _rtlog_label = 'callback:%s' % (self._request['request']['event'].get('name', 'undefined'), )

        self.rtlog_token = self.rt_log.log_child_activation_started(_rtlog_label)

        self._timings.begin_request_ts = time.monotonic()
        if self._vins_timings:
            if self.type == VinsRequest.RequestType.Apply:
                self._vins_timings.on_event(events.EventStartVinsApplyRequest)
            else:
                if self.eou:
                    self._vins_timings.on_event(events.EventStartVinsRequestEOU)

        if self.rtlog_token:
            token = self.rtlog_token
            token_ascii = token.decode('ascii') if isinstance(token, bytes) else token
            headers['X-RTLog-Token'] = token_ascii
            # In order to join alice and aphost logs ALICEINFRA-366
            headers['X-Yandex-Req-Id'] = token_ascii

        headers['X-Yandex-Internal-Request'] = '1'

        request_body = json.dumps(self._request).encode('utf-8')

        if self._is_quasar():
            GlobalTimings.store(VINS_REQUEST_SIZE_QUASAR_HGRAM, len(request_body))
        else:
            GlobalTimings.store(VINS_REQUEST_SIZE_OTHER_HGRAM, len(request_body))

        streamClass = FakeStream if self.fake else AsyncHttpStream

        self._http_stream = streamClass(
            self._vins_url(),
            self._result_callback,
            method='POST',
            on_error=self._error_callback,
            body=request_body,
            headers=headers,
            connect_timeout=config["vins"].get("connect_timeout", 0.1),
            connect_retries=config["vins"].get("connect_retries", 2),
            request_timeout=config["vins"].get("timeout", 5.0),
        )

    def _is_quasar(self):
        return self._system.is_quasar

    def session_id_noexcept(self):
        try:
            return self._system.session_id
        except ReferenceError:  # self._system is weak-ref
            return '???'

    def DLOG(self, *args):
        self._log.debug(self.session_id_noexcept(), *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self.session_id_noexcept(), *args, rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self.session_id_noexcept(), *args, rt_log=self.rt_log)

    def ERR(self, *args):
        self._log.error(self.session_id_noexcept(), *args, rt_log=self.rt_log)

    def EXC(self, exception):
        self._log.exception(exception, rt_log=self.rt_log)


class VinsApplyRequest(VinsRequest):
    def __init__(self, vins_request, *args):
        super().__init__(*args)
        self.type = VinsRequest.RequestType.Apply
        self.vins_request = vins_request

    def try_set_core_debug(self, core_debug):
        if core_debug and not self.asr_core_debug:
            self.asr_core_debug = core_debug

    def _vins_url(self, override_graph=None):
        return super()._vins_url(suffix="/speechkit/apply", override_graph=self._system.graph_overrides.override_apply)
