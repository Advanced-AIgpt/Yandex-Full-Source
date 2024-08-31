# -*- coding: utf-8
import json
import time
import datetime
import traceback
from random import random
import base64

from cityhash import hash64 as CityHash64

from tornado.ioloop import IOLoop
from tornado import gen
from tornado.concurrent import Future

from . import EventProcessor, register_event_processor
from .asr import Recognize
from .tts import UniproxyTTSTimings

from alice.uniproxy.library.backends_ctxs.contacts import get_contacts
from alice.uniproxy.library.backends_ctxs.contexts import ContextFutures
from alice.uniproxy.library.backends_ctxs.smart_home import make_smart_home_response_from_proto
from alice.uniproxy.library.backends_ctxs.smart_home import parse_smart_home_response
from alice.uniproxy.library.backends_ctxs.memento import Memento

from alice.uniproxy.library.backends_asr import YaldiStream, SpotterStream, get_yaldi_stream_type
from alice.uniproxy.library.vins import VinsAdapter, validate_vins_event

from alice.uniproxy.library.vins_context_storage import get_accessor as get_vins_context_accessor

from alice.uniproxy.library.musicstream import MusicStream2
from alice.uniproxy.library.personal_data import PersonalDataHelper
from alice.uniproxy.library.personal_cards import PersonalCardsHelper
from alice.uniproxy.library.backends_bio import YabioStream
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json
from alice.uniproxy.library.backends_common.context_save import AppHostedContextSave
from alice.uniproxy.library.backends_tts.cache import TTSPreloader
from alice.uniproxy.library.backends_tts.cache import UNIPROXY_DISABLE_TTS_PRECACHE_EXP, UNIPROXY_FULL_TTS_PRECACHE_EXP

from alice.uniproxy.library.utils import deepupdate, deepsetdefault, rtlog_child_activation
from alice.uniproxy.library.utils import conducting_experiment, mm_experiment_value
from alice.uniproxy.library.utils.action_counter import ActionCounter
from alice.uniproxy.library.utils.timestamp import TimestampLagStorage
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, GolovanBackend
from alice.uniproxy.library.global_counter.uniproxy import RESPONSE_DELAY_HGRAM

from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.events import EventException
from alice.uniproxy.library.events import Event
from alice.uniproxy.library.events import StreamControl

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.matrix_api import MatrixApi
from alice.uniproxy.library.notificator_api import NotificatorApi

from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.extlog.log_filter import VinsSensitiveDataFilter

from alice.uniproxy.library.perf_tester import events

from alice.uniproxy.library.uaas import FlagsJsonResponse

from alice.megamind.protos.speechkit.request_pb2 import TSpeechKitRequestProto


_Lags = {
    events.EventUsefulResponseForUser.NAME: (
        ("useful_asr_result_evage", "useful_asr_result_to_useful_response_for_user_lag"),
        ("useful_asr_result_evage", "useful_asr_result_to_useful_response_for_robot_lag"),
        ("asr_end_evage", "asr_end_to_useful_response_for_user_lag"),
        ("asr_end_evage", "asr_end_to_useful_response_for_robot_lag"),
    ),
    "tts_first_chunk_evage": (
        ("vins_response_evage", "vins_response_to_tts_first_chunk_lag"),
        ("vins_response_evage", "vins_response_to_tts_first_chunk_user_lag"),
        (events.EventVinsResponseSent.NAME, "vins_response_sent_to_tts_first_chunk_lag"),
        (events.EventVinsResponseSent.NAME, "vins_response_sent_to_tts_first_chunk_user_lag"),
        (events.EventTtsStart.NAME, "tts_start_to_tts_first_chunk_lag"),
        (events.EventTtsStart.NAME, "tts_start_to_tts_first_chunk_user_lag"),
        (events.EventTtsStart.NAME, "tts_start_to_tts_first_chunk_robot_lag"),
    ),
    "tts_first_chunk_nocache_evage": (
        (events.EventTtsStart.NAME, "tts_start_to_tts_first_chunk_nocache_lag"),
        (events.EventTtsStart.NAME, "tts_start_to_tts_first_chunk_user_nocache_lag"),
    ),
    events.EventTtsCacheResponse.NAME: (
        (events.EventTtsStart.NAME, "user_tts_cache_response_lag"),
        (events.EventTtsStart.NAME, "robot_tts_cache_response_lag"),
    ),
    "vins_response_evage": (
        ("asr_end_evage", "asr_end_to_vins_response_lag"),
        ("asr_end_evage", "user_asr_end_to_vins_response_lag"),
        ("asr_end_evage", "robot_asr_end_to_vins_response_lag"),
        ("vins_request_eou_evage", "vins_request_eou_to_vins_response_lag"),
    ),
    events.EventVinsUaasEnd.NAME: (
        (events.EventVinsUaasStart.NAME, "user_vins_uaas_lag"),
        (events.EventVinsUaasStart.NAME, "robot_vins_uaas_lag"),
    ),
    events.EventGetSpotterValidationResultEnd.NAME: (
        (events.EventGetSpotterValidationResultStart.NAME, "user_spotter_validation_lag"),
        (events.EventGetSpotterValidationResultStart.NAME, "robot_spotter_validation_lag"),
    ),
    "vins_session_save_end_evage": (
        ("vins_session_save_start_evage", "vins_session_save_duration"),
        ("vins_session_save_start_evage", "user_vins_session_save_duration"),
        ("vins_session_save_start_evage", "robot_vins_session_save_duration"),
    ),
    events.EventFinishExecuteVinsDirectives.NAME: (
        (events.EventStartExecuteVinsDirectives.NAME, "execute_vins_directives_lag"),
        (events.EventStartExecuteVinsDirectives.NAME, "user_execute_vins_directives_lag"),
        (events.EventStartExecuteVinsDirectives.NAME, "robot_execute_vins_directives_lag"),
    ),
}

_REVERSE_LAGS_FOR_POSTPONED_EVENTS = {
    "useful_asr_result_evage": (
        (events.EventVinsPersonalDataEnd.NAME, "useful_asr_result_to_vins_personal_data_end_lag"),
    )
}

VINS_CONTEXT_MAX_LEN = config['vins'].get('session_max_size') or -1


def get_by_path(dct, *keys):
    for k in keys:
        dct = dct.get(k, None)
        if dct is None:
            return None
    return dct


def get_response_delay(vins_timings, tts_timings):
    if not vins_timings:
        return None
    eou_event = vins_timings.get_event(events.EventEndOfUtterance)
    user_start_time = eou_event if eou_event is not None else 0

    response_start_time = None
    first_chunk_time = tts_timings.get_event(events.EventFirstTTSChunkSec)
    if first_chunk_time is not None:
        response_start_time = first_chunk_time
    else:
        response_start_time = vins_timings.get_event(events.EventVinsResponse)
    assert(response_start_time is not None)
    return max(response_start_time - user_start_time, 0)


def _get_asr_partial_threshold(payload, uaas_flags):
    threshold = payload.get('settings_from_manager', {}).get('asr_result_threshold', 0)

    threshold_flag_prefix = 'asr_partials_threshold_'
    for flag in uaas_flags:
        if flag.startswith(threshold_flag_prefix):
            try:
                threshold = float(flag[len(threshold_flag_prefix):])
            except:
                pass
            break

    return threshold


# -------------------------------------------------------------------------------------------------
class ContextLoadResponseWrap:
    @staticmethod
    def _try_http_content(resp, name):
        if not resp.HasField(name):
            return None
        http_response = getattr(resp, name)
        if (http_response.StatusCode // 100) != 2:
            return None
        return http_response.Content

    @classmethod
    def contacts(cls, resp):
        content = cls._try_http_content(resp, "ContactsResponse")
        if content is None and resp.HasField("PredefinedContacts"):
            content = resp.PredefinedContacts.Value
        if content is None:
            return None
        return json.loads(content)

    @classmethod
    def contacts_proto(cls, resp):
        if not resp.HasField('ContactsProto'):
            return ""
        proto = TSpeechKitRequestProto.TContacts()
        proto.Status = 'ok'
        proto.Data.CopyFrom(resp.ContactsProto)
        return base64.b64encode(proto.SerializeToString())

    @classmethod
    def memento(cls, resp):
        content = cls._try_http_content(resp, "MementoResponse")
        if content is None:
            return None
        return Memento.parse_response(content)

    @classmethod
    def smart_home(cls, resp):
        if resp.HasField("IoTUserInfo"):
            return make_smart_home_response_from_proto(resp.IoTUserInfo)

        content = cls._try_http_content(resp, "QuasarIotResponse")
        return parse_smart_home_response(content)

    @classmethod
    def notificator(cls, resp):
        content = cls._try_http_content(resp, "NotificatorResponse")
        if content is None:
            return None
        return NotificatorApi.parse_response(content)

    @classmethod
    def flags_json(cls, resp):
        content = cls._try_http_content(resp, "FlagsJsonResponse")
        if content is None:
            return None
        return FlagsJsonResponse(content, call_source="context_load")

    @classmethod
    def laas(cls, resp):
        content = cls._try_http_content(resp, "LaasResponse")
        if content is None:
            return None
        return json.loads(content)

    @classmethod
    def vins_session_ex(cls, resp):
        if not resp.HasField("MegamindSessionResponse"):
            return 0, None

        cachalot_response = resp.MegamindSessionResponse
        if not cachalot_response.HasField("MegamindSessionLoadResp"):
            return 0, None

        session = cachalot_response.MegamindSessionLoadResp.Data

        return CityHash64(session), base64.b64encode(session).decode("ascii")

    def __init__(self, fut, log, proc_id=None):
        self._fut = fut
        self._log = log
        self._proc_id = proc_id
        self._cancelled = False

    def set_cancelled(self):
        self._cancelled = True
        if (self._fut is not None) and (not self._fut.done()):
            self._fut.set_result(None)
            GlobalCounter.CLD_APPLY_CANCELLED_SUMM.increment()

    @gen.coroutine
    def _get(self, parser, counter_ok, counter_err, name, timeout, fallback_result=None):
        if self._fut is None:
            return fallback_result

        try:
            response = yield gen.with_timeout(datetime.timedelta(seconds=timeout), self._fut)
            if response is None:
                return fallback_result

            ret = parser(response)
            counter_ok.increment()
            return ret
        except Exception:
            if not self._cancelled:
                self._log.exception(f"{self._proc_id} {name} from ContextLoadResponse failed (fallback to classic)")
                counter_err.increment()
            return fallback_result

    @gen.coroutine
    def get_smart_home(self, timeout=10):
        ret = yield self._get(
            self.smart_home,
            GlobalCounter.CLD_APPLY_SMART_HOME_OK_SUMM,
            GlobalCounter.CLD_APPLY_SMART_HOME_ERR_SUMM,
            "SmartHome",
            timeout=timeout
        )
        return ret

    @gen.coroutine
    def get_notificator(self, timeout=10):
        ret = yield self._get(
            self.notificator,
            GlobalCounter.CLD_APPLY_NOTIFICATOR_OK_SUMM,
            GlobalCounter.CLD_APPLY_NOTIFICATOR_ERR_SUMM,
            "Notificator",
            timeout=timeout
        )
        return ret

    @gen.coroutine
    def get_memento(self, timeout=10):
        ret = yield self._get(
            self.memento,
            GlobalCounter.CLD_APPLY_MEMENTO_OK_SUMM,
            GlobalCounter.CLD_APPLY_MEMENTO_ERR_SUMM,
            "Memento",
            timeout=timeout
        )
        return ret

    @gen.coroutine
    def get_contacts(self, timeout=10):
        ret = yield self._get(
            self.contacts,
            GlobalCounter.CLD_APPLY_CONTACTS_OK_SUMM,
            GlobalCounter.CLD_APPLY_CONTACTS_ERR_SUMM,
            "Contacts",
            timeout=timeout
        )
        return ret

    @gen.coroutine
    def get_contacts_proto(self, timeout=10):
        ret = yield self._get(
            self.contacts_proto,
            GlobalCounter.CLD_APPLY_CONTACTS_OK_SUMM,
            GlobalCounter.CLD_APPLY_CONTACTS_ERR_SUMM,
            "Contacts",
            timeout=timeout
        )
        return ret

    async def get_vins_session(self, timeout=10):
        ret = await self._get(
            self.vins_session_ex,
            GlobalCounter.CLD_APPLY_VINS_SESSION_OK_SUMM,
            GlobalCounter.CLD_APPLY_VINS_SESSION_ERR_SUMM,
            "VinsSession",
            timeout=timeout,
            fallback_result=(0, None),
        )
        return ret

    async def get_flags_json(self, timeout=10):
        ret = await self._get(
            self.flags_json,
            GlobalCounter.CLD_APPLY_FLAGS_JSON_OK_SUMM,
            GlobalCounter.CLD_APPLY_FLAGS_JSON_ERR_SUMM,
            "FlagsJson",
            timeout=timeout,
        )
        return ret

    async def get_laas(self, timeout=10):
        ret = await self._get(
            self.laas,
            GlobalCounter.CLD_APPLY_LAAS_OK_SUMM,
            GlobalCounter.CLD_APPLY_LAAS_ERR_SUMM,
            "Laas",
            timeout=timeout,
        )
        return ret


# -------------------------------------------------------------------------------------------------
class VinsProcessor(EventProcessor):
    def store_event_age(self, name, ts=None):  # 'ts' args MUST BE time.monotonic() result
        try:
            if ts is None:
                t = self.event.event_age
            else:
                t = self.event.event_age_for(ts)
            self.unistat.store2(name, t)
            self._lag_storage.store(name, t)
        except Exception as e:
            self.DLOG('ERROR: store_event_age: {}'.format(e))

    def get_event_age(self, name):
        return self._lag_storage.timings.get(name)

    def events_timings(self):
        return self._lag_storage.timings

    def __init__(self, *args, **kwargs):
        super(VinsProcessor, self).__init__(*args, **kwargs)
        self.unistat = self.system.unistat
        self.tts = None
        self.asr_partialprocessor = None
        self.prepend_tts = ""
        self.vins_adapter = None
        self.personal_data_helper = None
        self.personal_cards_helper = None
        self.stateless = False
        self.vins_context_accessor = None
        self._ref_count = 1
        self._smart_activation = None
        self._tts_cache = self.system.cache_manager.add_cache()
        self._tts_full_cache = self.system.cache_manager.add_cache()
        self._tts_preloader = None
        self._event_start_time = None
        self._lag_storage = TimestampLagStorage(_Lags, _REVERSE_LAGS_FOR_POSTPONED_EVENTS, self.unistat)
        self.useful_response_for_user_ts = None  # keep here first tts chunk or sending vins response timestamp for speed metrics
        self.tts_cache_success = None
        self.streaming_backends = None
        self.user_ticket_set = False
        self.predefined_iot_config = False
        self.memento = Memento(
            self._get_user_ticket,
            rt_log=self.rt_log,
            host=self.system.srcrwr['MEMENTO'],
            responses_storage=self.system.responses_storage,
            request_info=self.payload_with_session_data,
        )
        self.notificator = NotificatorApi(
            self.system.oauth_token,
            self.system.client_ip,
            rt_log=self.rt_log,
            url=self.system.srcrwr['NOTIFICATOR'],
            app_id=self.system.app_id,
            metrics_backend=GolovanBackend.instance()
        )
        self.matrix = MatrixApi(
            rt_log=self.rt_log,
            url=self.system.srcrwr['MATRIX'],
            metrics_backend=GolovanBackend.instance(),
        )
        self._vins_request_id = None
        self._vins_prev_request_id = None
        self._vins_seq_id = None

        self._contacts_future = Future()
        self._context_load_fut = None
        self._context_load_response_wrap = ContextLoadResponseWrap(None, self._log)

        self._extra_experiments = {}  # experiment flags received from UaaS/flags_json for this event

    def _conducting_tts_backward_apphost(self):
        return self.payload_with_session_data.get('settings_from_manager', {}).get('mvp_apphosted_tts_generate_backward', False)

    def process_extra(self, extra):
        self.system.logger.log_directive({
            "ForEvent": self.event.message_id,
            "type": f"Extra: {extra.proc_id}/{extra.ext}"
        })

        if extra.ext == "contextloadapply":
            msg = extra.get_response_protobuf()
            if (self._context_load_fut is not None) and (not self._context_load_fut.done()):
                self._context_load_fut.set_result(msg)
        elif extra.ext == "closettsprocessor":
            if self.tts:
                self.tts.close()
                self.tts = None
        elif extra.ext == "asr.result":
            # self.ERR("TODO: PROCESS_EXTRA_ASR: ", str(extra.payload))
            self.on_asr_result(extra.payload.get('response'))
        elif extra.ext == "spotter.validation":
            # self.ERR("TODO: PROCESS_EXTRA_SPOTTER: ", str(extra.payload))
            spotter_stream = self.streaming_backends.get('spotter')
            if spotter_stream:
                spottered = bool(extra.payload.get('response', {}).get('result'))
                spotter_stream.on_spotter_validation(spottered)  # fork coroutine here
        elif extra.ext == "system.eventexception":
            # user alredy got this exception so we MUST silently cancel this request
            self.ERR("close VINS processor by receiving EventException from uniproxy2: ",
                     extra.payload.get('response', {}).get('error', '?'))
            self.close()

    def log_ex(self, action: str, body=None):
        entry = {
            'ForEvent': self.event.message_id,
            'type': 'LogEx',
            'Action': action,
            'Request': {
                'prev_request_id': self._vins_prev_request_id,
                'request_id': self._vins_request_id,
                'sequence_number': self._vins_seq_id,
            }
        }

        if body:
            entry.update(body)

        self.system.logger.log_directive(entry, rt_log=self.rt_log)

    def process_event(self, event):
        validate_vins_event(event)

        super(VinsProcessor, self).process_event(event)

        GlobalCounter.VINS_PROCESSOR_EVENTS_SUMM.increment()

        request_id = get_by_path(self.payload_with_session_data, 'header', 'request_id')
        self._vins_request_id = request_id
        self._vins_prev_request_id = get_by_path(self.payload_with_session_data, 'header', 'prev_req_id')
        self._vins_seq_id = get_by_path(self.payload_with_session_data, 'header', 'sequence_number')

        app_device_id = get_by_path(self.payload_with_session_data, 'application', 'device_id')

        device_state = get_by_path(self.payload_with_session_data, 'request', 'device_state')
        if device_state is None:
            device_id = ''
        elif not isinstance(device_state, dict):
            raise EventException('request.device_state is not a dict instance', event.message_id)
        else:
            device_id = device_state.get('device_id')
            self._smart_activation = device_state.get('smart_activation')

        self.rt_log.log_request_context(request_id=request_id, app_device_id=app_device_id, device_id=device_id)

        processor_has_spotter = False
        processor_has_biometry = False
        event_type = None
        if isinstance(self, TextInput):
            event_type = 'vins.text_input'
        elif isinstance(self, CustomInput):
            event_type = 'vins.custom_input'
        elif isinstance(self, VoiceInput):
            event_type = 'vins.voice_input'
            processor_has_spotter = True
            processor_has_biometry = True
        elif isinstance(self, MusicInput):
            event_type = 'vins.music_input'
        if event_type:
            self.rt_log('process_event_started', event_type=event_type)

        if self._conducting_experiment("context_load_apply"):
            self._context_load_fut = Future()
            self._context_load_response_wrap = ContextLoadResponseWrap(self._context_load_fut, self._log, proc_id=self.proc_id)
            self._context_load_response_wrap.get_contacts().add_done_callback(lambda f: self._contacts_future.set_result(f.result()))

        fake = self._conducting_experiment("no_vins_requests")
        self._try_url_experiments()

        if self.system.use_datasync:
            self.personal_data_helper = PersonalDataHelper(
                system=self.system,
                request_info=self.payload_with_session_data,
                rt_log=self.rt_log,
                personal_data_response_future=self._context_load_fut
            )
        else:
            self.personal_data_helper = None

        if self.system.use_personal_cards:
            self.personal_cards_helper = PersonalCardsHelper(
                self.system.puid,
                self.rt_log,
                self.system.srcrwr['PERSONAL_CARDS'],
                self.system.srcrwr.ports.get('PERSONAL_CARDS')
            )
        else:
            self.personal_cards_helper = None

        self._event_start_time = time.monotonic()
        self.vins_adapter = VinsAdapter(
            system=self.system,
            message_id=self.event.message_id,
            payload=self.payload_with_session_data,
            on_vins_partial=self.spawn_on_vins_partial,
            on_vins_response=self.spawn_on_vins_response,
            on_vins_cancel=self.on_vins_cancel,
            personal_data_helper=self.personal_data_helper,
            rt_log=self.rt_log,
            processor=self,
            event_start_time=self._event_start_time,
            epoch=time.time(),
            fake=fake,
            has_spotter=processor_has_spotter,
            has_biometry=processor_has_biometry,
        )
        IOLoop.current().spawn_callback(self._load_vins_session)
        IOLoop.current().spawn_callback(self._get_user_ticket)
        IOLoop.current().spawn_callback(self._get_flags_json)
        IOLoop.current().spawn_callback(self._get_laas)
        if self._conducting_experiment("use_contacts"):
            IOLoop.current().spawn_callback(self._get_contacts)
        if self._conducting_experiment("contacts_as_proto"):
            IOLoop.current().spawn_callback(self._get_contacts_proto)

        if self.system.is_notification_supported():
            IOLoop.current().spawn_callback(self._load_notification_state)

        if self._conducting_experiment("use_memento"):
            IOLoop.current().spawn_callback(self._get_memento)

        if get_by_path(self.payload_with_session_data, 'request', 'iot_config') and self.system.is_robot:
            self.predefined_iot_config = True
            result = get_by_path(self.payload_with_session_data, 'request', 'iot_config')
            self.vins_adapter.set_smart_home_result(result, None)

        if self._can_use_smart_home():
            self.vins_adapter.set_smart_home_future(self._get_smart_home_future())

        if not self.payload_with_session_data.get('header'):
            self.payload_with_session_data['header'] = {}
        self.payload_with_session_data['header'].update(
            {'ref_message_id': self.event.message_id, 'session_id': self.session_id})

    def _conducting_experiment(self, experiment):
        return conducting_experiment(experiment, self.payload_with_session_data, self.system.uaas_flags)

    def _exp_value(self, experiment):
        return mm_experiment_value(experiment, self.payload_with_session_data)

    def spawn_on_vins_partial(self, *args, **kwargs):
        IOLoop.current().spawn_callback(self.on_vins_partial, *args, **kwargs)

    def _try_url_experiments(self):
        experiments = self.payload_with_session_data.get('request', {}).get('experiments', {})
        for key in experiments.keys():
            if key.startswith('UaasVinsUrl_'):
                self._try_use_vins_url(self.payload_with_session_data, key)

    def spawn_on_vins_response(self, *args, **kwargs):
        IOLoop.current().spawn_callback(self.on_vins_response, *args, **kwargs)

    @gen.coroutine
    def _get_user_ticket(self):
        if not self.user_ticket_set:
            self.user_ticket = yield self.system.get_bb_user_ticket()
            self.user_ticket_set = True
        return self.user_ticket

    @gen.coroutine
    def _load_notification_state(self):
        try:
            state = yield self._context_load_response_wrap.get_notificator()
            if state is not None:
                state = proto_to_json(state)

            self.vins_adapter.set_notification_state(state)
        except:
            self.vins_adapter.set_notification_state(None)

    @gen.coroutine
    def _send_sup_notification(self, data):
        try:
            yield self.notificator.send_sup_push(self.system.puid, data, test_id=self._exp_value('notificator_sup_test_id'))
        except Exception as e:
            self.ERR("_send_sup_notification error:", e, type(e), traceback.format_exc(5).replace("\n", ""))

    @gen.coroutine
    def _send_push_directive(self, data):
        try:
            yield self.notificator.send_sup_card_push(self.system.puid, data, test_id=self._exp_value('notificator_sup_test_id'))
        except Exception as e:
            self.ERR("_send_sup_card_notification error:", e, type(e), traceback.format_exc(5).replace("\n", ""))

    @gen.coroutine
    def _delete_pushes_directive(self, data):
        try:
            yield self.notificator.delete_pushes_directive(self.system.puid, data)
        except Exception as e:
            self.ERR("_delete_pushes_directive error:", e, type(e), traceback.format_exc(5).replace("\n", ""))

    @gen.coroutine
    def _push_typed_semantic_frame(self, data):
        try:
            yield self.notificator.push_typed_semantic_frame(data)
        except Exception as e:
            self.ERR("_push_typed_semantic_frame error:", e, type(e), traceback.format_exc(5).replace("\n", ""))

    @gen.coroutine
    def _add_schedule_action(self, data):
        try:
            yield self.matrix.add_schedule_action(data)
        except Exception as e:
            self.ERR("_add_schedule_action error:", e, type(e), traceback.format_exc(5).replace("\n", ""))

    @gen.coroutine
    def _get_contacts(self):
        try:
            result = yield self._context_load_response_wrap.get_contacts()
            self.vins_adapter.set_contacts_state(result)
        except Exception as e:
            self.ERR("_get_contacts error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            self.vins_adapter.set_contacts_state(None)

    @gen.coroutine
    def _get_contacts_proto(self):
        try:
            result = yield self._context_load_response_wrap.get_contacts_proto()
            self.vins_adapter.set_contacts_proto_state(result)
        except Exception as e:
            self.ERR("_get_contacts_proto error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            self.vins_adapter.set_contacts_proto_state(None)

    @gen.coroutine
    def _get_memento(self):
        try:
            if self.system.is_quasar:
                # surface_id = device_id
                surface_id = get_by_path(self.payload_with_session_data, 'request', 'device_state', 'device_id')
            else:
                surface_id = self._get_uuid()

            result = None
            if surface_id:  # perf test do not send device_state (as minimum)
                result = yield self._context_load_response_wrap.get_memento()

            self.vins_adapter.set_memento_state(result)
        except Exception as e:
            self.ERR("_get_memento error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            self.vins_adapter.set_memento_state(None)

    @gen.coroutine
    def _update_memento(self, data):
        try:
            yield self.memento.update(data)
        except Exception as e:
            self.ERR("_update_memento error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            raise

    async def _get_flags_json(self):
        try:
            self._on_flags_json_response_wrap(await self._context_load_response_wrap.get_flags_json())
        except Exception as e:
            self.ERR("_get_flags_json error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            raise

    async def _get_laas(self):
        try:
            laas_data = await self._context_load_response_wrap.get_laas()
            if laas_data is not None:
                deepupdate(self.payload_with_session_data, {
                    "request": {
                        "laas_region": laas_data
                    }
                })

            self.vins_adapter.set_laas_result(laas_data)
        except Exception as e:
            self.ERR("_get_laas error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
            self.vins_adapter.set_laas_result({})
            raise

    async def _load_vins_session(self):
        try:
            # VOICESERV-1937 disable ydb logic for analytics purposes
            session = self.payload_with_session_data.get("request", {}).pop("session", None)
            if self._conducting_experiment("stateless_uniproxy_session"):
                self.stateless = True

            # DIALOG-5443 Use session from request when "stateless_uniproxy_session" experiment set
            if session is None and not self.stateless:
                self.vins_context_accessor = await get_vins_context_accessor(
                    self.system.session_id,
                    self.system.puid,
                    self.payload_with_session_data,
                    self.rt_log,
                )

                session_hash, session = await self._context_load_response_wrap.get_vins_session()

                try:
                    self.log_ex('load_vins_session', {
                        'key': self.vins_context_accessor._get_load_key(),
                        'size': len(session) if session else 0,
                        'hash': session_hash,
                    })
                except Exception as ex:
                    self.ERR('self.log_ex failed: {}'.format(ex))

                if VINS_CONTEXT_MAX_LEN != -1 and session and len(session) > VINS_CONTEXT_MAX_LEN:
                    session = None
                    self.ERR("too long vins-context session. drop it")
                    GlobalCounter.increment_counter('vins_context_too_long', self.system.app_type)

            self.vins_adapter.set_vins_session(session)
            if self._conducting_experiment("context_load_diff"):
                request_id = get_by_path(self.payload_with_session_data, "header", "request_id")
                self.system.responses_storage.store(request_id, "megamind_session", session)
        except ReferenceError:
            pass
        except Exception as e:
            self.rt_log.exception('finish_load_session')
            self.vins_adapter.set_vins_session(None)
            self.ERR("_load_vins_session error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
        finally:
            self.store_event_age(events.EventVinsSessionLoadEnd.NAME)

    async def _save_vins_sessions(self, sessions):
        self.store_event_age("vins_session_save_start_evage")
        try:
            size = 0
            for v in sessions.values():
                size += len(v)

            with rtlog_child_activation(self.rt_log, 'save_vins_session_to_ydb'):
                self.rt_log('start_save_session', size=size)
                infos = await self.vins_context_accessor.save_base64_ex(sessions)
                self.rt_log('finish_save_session')

                try:
                    self.log_ex('save_vins_session', {
                        'key': self.vins_context_accessor._get_save_key(),
                        'sessions': infos,
                    })
                except Exception as ex:
                    self.ERR('self.log_ex failed: {}'.format(ex))

        except ReferenceError:
            pass
        except Exception as e:
            self.rt_log.exception('finish_save_session')
            self.ERR("_save_vins_sessions error:", e, type(e), traceback.format_exc(5).replace("\n", ""))
        finally:
            self.store_event_age("vins_session_save_end_evage")

    def inc_ref_count(self):
        self._ref_count += 1

    def dec_ref_count(self):
        self._ref_count -= 1
        if self._ref_count == 0:
            self.close()

    @gen.coroutine
    def create_or_update_user_coro(self, params):
        self.store_event_age("vins_update_user_start_evage")
        try:
            group_id = YabioStream.get_group_id(params)
            with rtlog_child_activation(self.rt_log, 'create_or_update_yabio_user'):
                context_storage = self.system.get_yabio_storage(group_id)
                yield context_storage.add_user(params['user_id'], params['requests_ids'])
        except Exception as exc:
            self.rt_log.exception('yabio_result_received', type='create_or_update_user')
            self.ERR("save_voiceprint error:", str(exc))
            self.dispatch_error("Can't save voiceprint: {}".format(str(exc)))
            raise

        self.rt_log('yabio_result_received', type='create_or_update_user', result=params['user_id'])
        self.INFO("save_voiceprint success")

    @gen.coroutine
    def remove_user_coro(self, params):
        self.store_event_age("vins_remove_user_start_evage")
        try:
            group_id = YabioStream.get_group_id(params)
            with rtlog_child_activation(self.rt_log, 'remove_yabio_user'):
                context_storage = self.system.get_yabio_storage(group_id)
                yield context_storage.remove_user(params['user_id'])
        except Exception as exc:
            self.rt_log.exception('yabio_result_received', type='remove_user')
            self.ERR("remove_voiceprint error:", str(exc))
            self.dispatch_error("Can't remove voiceprint: {}".format(str(exc)))
            raise

        self.rt_log('yabio_result_received', type='remove_user', result=params['user_id'])
        self.INFO("remove_voiceprint success")

    def _get_uuid(self):
        uuid = self.payload_with_session_data.get("application", {}).get('uuid')
        if not uuid:
            uuid = self.payload_with_session_data.get('vins', {}).get("application", {}).get('uuid')
        return uuid

    def is_voice_input(self):
        return self.streaming_backends is not None

    async def on_vins_partial(self, what_to_say=None):
        self._start_precaching(what_to_say)

    @gen.coroutine
    def notification_subscribe_coro(self, subscription_id, need_subscribe):
        try:
            yield self.notificator.manage_subscriptions(self.system.puid, subscription_id, need_subscribe)
        except Exception as e:
            self.ERR("subscribe error: {}".format(e))

    @gen.coroutine
    def notifications_mark_read_coro(self, id_lst):
        try:
            yield self.notificator.mark_read(self.system.puid, id_lst)
        except Exception as e:
            self.ERR("notifications mark read error: {}".format(e))

    async def on_vins_response(self, raw_response=None, error=None, what_to_say=None,
                               vins_directives=None, force_eou=False, uniproxy_vins_timings=None):
        self.store_event_age("vins_response_evage")
        if error:
            self.dispatch_error(error)
            return

        if uniproxy_vins_timings:
            uniproxy_vins_timings.to_global_counters()
            uniproxy_vins_timings.import_events(self._lag_storage.timings)
            if self.is_voice_input() and self.first_asr_result_ts is not None:
                uniproxy_vins_timings.on_event(events.EventFirstAsrResult, self.first_asr_result_ts)
            if self.system.synchronize_state_finished_timer:
                uniproxy_vins_timings.on_event_with_value(
                    events.EventSynchronizeState, self.system.synchronize_state_finished_timer.start_ts)
                duration = self.system.synchronize_state_finished_timer.duration
                if duration is not None:
                    uniproxy_vins_timings.on_event_with_value(events.EventSynchronizeStateFinished, duration)
                uniproxy_vins_timings.on_event_with_value(
                    events.EventStartProcessVoiceInput, self.create_ts - self.system.synchronize_state_finished_timer.start_ts)
            self.ERR('UniproxyVinsTimings {} {}'.format(
                self.event.message_id,
                json.dumps(uniproxy_vins_timings.to_dict())))
            self.system.logger.log_directive(
                {
                    "header": {
                        "name": "UniproxyVinsTimings",
                        "namespace": "System",
                        "refMessageId": self.event.message_id,
                    },
                    "payload": uniproxy_vins_timings.to_dict()
                },
                rt_log=self.rt_log,
            )
            if self._need_to_send_timings():
                params = {
                    'message_id': self.event.message_id,
                    'uuid': self.system.uuid(),
                }
                self.system.write_directive(
                    uniproxy_vins_timings.to_directive(
                        event_id=self.event.message_id,
                        params=params))

        if raw_response:
            sessions_to_save = raw_response.get('sessions')
            # [DIALOG-5131] We want to return session in response when
            # "stateless_uniproxy_session" experiment set
            if sessions_to_save and not self.stateless:
                if self.vins_context_accessor:
                    IOLoop.current().spawn_callback(self._save_vins_sessions, sessions_to_save)
                del raw_response['sessions']

        if vins_directives:
            self.DLOG('process VINS directives')
            self.store_event_age(events.EventStartExecuteVinsDirectives.NAME)
            save_params = None
            update_datasync_directives = []
            vins_directives_exec_futures = []
            context_save_executor = AppHostedContextSave(
                self.system,
                self.payload_with_session_data.get("request", {}).get("experiments", {}),
            )
            for x in vins_directives:
                name = x.get("name")
                if name == "save_voiceprint":
                    payload = x.get("payload", {})
                    save_params = deepupdate(self.payload_with_session_data, {
                        "user_id": payload.get("user_id", payload.get("created_uid")),
                        "requests_ids": payload.get("requests", [])
                    })

                    # fork coroutine
                    # will not run this on CONTEXT_SAVE (it changes yabio_context)
                    vins_directives_exec_futures.append(self.create_or_update_user_coro(save_params))
                elif name == "remove_voiceprint":
                    remove_params = deepupdate(self.payload_with_session_data, {
                        "user_id": x.get("payload", {}).get("user_id")
                    })

                    # fork coroutine
                    # will not run this on CONTEXT_SAVE (it changes yabio_context)
                    vins_directives_exec_futures.append(self.remove_user_coro(remove_params))
                elif name == "update_datasync" and self.system.use_datasync:
                    update_datasync_directives.append(x.get("payload", {}))
                elif name == "force_interruption_spotter":  # deprecated, ignored
                    self.payload_with_session_data["force_interruption_spotter"] = True
                    fut = Future()
                    fut.set_result(None)
                    vins_directives_exec_futures.append(fut)
                elif name == "update_notification_subscription":
                    payload = x.get('payload', {})
                    subscription_id = payload.get('subscription_id')
                    unsubscribe = payload.get('unsubscribe')
                    if unsubscribe is None or subscription_id is None:
                        continue

                    # fork coroutine
                    context_save_executor.add_directive(
                        x, self.notification_subscribe_coro, subscription_id, not unsubscribe)

                elif name == "mark_notification_as_read":
                    payload = x.get('payload', {})
                    id_lst = payload.get('notification_ids', [])
                    id_ = payload.get('notification_id')
                    if id_:
                        id_lst.append(id_)

                    if id_lst:
                        # fork coroutine
                        context_save_executor.add_directive(x, self.notifications_mark_read_coro, id_lst)

                elif name == "update_memento":
                    payload = x.get('payload', {}).get('user_objects')
                    if payload and self._conducting_experiment("use_memento"):
                        context_save_executor.add_directive(x, self._update_memento, payload)

                elif name == "personal_cards":
                    context_save_executor.add_directive(
                        x, self.personal_cards_helper.send_personal_card, x.get("payload", {}))

                elif name == "push_message":
                    payload = x.get('payload')
                    if payload:
                        context_save_executor.add_directive(x, self._send_sup_notification, payload)

                elif name == 'send_push_directive':
                    payload = x.get('payload')
                    if payload:
                        context_save_executor.add_directive(x, self._send_push_directive, payload)

                elif name == 'delete_pushes_directive':
                    payload = x.get('payload')
                    if payload:
                        context_save_executor.add_directive(x, self._delete_pushes_directive, payload)

                elif name == 'push_typed_semantic_frame':
                    payload = x.get('payload')
                    if payload:
                        context_save_executor.add_directive(x, self._push_typed_semantic_frame, payload)

                elif name == 'add_schedule_action':
                    payload = x.get('payload')
                    if payload:
                        context_save_executor.add_directive(x, self._add_schedule_action, payload)

            if update_datasync_directives:
                context_save_executor.add_directive(
                    x, self.personal_data_helper.update_personal_data, update_datasync_directives)

            if self._conducting_experiment("apphosted_context_save"):
                user_ticket = await self._get_user_ticket()
                await context_save_executor.execute(user_ticket=user_ticket, message_id=self.event.message_id)

            for func, args in context_save_executor.unexecuted_funcs:
                vins_directives_exec_futures.append(func(*args))

            if vins_directives_exec_futures:
                try:
                    await gen.convert_yielded(gen.multi(vins_directives_exec_futures))
                    self.store_event_age(events.EventFinishExecuteVinsDirectives.NAME)
                except Exception as exc:
                    self.dispatch_error("Fail execute vins directives: {}".format(str(exc)))
                    return

        if raw_response:
            self.system.write_directive(Directive(
                namespace='Vins',
                name='VinsResponse',
                payload=raw_response,
                event_id=self.event.message_id))
            GlobalCounter.VINS_RESPONSE_SENT_SUMM.increment()
            self.store_event_age(events.EventVinsResponseSent.NAME)
            self.useful_response_for_user_ts = time.monotonic()

        uniproxy_tts_timings = UniproxyTTSTimings(request_start_time_sec=self._event_start_time)
        if what_to_say:
            if self.say(what=what_to_say, uniproxy_tts_timings=uniproxy_tts_timings):
                self.inc_ref_count()
        else:
            self._report_uniproxy_tts_timings(uniproxy_tts_timings)
        delay = get_response_delay(uniproxy_vins_timings, uniproxy_tts_timings)
        if delay is not None:
            suffix = '_quasar' if self.system.is_quasar else ''
            GlobalTimings.store(RESPONSE_DELAY_HGRAM + suffix, delay)
        self.dec_ref_count()

    def on_vins_cancel(self, reason):
        self.system.logger.log_directive(
            {
                'ForEvent': self.event.message_id,
                'type': 'VinsCanceled',
                'Body': {'reason': reason}
            },
            rt_log=self.rt_log,
        )

    def on_tts_cache_response(self, success):
        self.store_event_age(events.EventTtsCacheResponse.NAME)
        self.tts_cache_success = success

    def _create_tts_payload(self, what):
        d = {
            "lang": "ru-RU",
            "voice": "shitova.gpu",
            "format": "Opus",
            "quality": "UltraHigh",
            "cache_id": self._tts_cache.cache_id,
            "full_tts_cache_id": self._tts_full_cache.cache_id,
            "text": self.prepend_tts + what
        }
        if self.system.logger.get_message_filter() is VinsSensitiveDataFilter:
            # forbid logging on TTS if vins response contains sensitive data
            d["do_not_log"] = True
        d.update(self.payload_with_session_data)
        return d

    def _start_precaching(self, what):
        if self._conducting_tts_backward_apphost():
            return

        if what and not self.is_closed() and ((not self._conducting_experiment(UNIPROXY_DISABLE_TTS_PRECACHE_EXP)) or
                                              self._conducting_experiment(UNIPROXY_FULL_TTS_PRECACHE_EXP)):
            self.inc_ref_count()
            if self._tts_preloader:
                self._tts_preloader.close()
            self._tts_preloader = TTSPreloader(self._tts_cache, self._tts_full_cache, self.system, self.rt_log,
                                               self.event.message_id, on_close=self.dec_ref_count)
            self._tts_preloader.start(self._create_tts_payload(what))

    def say(self, what, uniproxy_tts_timings=None):
        if self.is_closed() or not what:
            return False

        d = self._create_tts_payload(what)
        self.store_event_age(events.EventTtsStart.NAME)

        def on_first_tts_chunk(from_cache):
            if self.is_closed():
                return
            self.store_event_age("tts_first_chunk_evage")
            if not from_cache:
                self.store_event_age("tts_first_chunk_nocache_evage")
            if uniproxy_tts_timings is not None:
                self.useful_response_for_user_ts = time.monotonic()
                uniproxy_tts_timings.on_event(events.EventFirstTTSChunkSec)
                self._report_uniproxy_tts_timings(uniproxy_tts_timings)

        def on_create_tts_processor(processor):
            processor.on_first_chunk = on_first_tts_chunk
            processor.on_tts_cache_response = self.on_tts_cache_response
            processor.on_close = self.dec_ref_count

        try:
            namespace = "TTS"
            if self._conducting_tts_backward_apphost():
                namespace = "TTSBackwardApphost"

            self.tts = self.system.process_event(
                Event(
                    {
                        "header": {
                            "name": "Generate",
                            "namespace": namespace,
                            # We use the same message id to bind all the results with the same refMessageId
                            "messageId": self.event.message_id,
                            "refStreamId": self.event.ref_stream_id
                        },
                        "payload": d,
                    },
                    birth_ts=self.event.birth_ts,
                    internal=True
                ),
                rt_log=self.rt_log,
                on_create_processor=on_create_tts_processor,
            )
            return True
        except:
            self.EXC("VinProcessor.say(): failed to create TTS.Generate sub processor")
            return False

    def close(self):
        try:
            self._context_load_response_wrap.set_cancelled()

            if self.tts:
                self.tts.close()
                self.tts = None

            if self._tts_preloader:
                self._tts_preloader.close()
                self._tts_preloader = None
                self.system.cache_manager.remove_cache(self._tts_cache)
                self.system.cache_manager.remove_cache(self._tts_full_cache)

            if self.vins_adapter:
                self.vins_adapter.close()
        except Exception:
            self.EXC('VinsProcessor.close() fail')
        try:
            self.system.logger.set_message_filter(None)
        except:
            pass
        super().close()

    def _report_uniproxy_tts_timings(self, uniproxy_tts_timings):
        if self.useful_response_for_user_ts is not None:
            self.store_event_age(events.EventUsefulResponseForUser.NAME, self.useful_response_for_user_ts)
            self.useful_response_for_user_ts = None

        if not uniproxy_tts_timings:
            return

        if self.tts_cache_success is not None:
            uniproxy_tts_timings.on_tts_cache(self.tts_cache_success)
        uniproxy_tts_timings.import_events(self._lag_storage.timings)
        self.ERR('UniproxyTTSTimings {} {}'.format(
            self.event.message_id,
            json.dumps(uniproxy_tts_timings.to_dict())))
        self.system.logger.log_directive(
            {
                "header": {
                    "name": "UniproxyTTSTimings",
                    "namespace": "System",
                    "refMessageId": self.event.message_id,
                },
                "payload": uniproxy_tts_timings.to_dict()
            },
            rt_log=self.rt_log,
        )
        if self._need_to_send_timings():
            params = {
                'message_id': self.event.message_id,
                'uuid': self.system.uuid()
            }
            self.system.write_directive(uniproxy_tts_timings.to_directive(
                event_id=self.event.message_id,
                params=params))

    def _need_to_send_timings(self):
        return self._conducting_experiment('uniproxy_vins_timings')

    def _can_use_smart_home(self):
        return self.system.get_oauth_token() and not self.predefined_iot_config

    def on_flags_json_response(self, resp):
        if resp is None:
            return

        try:
            self.system.uaas_test_ids = resp.get_all_test_id()
            self.system.uaas_exp_boxes = resp._exp_boxes

            flags = resp.get_all_flags()
            self._extra_experiments.update(flags)

            for test_id in self.system.uaas_test_ids:
                self.system.log_experiment({
                    'id': test_id,
                    'type': 'flags_json',
                    'control': False,
                })

            # Try to set vins url via experiment
            for flag in flags:
                if flag.startswith('UaasVinsUrl_'):
                    self._try_use_vins_url(self.vins_adapter.params, flag, force=True)
                    break

        except Exception as exc:
            self.WARN("error on handling flags_json response: {}".format(exc))
            self.EXC(exc)

    def start_uaas(self):

        def _on_uaas_error(err):
            self.store_event_age(events.EventVinsUaasEnd.NAME)

        self._uaas_start_ts = time.monotonic()
        if self._try_use_flags_json(self.event, call_source="zalipanie"):
            self.store_event_age(events.EventVinsUaasStart.NAME)
        else:
            self.after_uaas()

    def _process_external_experiments(self):
        super()._process_external_experiments()

        try:
            GlobalTimings.store('vins_uaas_proc_dur', time.monotonic() - self._uaas_start_ts)
        except:
            # AttributeError: 'VoiceInput' object has no attribute '_uaas_start_ts'
            pass

        self.vins_adapter.add_extra_experiments(self._extra_experiments)
        self.after_uaas()
        if self.system:
            try:
                self.system.logger.log_directive(
                    {
                        "type": "ZeroTesting",
                        "ForEvent": self.event.message_id,
                        "result": {"extra_experiments": self._extra_experiments}
                    },
                    rt_log=self.rt_log,
                )
            except Exception as ex:
                self.system.logger.log_directive(
                    {
                        "type": "ZeroTesting",
                        "ForEvent": self.event.message_id,
                        "result": {
                            "Error": "{}".format(ex)
                        },
                    },
                    rt_log=self.rt_log,
                )

    @gen.coroutine
    def _get_smart_home(self):
        result = yield self._context_load_response_wrap.get_smart_home()

        if result is None:
            self.ERR('Smart home: failed to load smart home info')
            result = ("", {})

        return result

    @gen.coroutine
    def _get_smart_home_and_set_result(self):
        new_proto_api, smart_home = yield self._get_smart_home()
        self.vins_adapter.set_smart_home_result(new_proto_api, smart_home)
        return smart_home

    def _get_smart_home_future(self):
        fut = Future()

        @gen.coroutine
        def coro():
            try:
                result = yield self._get_smart_home()
                fut.set_result(result)
            except Exception as err:
                fut.set_exception(err)

        IOLoop.current().spawn_callback(coro)
        return fut


class BufferBackend:
    def __init__(self):
        self.buffer = []
        self.back = None
        self._closed = False
        self.attrs = [
            'attrs',
            'back',
            'buffer',
            '_closed',
            'add_chunk',
            'close',
            'is_closed',
            'init_back',
        ]

    def add_chunk(self, *args, **kwargs):
        if self.back:
            self.back.add_chunk(*args, **kwargs)
        else:
            self.buffer.append((args, kwargs))

    def close(self, *args, **kwargs):
        if self.back:
            self.back.close(*args, **kwargs)
            self.back = None
        self._closed = True

    def is_closed(self, *args, **kwargs):
        if self.back:
            return self.back.is_closed(*args, **kwargs)
        else:
            return self._closed

    def init_back(self, back):
        self.back = back
        for msg in self.buffer:
            self.back.add_chunk(*(msg[0]), **(msg[1]))

        if self._closed:
            self.back.close()

    def __getattribute__(self, name):
        if name not in object.__getattribute__(self, 'attrs'):
            back = object.__getattribute__(self, "back")
            if back is not None:
                return getattr(back, name)
        return object.__getattribute__(self, name)


class ProxyBackend(object):
    def __init__(self, create_back, is_buffer=False, **kwargs):
        if is_buffer:
            self.is_buffer = True
            self.back = BufferBackend()
            self.create_back = create_back
            self.additional_kwargs = kwargs
        else:
            self.is_buffer = False
            self.back = create_back(**kwargs)

        self.attrs = [
            'attrs',
            'back',
            'is_buffer',
            'create_back',
            'additional_kwargs',
            'close',
            'init_back',
            'is_closed',
        ]

    def close(self, *args, **kwargs):
        if self.back:
            self.back.close(*args, **kwargs)
            self.back = None

    def is_closed(self, *args, **kwargs):
        if not self.back:
            return True
        return self.back.is_closed()

    def init_back(self):
        if self.is_buffer:
            self.back.init_back(self.create_back(**self.additional_kwargs))

    def __getattribute__(self, name):
        if name not in object.__getattribute__(self, 'attrs'):
            back = object.__getattribute__(self, "back")
            if back is not None:
                return getattr(back, name)
        return object.__getattribute__(self, name)


@register_event_processor
class VoiceInput(VinsProcessor):
    def __init__(self, *args, **kwargs):
        super(VoiceInput, self).__init__(*args, **kwargs)
        self.streaming_backends = {}
        self.yabio_scoring_logger = None
        self.yabio_logger = None
        self.first_logger = None
        self.full_logger = None
        self.first_asr_result_ts = None
        self.data_size = 0

        self.finish_ts = None
        self._spotter_future = None
        self._spotter_timeout = config.get("spotter", {}).get("timeout", 500)
        self._spotter_fast_circuit_timeout = config.get("spotter", {}).get("fast_circuit_timeout", 100)
        self._spotter_validation_result = None
        self._spotter_result_time = None
        self._apphosted_asr = False
        self.partials_numbers = {
            'asr': 0,
            'classification': 0,
            'scoring': 0
        }
        self.context_futures = ContextFutures()
        self.predefined_asr_result = False
        self.predefined_bio_scoring_result = False
        self.predefined_bio_classify_result = False

        if not hasattr(self.system, "voice_input_rerequest_counter"):
            self.system.voice_input_rerequest_counter = self._make_rerequest_counter()

    async def on_vins_response(self, force_eou=False, **kwargs):
        if self._spotter_result_time is not None:
            GlobalTimings.store('spotter_ahead_vins_time', time.time() - self._spotter_result_time)
        else:
            self.DLOG("on_vins_response: Spotter result time is not initialized")
        if force_eou and not self.streaming_backends["asr"].is_closed():
            self.process_streamcontrol(StreamControl({
                "messageId": self.event.message_id,
                "streamId": self.event.stream_id,
                "reason": 0,
                "action": StreamControl.ActionType.CLOSE
            }, is_client=False))
            self.system.close_stream(self.event.stream_id)
            return

        spotter_ok = await self.get_spotter_validation_result()
        if not spotter_ok:
            self.DLOG("spotter is not ok - break on_vins_response()")
            self.dec_ref_count()
            return

        if 'raw_response' in kwargs:
            self._patch_vins_response_with_contact_data(kwargs['raw_response'])

        await super(VoiceInput, self).on_vins_response(force_eou=force_eou, **kwargs)

    def process_event(self, event):
        self.system.voice_input_rerequest_counter.record_action()

        if self.system.is_quasar:
            deepsetdefault(event.payload, {"header": {
                "sequence_number": None,
                "prev_req_id": None
            }})

        if self.system.ab_asr_topic:
            event.payload['topic'] = self.system.ab_asr_topic

        self._apphosted_asr = event.payload.get('uniproxy2', {}).get('apphosted_asr', False)

        super(VoiceInput, self).process_event(event)
        self.store_event_age("vins_start_evage")

        if get_by_path(self.payload_with_session_data, 'request', 'predefined_bio_scoring_result') is not None and self.system.is_robot:
            self.predefined_bio_scoring_result = True
            result = get_by_path(self.payload_with_session_data, 'request', 'predefined_bio_scoring_result')
            self.on_scoring_result(result, None)

        if get_by_path(self.payload_with_session_data, 'request', 'predefined_bio_classify_result') is not None and self.system.is_robot:
            self.predefined_bio_classify_result = True
            result = get_by_path(self.payload_with_session_data, 'request', 'predefined_bio_classify_result')
            self.vins_adapter.set_classify_result(result, None)

        if get_by_path(self.payload_with_session_data, 'request', 'predefined_asr_result') is not None and self.system.is_robot:
            self.predefined_asr_result = True
            result = get_by_path(self.payload_with_session_data, 'request', 'predefined_asr_result')
            self.on_asr_result(result)
            return

        if get_by_path(self.payload_with_session_data, 'request', 'megamind_cookies'):
            self.complete_processing(need_buffer=True)
            self.start_uaas()
            return

        self.complete_processing()

    def complete_processing(self, need_buffer=False):
        if (
                not self._conducting_experiment("disable_biometry_scoring")
                and self._conducting_experiment("enable_biometry_scoring")
        ):
            self.payload_with_session_data["vins_scoring"] = True
            self.payload_with_session_data["biometry_score"] = True

        if "biometry_group" not in self.payload_with_session_data:
            self.payload_with_session_data["biometry_group"] = self.payload_with_session_data["uuid"]

        if self._conducting_experiment("enable_biometry_classify"):
            self.payload_with_session_data["biometry_classify"] = "gender,children"

        if self.payload_with_session_data.get("biometry_classify") and not self.predefined_bio_classify_result:
            if self._conducting_experiment("disable_biometry_classify"):
                self.payload_with_session_data.pop("biometry_classify")
            else:
                self.yabio_logger = AccessLogger("yabio_full", self.system, rt_log=self.rt_log)
                self.yabio_logger.start(
                    event_id=self.event.message_id,
                    resource="classify"
                )
                self.DLOG("Appending classify backend")
                self.streaming_backends['yabio'] = ProxyBackend(self.create_yabio, need_buffer)
        else:
            self.yabio_logger = None

        # vins_scoring must be set by expriment to controll this deadly feature
        need_scoring = self.payload_with_session_data.get("biometry_score") and \
            self.payload_with_session_data.get("vins_scoring") and \
            self.payload_with_session_data.get("biometry_group")

        self.payload_with_session_data["need_scoring"] = need_scoring

        if self.payload_with_session_data.get("enable_spotter_validation"):
            self.start_spotter(need_buffer)
        else:
            self.start_asr(need_buffer)

    def after_uaas(self):
        for name, cache_back in self.streaming_backends.items():
            cache_back.init_back()

    def start_spotter(self, need_buffer=False):
        self.store_event_age("spotter_start_evage")
        self._spotter_future = Future()

        self.streaming_backends['spotter'] = ProxyBackend(self.create_spotter, need_buffer)

        if self.payload_with_session_data.get("need_scoring"):
            self.start_scoring(need_buffer, spotter=True)

    def start_scoring(self, need_buffer=False, spotter=False):
        if self.predefined_bio_scoring_result:
            return

        if 'score' not in self.streaming_backends:
            self.store_event_age("scoring_start_evage")
            self.DLOG("need scoring")
            self.INFO("Appending scoring backend")
            self.system.logger.set_bio_group_id(
                self.event.stream_id,
                YabioStream.get_group_id(self.payload_with_session_data)
            )
            self.streaming_backends['score'] = ProxyBackend(self.create_scoring, need_buffer, spotter=spotter)

    def score_backend(self):
        return self.streaming_backends.get('score')

    def classify_backend(self):
        return self.streaming_backends.get('yabio')

    def start_asr(self, need_buffer=False):
        if not self._apphosted_asr:
            self.streaming_backends['asr'] = ProxyBackend(self.create_asr, need_buffer)

        if self.payload_with_session_data.get("need_scoring"):
            self.start_scoring(need_buffer)

        duplicate_asr_params = self.payload_with_session_data.get("duplicate_asr")
        if duplicate_asr_params:
            self.DLOG("Create duplcate asr stream")
            self.streaming_backends['duplicate_asr'] = ProxyBackend(self.create_duplicate_asr, need_buffer)

    def add_data(self, data):
        self.data_size += len(data)

        if self.first_logger:
            self.store_event_age("asr_first_chunk_evage")
            self.first_logger.end(code=200, size=self.data_size)
            self.first_logger = None

            self.rt_log('vins.asr_first_chunk_received_from_client', data_size=self.data_size)

        for backend in self.streaming_backends.values():
            backend.add_chunk(data)

    def process_streamcontrol(self, event):
        spotter_backend = self.streaming_backends.get('spotter')
        if event.action == StreamControl.ActionType.SPOTTER_END:
            if self.score_backend():
                self.score_backend().add_chunk(last_spotter_chunk=True)
            self.start_asr()
            if spotter_backend:
                spotter_backend.add_chunk()
            return EventProcessor.StreamControlAction.Flush
        else:
            asr_backend = self.streaming_backends.get('asr')
            if asr_backend:
                asr_backend.add_chunk()
            if spotter_backend:
                # spotter require double empty chunk for closing!
                spotter_backend.add_chunk()
                spotter_backend.add_chunk()

        return EventProcessor.StreamControlAction.Close

    def on_duplicate_asr_result(self, result):
        if self.is_closed():
            return

        try:
            self.system.logger.log_directive(
                {
                    "header": {
                        "name": "DuplicateResult",
                        "namespace": "ASR",
                        "messageId": "invalid_message_id",
                        "refMessageId": self.event.message_id,
                    },
                    "payload": {
                        "result": result,
                    }
                },
                rt_log=self.rt_log,
            )
        except ReferenceError:
            pass

    def on_duplicate_asr_error(self, err):
        try:
            self.system.logger.log_directive(
                {
                    "header": {
                        "name": "DuplicateResult",
                        "namespace": "ASR",
                        "messageId": "invalid_message_id",
                        "refMessageId": self.event.message_id,
                    },
                    "payload": {
                        "error": err,
                    }
                },
                rt_log=self.rt_log,
            )
        except ReferenceError:
            pass

    def on_asr_error(self, err):
        self.rt_log.error('vins.asr_error_received', error=err, data_size=self.data_size)
        if self.full_logger:
            self.full_logger.end(code=500, size=self.data_size)
            self.full_logger = None
        self.dispatch_error(err)
        # VOICESERV-2255
        self.system.close_stream(self.event.stream_id)
        self.on_vins_cancel('on_asr_error')
        self.close()

    def on_asr_error_ex(self, message, details=None):
        self.rt_log.error('vins.asr_error_received', error=message, data_size=self.data_size)
        if self.full_logger:
            self.full_logger.end(code=500, size=self.data_size)
            self.full_logger = None
        self.dispatch_error_ex(message, details=details)
        # VOICESERV-2255
        self.system.close_stream(self.event.stream_id)
        self.on_vins_cancel('on_asr_error')
        self.close()

    def on_scoring_result(self, result, processed_chunks):
        partial_number = self.partials_numbers['scoring']
        self.partials_numbers['scoring'] += 1
        self.store_event_age("scoring_end_evage")
        if self.yabio_scoring_logger:
            self.yabio_scoring_logger.end(code=200, size=self.data_size)
            self.yabio_scoring_logger = None
        self.rt_log('vins.yabio_result_received', type='score', data_size=self.data_size)
        self.INFO("Got scoring {} chunks result: {}".format(processed_chunks, result))
        result['partial_number'] = partial_number
        self.vins_adapter.set_scoring_result(result, processed_chunks)

    def on_scoring_error(self, err):
        if self.yabio_scoring_logger:
            self.yabio_scoring_logger.end(code=500, size=self.data_size)
            self.yabio_scoring_logger = None
        self.rt_log.error('vins.yabio_result_received',
                          type='score',
                          data_size=self.data_size,
                          error=err)
        self.vins_adapter.set_scoring_result(None, None)
        self.ERR("Yabio scoring error on Vins.VoiceInput:", err)

    def debug_hack(self, result):
        if result.get("endOfUtt"):
            recognition = result.get("recognition", [])
            if recognition:
                text = "".join([x.get("value", "") for x in recognition[0].get("words", [])])
                if text.lower() == u"ктовдомикесорокдва":
                    meta = result.get("metainfo", {})
                    self.prepend_tts = "{} {} {} ".format(
                        meta.get("lang", ""),
                        meta.get("topic", ""),
                        meta.get("version", ""),
                    ).translate(str.maketrans("-.", "  "))
                if text.lower() == u"сколькоконтактоввдомике":
                    self.prepend_tts = str(self.system.contacts.number) + '     '

    def on_asr_result(self, result):
        if self._apphosted_asr:
            # use partial number from apphosted asr
            part_num = result.get('asr_partial_number')
            self.partials_numbers['asr'] = part_num
        else:
            result['asr_partial_number'] = self.partials_numbers['asr']
            self.partials_numbers['asr'] += 1

        if self.first_asr_result_ts is None:
            self.first_asr_result_ts = time.monotonic()
        if result.get("endOfUtt", False):
            self.store_event_age("asr_end_evage")
            if self._spotter_result_time is not None:
                GlobalTimings.store('spotter_ahead_asr_time', time.time() - self._spotter_result_time)
            else:
                self.DLOG("on_asr_result: Spotter result time is not initialized")
        self.debug_hack(result)

        self.rt_log('vins.asr_result_received',
                    early_end_of_utt=result.get("earlyEndOfUtt", False),
                    end_of_utt=result.get("endOfUtt", False))

        if self.is_closed():
            return

        # From my very heart to all the users
        text = None

        # Recognize.on_result_asr can pop coreDebug, so we will copy it
        coreDebug = result.get('coreDebug')
        if self.payload_with_session_data.get("vins_partials_hearts"):
            if result.get("recognition"):
                text_result = result["recognition"][0]["normalized"]
                result["recognition"][0]["normalized"] += self.vins_adapter.get_partial_hearts(text_result.lower())
                if not self._apphosted_asr:
                    Recognize.on_result_asr(self, result)
                result["recognition"][0]["normalized"] = text_result
                text = text_result
        else:
            if not self._apphosted_asr or self.predefined_asr_result:
                Recognize.on_result_asr(self, result)
            if result.get("recognition"):
                text = result["recognition"][0]["normalized"]

        if coreDebug is not None:
            result['coreDebug'] = coreDebug

        if result.get("earlyEndOfUtt", False):
            self.DLOG("VINS got earlyEndOfUtt, close stream={}".format(self.event.stream_id))
            for name, backend in self.streaming_backends.items():
                if name == 'yabio' or name == 'score':
                    backend.add_chunk(last_chunk=True, text=text)
                else:
                    backend.add_chunk()
            asr_backend = self.streaming_backends.get("asr")
            if asr_backend:
                if len(self.streaming_backends) == 1:
                    self.system.close_stream(self.event.stream_id)
            return

        if result.get("endOfUtt", False):
            if not self.predefined_asr_result:
                self.finalize_asr_and_all_streaming_backends(text)

            context_hints = {}
            try:
                if self._can_use_contacts():
                    contact_index = self.context_futures['contacts']
                    result['contextRef'].sort(key=lambda x: x['confidence'], reverse=True)
                    contact_refs = [int(cr['contentIndex'])
                                    for cr in result['contextRef'] if cr['index'] == contact_index]
                    contacts = self.system.contacts.get_values(contact_refs)
                    if contacts:
                        self.INFO('contacts context success:', contacts)
                        context_hints['possible_contacts'] = [contacts[0]]
            except Exception as err:
                self.ERR('contacts context error:', err)

            if context_hints:
                result['context_hints'] = context_hints

        if result.get('endOfUtt', False) or self.is_partial_needed(result):
            self.vins_adapter.set_asr_result(result)

        if result.get('responseCode', 'OK') != 'OK':
            self.on_vins_cancel('got_asr_error')
            # got ASR error, error already transmitted to client in ASR.Result, so close processor
            self.close()

    def is_partial_needed(self, asr_result):
        thrown_partials_fraction = asr_result.get('thrownPartialsFraction', 1)
        if thrown_partials_fraction < 1:
            threshold = _get_asr_partial_threshold(self.payload_with_session_data, self.system.uaas_flags)
            if thrown_partials_fraction < threshold:
                self.system.logger.log_directive(
                    {
                        'type': 'SkipAsrPartial',
                        'ForEvent': self.event.message_id,
                        'Body': {
                            'threshold': threshold,
                            'thrown_partials_fraction': thrown_partials_fraction
                        }
                    },
                    rt_log=self.rt_log,
                )
                return False

        dropped_partials_fraction = self.payload_with_session_data.get(
            'settings_from_manager', {}).get('asr_dropped_partials_fraction', 0)
        if dropped_partials_fraction > 0:
            if random() < dropped_partials_fraction:
                self.system.logger.log_directive(
                    {
                        'type': 'DropAsrPartial',
                        'ForEvent': self.event.message_id,
                        'Body': {
                            'asr_dropped_partials_fraction': dropped_partials_fraction
                        }
                    },
                    rt_log=self.rt_log,
                )
                return False

        return True

    def on_spotter_result(self, is_spotter_valid, spotter_text=None, canceled_cause_of_multiactivation=False,
                          multiactivation_id=None, allow_activation_by_unvalidated_spotter=None):
        if self._spotter_validation_result is not None:
            return  # second call after timeout, - skip it

        self._spotter_validation_result = (
            is_spotter_valid or
            self.payload_with_session_data.get("disable_spotter_validation", False) or
            bool(allow_activation_by_unvalidated_spotter)
        ) and (
            not canceled_cause_of_multiactivation
        )

        self.store_event_age("spotter_end_evage")
        self._spotter_result_time = time.time()
        self.rt_log('vins.spotter_result_received', valid=is_spotter_valid)
        self.system.write_directive(Directive(
            "Spotter",
            "Validation",
            {
                "result": int(self._spotter_validation_result),
                "valid": int(is_spotter_valid),
                "canceled_cause_of_multiactivation": int(canceled_cause_of_multiactivation),
                "multiactivation_id": multiactivation_id,
                "spotted_text": spotter_text,
                "allow_activation_by_unvalidated_spotter": allow_activation_by_unvalidated_spotter,
            },
            self.event.message_id
        ))

        spotter_backend = self.streaming_backends.get("spotter")
        if spotter_backend:
            spotter_backend.close()
            self.streaming_backends.pop("spotter")
        if self._spotter_future:
            self._spotter_future.set_result(self._spotter_validation_result)
        if is_spotter_valid:
            GlobalCounter.SPOTTER_VALIDATION_OK_SUMM.increment()
        else:
            GlobalCounter.SPOTTER_VALIDATION_FAIL_SUMM.increment()
        if self._spotter_validation_result:
            self.on_end_spotter(spotter_text)
        else:
            if canceled_cause_of_multiactivation:
                self.on_vins_cancel('multiactivation')
            else:
                self.on_vins_cancel('spotter_validation_failed')
            # push logs into MDS queue
            self.system.logger.close_stream(self.event.stream_id)
            self.close()

    def on_spotter_error(self, err):
        if self._spotter_validation_result is not None:
            return  # called after timeout, - skip

        self._spotter_validation_result = True
        self.rt_log.error('spotter_result_received', error=err)
        self.system.write_directive(Directive(
            "Spotter",
            "Validation",
            {"result": 1},
            self.event.message_id
        ))
        if self._spotter_future:
            self._spotter_future.set_result(self._spotter_validation_result)
        GlobalCounter.SPOTTER_VALIDATION_ERR_SUMM.increment()
        self.ERR("Spotter check error:", err)
        self.streaming_backends["spotter"].close()
        self.streaming_backends.pop("spotter")
        self.on_end_spotter('Error occured while tried to recognize')

    def on_end_spotter(self, text=None):
        if self.score_backend():
            self.score_backend().add_chunk(last_spotter_chunk=True, text=text)

    @gen.coroutine
    def multi_activation_fast_circuit(self, spotter_backend):
        if not spotter_backend:
            return True
        need_activate = True
        try:
            need_activate = yield gen.with_timeout(
                datetime.timedelta(milliseconds=self._spotter_fast_circuit_timeout),
                spotter_backend.fast_circuit()
            )
        except (TimeoutError, gen.TimeoutError):
            pass
        return need_activate

    @gen.coroutine
    def finalize_spotter_feature(self, spotter_backend):
        if spotter_backend:
            max_wait_on_final = self._spotter_timeout - self._spotter_fast_circuit_timeout
            yield spotter_backend.final(max_wait_on_final)
        rval = yield self._spotter_future
        return rval

    @gen.coroutine
    def get_spotter_validation_result(self):
        if self._spotter_validation_result is None:
            if self._spotter_future is None:
                # spotter validation was not used
                self._spotter_validation_result = True
            else:  # wait result
                self.store_event_age(events.EventGetSpotterValidationResultStart.NAME)
                try:
                    spotter_backend = self.streaming_backends.get('spotter')
                    self._spotter_validation_result = yield gen.with_timeout(
                        datetime.timedelta(milliseconds=self._spotter_timeout),
                        self.finalize_spotter_feature(spotter_backend)
                    )
                except (TimeoutError, gen.TimeoutError):
                    GlobalCounter.SPOTTER_VALIDATION_TIMEOUT_SUMM.increment()
                    if self.system:
                        self.system.logger.log_directive(
                            {
                                "type": "SpotterValidationResult",
                                "ForEvent": self.event.message_id,
                                "result": "Timeout",
                            },
                            rt_log=self.rt_log,
                        )
                    need_activate = yield self.multi_activation_fast_circuit(spotter_backend)
                    self.on_spotter_result(need_activate)
                self.store_event_age(events.EventGetSpotterValidationResultEnd.NAME)
        return self._spotter_validation_result

    def on_yabio_result(self, result, processed_chunks):
        partial_number = self.partials_numbers['classification']
        self.partials_numbers['classification'] += 1
        self.store_event_age("yabio_end_evage")
        self.rt_log('yabio_result_received', data_size=self.data_size, type='classify')
        self.yabio_logger.end()
        result_like_scores = {}
        result_like_scores['partial_number'] = partial_number
        if result and result.get('status', '') == 'ok':
            result_like_scores['scores'] = result['bioResult']
            result_like_scores['simple'] = result['classification_results']
        else:
            result = {}
        self.vins_adapter.set_classify_result(result_like_scores, processed_chunks)
        try:
            result['partial_number'] = partial_number
            directive = Directive("Biometry", "Classification", result, self.event.message_id)
            self.system.write_directive(directive)
            self.system.write_directive(
                Directive(
                    'Uniproxy2',
                    'AnaLogInfo',
                    directive.create_message(self.system),
                    event_id=self.event.message_id
                ),
                log_message=False
            )
        except ReferenceError:
            pass

    def on_yabio_error(self, error):
        self.rt_log.error('yabio_result_received',
                          type='classify',
                          data_size=self.data_size,
                          error=error)
        self.vins_adapter.set_classify_result(None, None)
        self.ERR(error)

    def on_yabio_close(self):
        self.dec_ref_count()

    def reply_timedout(self):  # overload Event
        return (time.time() - self.create_ts) > 20

    def close(self):
        try:
            for backend in self.streaming_backends.values():
                backend.close()
            self.streaming_backends = {}
        except Exception as exc:
            self.EXC('VoiceInput.close() fail: {}'.format(exc))
        super().close()

    def finalize_asr_and_all_streaming_backends(self, text):
        self.DLOG('finalize asr and all streaming backend')
        if self.full_logger:
            self.full_logger.end(code=200, size=self.data_size)
            self.full_logger = None
        for name, backend in self.streaming_backends.items():
            if name == 'asr':
                backend.close()
            elif name == 'yabio' or name == 'score':
                backend.add_chunk(last_chunk=True, text=text)
            else:
                backend.add_chunk()
            if name == "score":
                self.yabio_scoring_logger = AccessLogger("yabio_score", self.system,
                                                         event_id=self.event.message_id,
                                                         rt_log=self.rt_log)
        try:
            self.system.close_stream(self.event.stream_id)
        except ReferenceError:
            pass
        self.rt_log('vins.asr_client_stream_closed', data_size=self.data_size)

    def _can_use_contacts(self):
        return (
            any(self._conducting_experiment(e) for e in ['contact_asr_help', 'contact_asr_only_save_data'])
            and not self.system.contacts.is_empty()
            and self.system.get_oauth_token()
            and ('dialog-general-gpu' in self.payload_with_session_data.get('topic', ''))
        )

    def _get_contacts_context_future(self, device_id):
        if self.system.contacts.is_undefined():
            future = self._get_contacts_and_make_context(device_id)
        else:
            future = Future()
            if self._conducting_experiment('contact_asr_help'):
                future.set_result(self.system.contacts.make_context())
            else:
                future.set_result(None)
        return future

    @gen.coroutine
    def _get_contacts_and_make_context(self, device_id):
        try:
            contacts = yield get_contacts(device_id, self.system.uid, rt_log=self.rt_log)
        except Exception as err:
            self.ERR('Contacts: request error', err)
            raise
        else:
            if contacts:
                self.INFO('Contacts: got {} contacts'.format(len(contacts)))
                self.system.contacts.set_contacts(contacts)
                if self._conducting_experiment('contact_asr_help'):
                    return self.system.contacts.make_context()
                return None
            else:
                self.WARN('Contacts: got no contacts')
                self.system.contacts.set_empty()
                return None

    def _patch_vins_response_with_contact_data(self, vins_response):
        if self._conducting_experiment('find_contacts_view_data'):
            try:
                directives = vins_response['response']['directives']
                if not directives or directives[0]['name'] != 'find_contacts':
                    return
                blocks = vins_response['response']['cards'][0]['body']['states'][1]['blocks']
                blocks.insert(0, {
                    'columns': [
                        {'left_padding': 'xs', 'right_padding': 'xs', 'weight': 0},
                        {'left_padding': 'xs', 'right_padding': 'xs', 'weight': 0}
                    ],
                    'rows': [
                        {
                            'bottom_padding': 'xs',
                            'cells': [
                                {'text': '# of contacts', 'text_style': 'text_m'},
                                {'text': str(self.system.contacts.number), 'text_style': 'text_m'}
                            ],
                            'top_padding': 'xs',
                            'type': 'row_element'
                        },
                    ],
                    'type': 'div-table-block'
                })
                blocks.insert(1, {'has_delimiter': 1, 'size': 'xs', 'type': 'div-separator-block'})
            except Exception as err:
                self.WARN('Contacts: fail to patch VinsResponse view data:', err)

    def create_asr(self, **kwargs):
        self.store_event_age("asr_start_evage")
        topic = self.payload_with_session_data.get("topic", "dialogeneral")

        self.first_logger = AccessLogger("asr_first_chunk", self.system, rt_log=self.rt_log)
        self.full_logger = AccessLogger("asr_full", self.system, rt_log=self.rt_log)
        self.first_logger.start(event_id=self.event.message_id, resource=topic)
        self.full_logger.start(event_id=self.event.message_id, resource=topic)

        if self._can_use_contacts():
            app_device_id = get_by_path(self.payload_with_session_data, 'application', 'device_id')
            self.context_futures['contacts'] = self._get_contacts_context_future(app_device_id)

        unistat_counter = 'asr' if topic.endswith('gpu') else 'yaldi'
        # need wait futures or not. "Hint" feature work only with *-gpu
        asr_context_futures = self.context_futures.values if topic.endswith('gpu') else []

        contacts_future = self._contacts_future if self._conducting_experiment("use_contacts_asr") else None

        return get_yaldi_stream_type(topic)(
            self.on_asr_result,
            self.on_asr_error,
            self.payload_with_session_data,
            self.system.session_id,
            self.event.message_id,
            host=self.system.srcrwr['ASR'],
            port=self.system.srcrwr.ports.get('ASR'),
            unistat_counter=unistat_counter,
            rt_log=self.rt_log,
            rt_log_label='asr',
            context_futures=asr_context_futures,
            contacts_future=contacts_future,
            system=self.system,
            error_ex_cb=self.on_asr_error_ex,
        )

    def create_duplicate_asr(self, **kwargs):
        duplicate_asr_params = self.payload_with_session_data.get('duplicate_asr')
        return YaldiStream(
            self.on_duplicate_asr_result,
            self.on_duplicate_asr_error,
            deepupdate(self.payload_with_session_data, {
                "topic": duplicate_asr_params.get("topic", "dialog-general-gpu"),
            }),
            self.system.session_id,
            self.event.message_id,
            host=duplicate_asr_params.get("host"),
            unistat_counter='dup_yaldi',
            rt_log=self.rt_log,
            rt_log_label='duplicate_asr',
            system=self.system,
        )

    def create_scoring(self, spotter=False, **kwargs):
        return YabioStream(
            YabioStream.YabioStreamType.Score,
            self.on_scoring_result,
            self.on_scoring_error,
            self.payload_with_session_data,
            host=self.system.srcrwr['YABIO'],
            session_id=self.system.session_id,
            spotter=spotter,
            rt_log=self.rt_log,
            rt_log_label='yabio_score',
            system=self.system,
            message_id=self.event.message_id,
            stream_id=self.event.stream_id,
        )

    def create_spotter(self, **kwargs):
        return SpotterStream(
            self.on_spotter_result,
            self.on_spotter_error,
            self.payload_with_session_data,
            self.system.session_id,
            self.event.message_id,
            system=self.system,
            rt_log=self.rt_log,
            rt_log_label='spotter',
            smart_activation=self._smart_activation,
            apphosted_asr=self._apphosted_asr,
        )

    def create_yabio(self, **kwargs):
        yabio = YabioStream(
            YabioStream.YabioStreamType.Classify,
            self.on_yabio_result,
            self.on_yabio_error,
            self.payload_with_session_data,
            close_callback=self.on_yabio_close,
            host=self.system.srcrwr['YABIO'],
            session_id=self.system.session_id,
            rt_log=self.rt_log,
            rt_log_label='yabio_classify',
            system=self.system,
            message_id=self.event.message_id,
            stream_id=self.event.stream_id,
        )
        self.inc_ref_count()
        return yabio

    def _make_rerequest_counter(self):
        def get_signal_name_template():
            if self.system.is_quasar:
                return 'vins_rerequest_quasar_user'
            else:
                return 'vins_rerequest_other_user'

        return ActionCounter(get_signal_name_template())


@register_event_processor
class MusicInput(VinsProcessor):
    def __init__(self, *args, **kwargs):
        super(MusicInput, self).__init__(*args, **kwargs)
        self.streams = []
        self.event = None
        self.data_size = 0

    def close(self):
        try:
            for s in self.streams:
                s.close()
        except Exception:
            self.EXC('MusicInput.close() fail')
        self.streams = []
        super(MusicInput, self).close()

    def close_stream(self, streamType):
        streams = []
        for stream in self.streams:
            if isinstance(stream, streamType):
                stream.close()
            else:
                streams.append(stream)
        self.streams = streams

    def process_streamcontrol(self, _):
        for s in self.streams:
            s.close()
        return EventProcessor.StreamControlAction.Close

    def add_data(self, data):
        self.data_size += len(data)
        for s in self.streams:
            s.add_chunk(data)

    def on_result_music(self, result, finish=True):
        if self.is_closed():
            return

        self.write_directive("MusicResult", result)
        if finish:
            self.vins_adapter.set_music_result(result=result)
            self.close_stream(MusicStream2)

    def on_error_music(self, err):
        if self.is_closed():
            return

        self.dispatch_error(err, close=False)
        self.vins_adapter.set_music_result(error=err)
        self.close_stream(MusicStream2)

    def write_directive(self, name, result):
        self.system.write_directive(Directive("ASR", name, result, self.event.message_id))

    def process_event(self, event):
        super(MusicInput, self).process_event(event)
        if 'music_request2' not in self.payload_with_session_data:
            self.payload_with_session_data['music_request2'] = {}
        self.oauth_token = self.system.get_oauth_token()
        if not self.oauth_token:
            self.oauth_token = self.event.payload.get('request', {}).get('additional_options', {}).get('oauth_token')
        if get_by_path(self.payload_with_session_data, 'request', 'megamind_cookies'):
            self.complete_processing(need_buffer=True)
            self.start_uaas()
            return

        self.complete_processing()

    def after_uaas(self):
        for stream in self.streams:
            stream.init_back()

    def complete_processing(self, need_buffer=False):
        self.streams.append(ProxyBackend(self.create_music_stream, need_buffer))

    def create_music_stream(self):
        music_stream = MusicStream2(
            self.on_result_music,
            self.on_error_music,
            self.payload_with_session_data,
            uuid=self.system.session_data.get('uuid'),
            api_key=self.system.session_data.get('apiKey'),
            oauth_token=self.oauth_token,
            client_ip=self.system.client_ip,
            client_port=self.system.client_port,
            rt_log=self.rt_log,
            rt_log_label='music'
        )
        return music_stream


class DirectRequest(VinsProcessor):
    def __init__(self, *args, **kwargs):
        super(DirectRequest, self).__init__(*args, **kwargs)
        self.futures = {}

    def process_event(self, event):
        super(DirectRequest, self).process_event(event)

        if self._can_use_smart_home():
            self.add_smart_home_future()

        if get_by_path(self.payload_with_session_data, 'request', 'megamind_cookies'):
            self.add_uaas_future()
            self.start_uaas()
        # forking coroutine
        self.process_event_with_futures()

    def add_smart_home_future(self):
        self.futures['smart_home'] = gen.with_timeout(datetime.timedelta(
            milliseconds=1000), self._get_smart_home_and_set_result())

    def add_uaas_future(self):
        self.futures['uaas'] = Future()

    @gen.coroutine
    def process_event_with_futures(self):
        if len(self.futures) != 0:
            try:
                yield self.futures
                self.INFO('Futures success')
            except (TimeoutError, gen.TimeoutError):
                self.EXC('Futures timed out')
            except Exception as exc:
                self.EXC('Future error: {}'.format(exc))
        else:
            self.INFO('Can\'t use additional data: no futures set')
        self.complete_processing()

    def complete_processing(self):
        self.vins_adapter.set_text_result()

    def after_uaas(self):
        try:
            self.futures['uaas'].set_result(True)
        except:
            # future is already set
            pass


@register_event_processor
class TextInput(DirectRequest):
    pass


@register_event_processor
class CustomInput(TextInput):
    pass
