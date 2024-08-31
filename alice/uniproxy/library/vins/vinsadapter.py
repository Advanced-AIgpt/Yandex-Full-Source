# -*- coding: utf-8
import alice.uniproxy.library.utils.deepcopyx as copy
import time
from weakref import WeakMethod
from tornado import gen
from .vinsrequest import VinsApplyRequest, VinsRequest

import alice.uniproxy.library.perf_tester.events as events
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import conducting_experiment
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import YALDI_PARTIAL_HGRAM, YABIO_PARTIAL_HGRAM, CLASSIFY_PARTIAL_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import VINS_RUN_DELAY_AFTER_EOU_HGRAM, VINS_RUN_WAIT_AFTER_EOU_HGRAM


CHILDREN_BIOMETRY_HANDLE = '/v1/personality/profile/alisa/kv/alice_children_biometry'


class VinsTimings(object):
    def __init__(self, request_start_time_sec, epoch, is_quasar, vins_partial, unistat):
        self.request_start_time_sec = request_start_time_sec
        self._is_quasar = is_quasar
        self._vins_partial = vins_partial
        self._unistat = unistat
        self.useful_partial = False  # has vins request for partial with same text as asr-eou results, so not make eou request to vins
        self.useful_vins_response_before_eou = False
        self._vins_requests = []
        self._request_timings = None  # timings for request to Vins/MM which response was used for vins_response_callback
        self._vins_preparing_requests = []
        self._dict = {
            events.EventEpoch.NAME: int(epoch),
            events.EventHasApplyVinsRequest.NAME: False
        }

    @property
    def request_timings(self):
        return self._request_timings

    def import_events(self, d):
        for k, v in d.items():
            if k in events.BY_NAME:
                self._dict[k] = v

    def on_event(self, event, timestamp_sec=None):
        """
        Stores time delta for event since request start time.

        Parameters:

        event:
            Uniproxy perf event, must be selected from the list of
            perf tester events.

        timestamp_sec: float
            Timestamp of event, measured in seconds. When None,
            current timestamp from some steady clock will be used.

        Returns:

        Timedelta measured in seconds since last event of the same
        type, or timedelta since request processing if this is the
        first such event.
        """
        name = event.NAME

        if timestamp_sec is None:
            timestamp_sec = time.monotonic()
        delta_sec = timestamp_sec - self.request_start_time_sec
        passed_sec = delta_sec
        if name in self._dict:
            passed_sec = delta_sec - self._dict[name]
        self._dict[name] = delta_sec
        return passed_sec

    def on_event_with_value(self, event, value):
        """
        Stores value for event.

        Parameters:

        event: str
            Uniproxy perf event, must be selected from the list of
            perf tester events.

        value: any
            Value of event.

        Returns: None
        """
        self._dict[event.NAME] = value

    def on_request(self, start_time_sec, end_time_sec):
        """
        Stores time delta for vins request.

        Parameters:

        start_time_sec: float
            Timestamp of request start, measured in seconds.

        end_time_sec: float
            Timestamp of request end, measured in seconds.

        Returns: None
        """
        try:
            self._vins_requests.append(end_time_sec - start_time_sec)
        except:
            return
        self.on_last_request(start_time_sec, end_time_sec)

    def on_request_prepared(self, start_time_sec, end_times_sec):
        try:
            delta_sec = end_times_sec - start_time_sec
            self._vins_preparing_requests.append(delta_sec)
        except:
            return

        self._dict[events.EventLastVinsPreparingRequestDurationSec.NAME] = delta_sec

    def on_last_request(self, start_time_sec, end_time_sec):
        eou_time = self._dict.get(events.EventEndOfUtterance.NAME)
        has_wait_after_eou = eou_time is not None
        try:
            delta_sec = end_time_sec - start_time_sec
            if has_wait_after_eou:
                wait_time = end_time_sec - self.request_start_time_sec - eou_time
        except:
            return
        self._dict[events.EventLastVinsRunRequestDurationSec.NAME] = delta_sec

        if self._vins_partial:
            if has_wait_after_eou:
                self._dict[events.EventHasVinsFullResultOnEOU.NAME] = False
                self._dict[events.EventVinsRunWaitAfterEOUDurationSec.NAME] = wait_time
                self._dict[events.EventVinsRunDelayAfterEOUDurationSec.NAME] = wait_time
                self._dict[events.EventVinsWaitAfterEOUDurationSec.NAME] = wait_time
            else:
                self._dict[events.EventHasVinsFullResultOnEOU.NAME] = True
                self._dict[events.EventVinsRunDelayAfterEOUDurationSec.NAME] = 0

        self._update_full_last_request()

    def on_apply_request(self, start_time_sec, end_time_sec):
        eou_time = self._dict.get(events.EventEndOfUtterance.NAME, 0)
        wait_time = end_time_sec - self.request_start_time_sec - eou_time
        self._dict[events.EventVinsWaitAfterEOUDurationSec.NAME] = wait_time
        try:
            delta_sec = end_time_sec - start_time_sec
        except:
            return
        self._dict[events.EventLastVinsApplyRequestDurationSec.NAME] = delta_sec
        self._dict[events.EventHasApplyVinsRequest.NAME] = True
        if self._vins_partial:
            self._dict[events.EventHasVinsFullResultOnEOU.NAME] = False
        self._update_full_last_request()

    def _update_full_last_request(self):
        d = self._dict
        r = d.get(events.EventLastVinsRunRequestDurationSec.NAME, 0)
        a = d.get(events.EventLastVinsApplyRequestDurationSec.NAME, 0)
        f = r + a
        d[events.EventLastVinsFullRequestDurationSec.NAME] = f

    def get_event(self, event):
        return self._dict.get(event.NAME)

    def to_dict(self):
        request_events = {
            events.EventVinsRequestCount.NAME: len(self._vins_requests),
        }
        if self._vins_requests:
            mean = sum(self._vins_requests) / len(self._vins_requests)
            request_events[events.EventMeanVinsRequestDurationSec.NAME] = mean
        if self._vins_preparing_requests:
            mean = sum(self._vins_preparing_requests) / len(self._vins_preparing_requests)
            request_events[events.EventMeanVinsPreparingRequestDurationSec.NAME] = mean
        if self._request_timings:
            for k, v in self._request_timings.prepare_request.items():
                request_events[k] = v
        eou_time = self._dict.get(events.EventEndOfUtterance.NAME)
        if self._request_timings:
            for k, v in self._request_timings.prepare_request.items():
                request_events[k] = v
            if self.useful_partial and eou_time is not None:
                request_events[events.EventUsefulPartial.NAME] = self._request_timings.start_ts - \
                    self.request_start_time_sec
            if self._request_timings.begin_request_ts and self._request_timings.end_request_ts:
                request_events[events.EventUsefulVinsRequestDuration.NAME] = self._request_timings.end_request_ts - \
                    self._request_timings.begin_request_ts

        return dict(self._dict, **request_events)

    def to_directive(self, event_id, params=None):
        payload = self.to_dict()
        if params:
            payload['params'] = params

        return Directive(namespace='Vins',
                         name='UniproxyVinsTimings',
                         payload=payload,
                         event_id=event_id)

    def on_select_useful_request(self, vins_request):
        """
        store here timings request to vins choosen for response to user
        (eou response, partial used instead eou (same asr text), with force_eou flag, etc)
        """
        if vins_request.type != vins_request.RequestType.Apply:
            self._request_timings = vins_request.timings

    def to_global_counters(self):
        vins_run_delay = self._dict.get(events.EventVinsRunDelayAfterEOUDurationSec.NAME)
        if vins_run_delay is not None:
            self._unistat.store(VINS_RUN_DELAY_AFTER_EOU_HGRAM, vins_run_delay)

        vins_run_wait_time = self._dict.get(events.EventVinsRunWaitAfterEOUDurationSec.NAME)
        if vins_run_wait_time is not None:
            self._unistat.store(VINS_RUN_WAIT_AFTER_EOU_HGRAM, vins_run_wait_time)

        vins_wait_time = self._dict.get(events.EventVinsWaitAfterEOUDurationSec.NAME)
        if vins_run_wait_time is not None:
            self._unistat.store(events.EventVinsWaitAfterEOUDurationSec.NAME, vins_wait_time)

        if self.useful_vins_response_before_eou:
            self._unistat.inc_counter2('user_useful_vins_response_before_eou')
            self._unistat.inc_counter2('robot_useful_vins_response_before_eou')
        if self._request_timings:
            for k, v in self._request_timings.prepare_request.items():
                self._unistat.store(k, v)
            eou_time = self._dict.get(events.EventEndOfUtterance.NAME)
            if self.useful_partial and eou_time is not None:
                asr_end = self.request_start_time_sec + eou_time
                self._unistat.store("user_useful_partial_to_asr_end", asr_end - self._request_timings.start_ts)
                self._unistat.store("robot_useful_partial_to_asr_end", asr_end - self._request_timings.start_ts)
            if self._request_timings.begin_request_ts and self._request_timings.end_request_ts:
                self._unistat.store(events.EventUsefulVinsRequestDuration.NAME,
                                    self._request_timings.end_request_ts - self._request_timings.begin_request_ts)

        has_full_result_on_eou = self._dict.get(events.EventHasVinsFullResultOnEOU.NAME)
        if has_full_result_on_eou is not None:
            if has_full_result_on_eou:
                GlobalCounter.VINS_FULL_RESULT_READY_ON_EOU_SUMM.increment()
            else:
                GlobalCounter.VINS_FULL_RESULT_NOT_READY_ON_EOU_SUMM.increment()


class AllVinsRequests(object):
    def __init__(self):
        self._vins_requests = {}
        self._vins_apply_backend = None

    def get_requests(self):
        for request in self._vins_requests.values():
            yield request
        if self._vins_apply_backend:
            yield self._vins_apply_backend

    def num_requests(self):
        if self._vins_apply_backend:
            return len(self._vins_requests) + 1
        return len(self._vins_requests)

    def set_music_result(self, result):
        for request in self.get_requests():
            request.set_music_result(result)

    def set_yabio_result(self, result, processed_chunks=None, end_of_utterance=False):
        for request in self.get_requests():
            request.set_yabio_result(result, processed_chunks=processed_chunks, end_of_utterance=end_of_utterance)

    def set_classify_result(self, result, processed_chunks=None):
        for request in self.get_requests():
            request.set_classify_result(result, processed_chunks=processed_chunks)

    def set_vins_session_result(self, result):
        for request in self.get_requests():
            request.set_vins_session_result(result)

    def set_notification_state_result(self, result):
        for request in self.get_requests():
            request.set_notification_state_result(result)

    def set_memento_state_result(self, result):
        for request in self.get_requests():
            request.set_memento_state_result(result)

    def set_contacts_state_result(self, result):
        for request in self.get_requests():
            request.set_contacts_state_result(result)

    def set_contacts_proto_state_result(self, result):
        for request in self.get_requests():
            request.set_contacts_proto_state_result(result)

    def set_laas_result(self, result):
        for request in self.get_requests():
            request.set_laas_result(result)

    def set_smart_home_result(self, result):
        for request in self.get_requests():
            request.set_smart_home_result(result)

    def has(self, request_text):
        return request_text in self._vins_requests

    def get(self, request_text):
        return self._vins_requests.get(request_text)

    def abort(self):
        for request in self._vins_requests.values():
            request.close()
        self._vins_requests = {}
        if self._vins_apply_backend:
            self._vins_apply_backend.close()
            self._vins_apply_backend = None

    def add_vins_request(self, request_text, vins_request):
        self._vins_requests[request_text] = vins_request

    def has_vins_apply_backend(self):
        return self._vins_apply_backend is not None

    def add_vins_apply_backend(self, vins_apply_backend):
        assert not self.has_vins_apply_backend()
        self._vins_apply_backend = vins_apply_backend

    def get_vins_apply_backend(self):
        return self._vins_apply_backend


class VinsAdapter(object):
    def __init__(self,
                 system,
                 message_id,
                 payload,
                 on_vins_partial,
                 on_vins_response,
                 on_vins_cancel,
                 personal_data_helper,
                 rt_log,
                 processor,
                 event_start_time,
                 epoch,
                 fake=False,
                 has_spotter=False,
                 has_biometry=False):
        self.unistat = system.unistat
        self.all_vins_requests = AllVinsRequests()
        self.vins_response_processed = False
        self._log = Logger.get('.vins.adapter')
        self.vins_request_text = ""
        self.vins_session = None
        self.vins_session_loaded = False
        self.notification_state = None
        self.notification_state_loaded = False
        self.contacts_state = None
        self.contacts_state_loaded = False
        self.contacts_proto_state = ""
        self.contacts_proto_state_loaded = False
        self.laas_data = None
        self.laas_data_loaded = False
        self.memento_state = None
        self.memento_state_loaded = False
        self.smart_home = None
        self.smart_home_loaded = False
        self.scoring_result = None
        self.classify_result = None
        self.scoring_chunks = None
        self.classify_chunks = None
        self.end_of_utt = False
        self.vins_partial = False
        self.vins_partial_results = {}
        self.vins_request_parts = VinsRequest.RequestParts.ASR
        self.support_forced_eou = False
        self.rt_log = rt_log
        self.personal_data_getter_future = None
        self.fake = fake
        self.has_spotter = has_spotter
        self.has_biometry = has_biometry
        self.allow_deferred_apply_check = False  # MEGAMIND-1642
        self.children_biometry_disabled = False
        self.music_result = None
        self.last_asr_core_debug = None

        self.system = system
        self.message_id = message_id
        self.params = payload
        self.processor = processor

        self.delayed_vins_params = {}
        self.last_partial_number = None

        self.spotter_glue_phrase = ''
        if self.system and self.system.use_spotter_glue:
            self.spotter_glue_phrase = self.params.get('spotter_phrase', '').lower()

        self._partial_callback = WeakMethod(on_vins_partial)
        self.result_callback = WeakMethod(on_vins_response)
        self.vins_cancel_callback = WeakMethod(on_vins_cancel)

        if "vins_partial" in self.params:
            self.vins_partial = self.params["vins_partial"]
        elif self._conducting_experiment("disable_partials"):
            self.vins_partial = False
        elif self._conducting_experiment("enable_partials"):
            self.vins_partial = True
        elif "use_mm_partials" in self.params.get("settings_from_manager", {}):
            self.vins_partial = self.params["settings_from_manager"]["use_mm_partials"]

        self.vins_timings = VinsTimings(
            request_start_time_sec=event_start_time,
            epoch=epoch,
            vins_partial=self.vins_partial,
            is_quasar=self.system.is_quasar,
            unistat=self.unistat,
        )

        if "force_eou" in self.params:
            self.support_forced_eou = self.params["force_eou"]
        if self._conducting_experiment("force_eou"):
            self.support_forced_eou = True
        if not self._conducting_experiment("no_datasync_uniproxy"):
            self.personal_data_getter_future = self._get_personal_data_and_update_params(personal_data_helper)

        self.yaldi_partials = {}
        self.e2e_partials = {}

        self._form_dialog_id()
        self._is_silent = self._conducting_experiment("silent_vins")
        self._emotion_hack = self._conducting_experiment("tts_emotion_hack")
        self.whisper = None

    def _form_dialog_id(self):
        """
        Adds "Dialogovo:" prefix to dialog_id (if exists) for old dialogs according to backward compatibility.
        Previously only external skills were supposed to manage tabs, now any scenario has such opportunity.
        Megamind adds scenario name as prefix to determine a host of dialog_id
        We have to add scenario name to old dialog_id (without scenario name prefix) according to the Megamind protocol.
        :return: None
        """
        if not self.params.get("header", {}).get("dialog_id"):
            return
        dialog_id = self.params["header"]["dialog_id"]
        if ":" in dialog_id:
            return
        skills = config.get("megamind", {}).get("dialogovo_skills", {})
        # TODO(@alkapov): remove this condition (see: MEGAMIND-504)
        if self._conducting_experiment("dialogovo_enable_internal_skills") and dialog_id in skills:
            self.params["header"]["dialog_id"] = "Dialogovo:" + self.params["header"]["dialog_id"]

    @gen.coroutine
    def _get_personal_data_and_update_params(self, personal_data_helper):
        if personal_data_helper is None:
            return

        try:
            self.processor.store_event_age(events.EventVinsPersonalDataStart.NAME)
            personal_data, _ = yield personal_data_helper.get_personal_data()
            if CHILDREN_BIOMETRY_HANDLE in personal_data:
                value = personal_data[CHILDREN_BIOMETRY_HANDLE]
                if value == 'disabled':
                    self.children_biometry_disabled = True
                    # here we can want to filter old classify result
                    self.set_classify_result(None, None)
                elif value != 'enabled':
                    self.WARN('incorrect value of childreh_biometry: {}.'.format(value))
        except Exception as err:
            self.ERR('failed to get personal data:', err)
            raise
        else:
            self.params = copy.deepcopy(self.params)
            self.params['request']['personal_data'] = personal_data
        finally:
            self.processor.store_event_age(events.EventVinsPersonalDataEnd.NAME)

    def add_extra_experiments(self, exps):
        if exps is None:
            return
        if self.params['request'].get('experiments') is None:
            self.params['request']['experiments'] = dict()
        self.params['request']['experiments'].update(exps)

    def _conducting_experiment(self, experiment):
        return conducting_experiment(experiment, self.params, self.system.uaas_flags)

    def set_text_result(self):
        self.vins_partial = False
        self.set_allow_deferred_apply_check()
        self.__start_vins_request__(0)

    def set_music_result(self, result=None, error=None):
        self.vins_partial = False
        if result is None:
            result = {
                "result": "error",
                "error_text": str(error),
            }

        self.music_result = result
        self.set_allow_deferred_apply_check()
        self.__start_vins_request__(VinsRequest.RequestParts.MUSIC)

        if self.all_vins_requests.num_requests() != 1:
            self.WARN("set_music_result with multiple/zero vins request")

        self.all_vins_requests.set_music_result(result)

    def set_scoring_result(self, result, processed_chunks):
        passed = self.vins_timings.on_event(event=events.EventLastScorePartial)
        GlobalTimings.store(YABIO_PARTIAL_HGRAM, passed)

        if not result:  # None mean error from yabio, try use last result instead
            result = self.scoring_result
            if not result:
                result = {"status": "ok", "scores": []}
        else:
            self.scoring_result = result
            self.scoring_result["status"] = "ok"
            self.scoring_chunks = processed_chunks

        if self.all_vins_requests.num_requests() == 0:
            return

        # if processed_chunks is None - we has yabio error, so useless continue waiting
        try:
            self.all_vins_requests.set_yabio_result(result, processed_chunks, end_of_utterance=self.end_of_utt)
        except Exception as exc:
            self.WARN("No need in yabio result", exc)

    def filter_classify_result(self, result):
        filtered_result = []
        for item in result['simple']:
            if item.get('tag') != 'children':
                filtered_result.append(item)
        result['simple'] = filtered_result
        return result

    def set_classify_result(self, result, processed_chunks):
        passed = self.vins_timings.on_event(
            event=events.EventLastClassifyPartial)
        GlobalTimings.store(CLASSIFY_PARTIAL_HGRAM, passed)

        if not result:  # None mean error from yabio, try use last result instead
            if not self.classify_result:
                result = {"status": "ok", "scores": []}
            else:
                if self.children_biometry_disabled:
                    self.classify_result = self.filter_classify_result(self.classify_result)
                result = self.classify_result
        else:
            if self.children_biometry_disabled:
                result = self.filter_classify_result(result)
            self.classify_result = result
            self.classify_result["status"] = "ok"
            self.classify_chunks = processed_chunks

        if self.all_vins_requests.num_requests() == 0:
            return

        # if processed_chunks is None - we has yabio error, so useless continue waiting
        try:
            self.all_vins_requests.set_classify_result(result, processed_chunks)
        except Exception as exc:
            self.WARN("No need in classification result", exc)

    def set_asr_result(self, _result):
        asr_result_ts = time.monotonic()
        result = copy.deepcopy(_result)
        if result.get('coreDebug'):
            self.last_asr_core_debug = result.pop('coreDebug', None)

        asr_partial_number = result['asr_partial_number']
        is_whisper = result.get('whisperInfo', {}).get('isWhisper')
        if is_whisper:
            self.whisper = True
        # self.DLOG("Vins.VoiceRequest {}".format(partial_changed))
        if result.get("endOfUtt", False):
            self.__on_end_of_utt__(result)
        elif self.vins_partial:
            passed = self.vins_timings.on_event(event=events.EventLastPartial)
            GlobalTimings.store(YALDI_PARTIAL_HGRAM, passed)
            (text_result, asr_result) = self.__update_asr_result_to_vins_format__(result)

            if text_result and not self.all_vins_requests.has(text_result):
                if "e2e_recognition" in result:
                    self.e2e_partials[text_result] = time.monotonic()
                else:
                    self.yaldi_partials[text_result] = time.monotonic()

                self.DLOG("Vpcase0: Partial changed, make request to vins {}".format(text_result))
                self.rt_log('vins.partial_changed', text_result=text_result)
                self.vins_request_text = text_result
                parts = self.get_request_parts()
                self.__start_vins_request__(
                    parts,
                    asr_result=asr_result,
                    end_of_utterance=False,
                    is_whisper=is_whisper,
                    request_text=text_result,
                    start_ts=asr_result_ts,
                    asr_partial_number=asr_partial_number,
                )

    def request_scores_if_needed(self, parts, vins_request):
        if parts & VinsRequest.RequestParts.YABIO:
            stream = self.processor.score_backend()
            if stream:
                self.request_score(stream, 'yabio', vins_request)

        if parts & VinsRequest.RequestParts.CLASSIFY:
            stream = self.processor.classify_backend()
            if stream:
                self.request_score(stream, 'classify', vins_request)

    def get_request_parts(self):
        parts = self.vins_request_parts
        try:
            if self.has_biometry and self.params.get("need_scoring"):
                yabiostream = self.processor.score_backend()
                if yabiostream and not yabiostream._closed and not (
                        yabiostream.last_chunk and yabiostream.last_result_is_actual()):
                    parts |= VinsRequest.RequestParts.YABIO

            if self.has_biometry and self.params.get("biometry_classify"):
                stream = self.processor.classify_backend()
                if stream and not stream._closed and not (
                        stream.last_chunk and stream.last_result_is_actual()):
                    parts |= VinsRequest.RequestParts.CLASSIFY
        except Exception as ex:
            # closed, but still exists bio backends can cause exceptions like:
            # AttributeError: 'BufferBackend' object has no attribute 'last_chunk'
            self.ERR("fail get (bio)request parts: ", str(ex))
        return parts

    def set_vins_session(self, session):
        self.vins_session = session
        self.vins_session_loaded = True
        self.all_vins_requests.set_vins_session_result(self.vins_session)

    def set_notification_state(self, notification_state):
        self.notification_state = notification_state
        self.notification_state_loaded = True
        self.all_vins_requests.set_notification_state_result(self.notification_state)

    def set_memento_state(self, memento_state):
        self.memento_state = memento_state
        self.memento_state_loaded = True
        self.all_vins_requests.set_memento_state_result(self.memento_state)

    def set_contacts_state(self, contacts_state):
        self.contacts_state = contacts_state
        self.contacts_state_loaded = True
        self.all_vins_requests.set_contacts_state_result(self.contacts_state)

    def set_contacts_proto_state(self, contacts_state):
        self.contacts_proto_state = contacts_state
        self.contacts_proto_state_loaded = True
        self.all_vins_requests.set_contacts_proto_state_result(self.contacts_proto_state)

    def set_laas_result(self, laas_data):
        self.laas_data = laas_data
        self.laas_data_loaded = True
        self.all_vins_requests.set_laas_result(self.laas_data)

    def set_smart_home_result(self, result, raw_data):
        self.params = copy.deepcopy(self.params)
        self.params['request']['smart_home'] = raw_data
        self.params['iot_user_info_data'] = result

    def set_smart_home_future(self, fut):
        self.vins_request_parts |= VinsRequest.RequestParts.SMART_HOME

        def on_done(fut):
            try:
                self.smart_home = fut.result()
                self.smart_home_loaded = True
                self.all_vins_requests.set_smart_home_result(self.smart_home)
            except Exception as exc:
                self.ERR('Failed to get result of smart_home future. Exception: {}'.format(str(exc)))

        fut.add_done_callback(on_done)

    def set_allow_deferred_apply_check(self):
        self.allow_deferred_apply_check = True

    def get_partial_hearts(self, text_result):
        if self.vins_partial:
            if self.all_vins_requests.has(text_result):
                if text_result not in self.vins_partial_results:
                    return "\U0001F495"
                elif self.vins_partial_results[text_result] is None:
                    return "\U0001F49B"
                else:
                    return "♥"
            else:
                return "\U0001F494"
        return ""

    def close(self):
        self._partial_callback = None
        self.result_callback = None
        self.__abort_vins_requests__()

    def __on_vins_partial__(self, *args, **kwargs):
        if self._partial_callback:
            if self._is_silent:
                kwargs.pop("what_to_say", None)
            callback = self._partial_callback()
            callback(*args, **kwargs)

    def __on_vins_response__(self, *args, **kwargs):
        self.vins_timings.on_event(event=events.EventVinsResponse)
        if self.vins_timings.request_timings:
            request_timings = self.vins_timings.request_timings
            self.processor.store_event_age("useful_asr_result_evage", request_timings.start_ts)
            if request_timings.begin_request_ts:
                self.processor.store_event_age(events.EventUsefulVinsRequest.NAME, request_timings.begin_request_ts)
        if self.result_callback:
            kwargs['uniproxy_vins_timings'] = self.vins_timings
            if self._is_silent:
                kwargs.pop("what_to_say", None)
            callback = self.result_callback()
            callback(*args, **kwargs)

    def __on_vins_cancel__(self, *args, **kwargs):
        if self.vins_cancel_callback:
            callback = self.vins_cancel_callback()
            callback(*args, **kwargs)

    def __abort_vins_requests__(self):
        self.all_vins_requests.abort()

    @gen.coroutine
    def __start_vins_apply_request(self, vins_request, session, request_text, asr_result):
        if self.has_spotter:
            spotter_ok = yield self.processor.get_spotter_validation_result()
            if not spotter_ok:
                self.processor.dec_ref_count()  # << replace on_vins_response() call
                return  # spotter validation failed, skip apply_request

        self.processor.store_event_age("vins_apply_request_evage")
        vins_apply_backend = VinsApplyRequest(
            vins_request,
            self.system,
            self.message_id,
            self.params,
            self.__on_vins_result__,
            self.__on_vins_error__,
            self.__on_vins_cancel__,
            self.rt_log,
            self.fake,
            self.vins_timings,
            None,  # request_fallback_parts
            None,  # start_ts
            self.processor._get_user_ticket,
        )

        self.all_vins_requests.add_vins_apply_backend(vins_apply_backend)

        self.DLOG('Starting VINS apply request')
        parts = self.get_request_parts()

        vins_apply_backend.start_request(parts)

        vins_apply_backend.set_vins_session_result(session)

        if self.notification_state_loaded:
            vins_apply_backend.set_notification_state_result(self.notification_state)

        if self.memento_state_loaded:
            vins_apply_backend.set_memento_state_result(self.memento_state)

        if self.contacts_state_loaded:
            vins_apply_backend.set_contacts_state_result(self.contacts_state)
        if self.contacts_proto_state_loaded:
            vins_apply_backend.set_contacts_proto_state_result(self.contacts_proto_state)

        if self.laas_data_loaded:
            vins_apply_backend.set_laas_result(self.laas_data)

        if self.smart_home_loaded:
            vins_apply_backend.set_smart_home_result(self.smart_home)

        # asr_result can be None in case of text-only input and it's fine.
        vins_apply_backend.set_asr_result(
            asr_result,
            None,
            end_of_utterance=True,
            is_whisper=self.whisper,
            request_text=request_text,
            core_debug=self.last_asr_core_debug,
        )

        vins_apply_backend.set_music_result(self.music_result)

        self.request_scores_if_needed(parts, vins_apply_backend)

    def __start_vins_request__(self, parts, request_text="", asr_result=None, end_of_utterance=False, is_whisper=False, start_ts=None, asr_partial_number=None):
        if self.personal_data_getter_future:
            self.__save_vins_params__(
                parts,
                request_text,
                asr_result,
                end_of_utterance,
                is_whisper,
                start_ts=start_ts,
                asr_partial_number=asr_partial_number)
            self.personal_data_getter_future.add_done_callback(
                lambda _: self.__on_start_vins_request_wrapper__())
        else:
            return self.__on_start_vins_request__(
                parts,
                request_text,
                asr_result,
                end_of_utterance,
                is_whisper,
                start_ts=start_ts,
                asr_partial_number=asr_partial_number)

    def __on_start_vins_request_wrapper__(self):
        self.__on_start_vins_request__(
            self.delayed_vins_params.get('parts'),
            self.delayed_vins_params.get('request_text'),
            self.delayed_vins_params.get('asr_result'),
            self.delayed_vins_params.get('end_of_utterance'),
            self.delayed_vins_params.get('is_whisper'),
            start_ts=self.delayed_vins_params.get('start_ts'),
            asr_partial_number=self.delayed_vins_params.get('asr_partial_number'))

    def __save_vins_params__(self, parts, request_text, asr_result, end_of_utterance, is_whisper, asr_partial_number, start_ts=None):
        current_partial_number = self.delayed_vins_params.get('asr_partial_number')
        if current_partial_number is None or (asr_partial_number > current_partial_number and request_text):
            self.delayed_vins_params['parts'] = parts
            self.delayed_vins_params['request_text'] = request_text
            self.delayed_vins_params['asr_result'] = asr_result
            self.delayed_vins_params['end_of_utterance'] = end_of_utterance
            self.delayed_vins_params['asr_partial_number'] = asr_partial_number
            self.delayed_vins_params['start_ts'] = start_ts
            self.delayed_vins_params['is_whisper'] = is_whisper

    def __on_start_vins_request__(self, parts, request_text, asr_result, end_of_utterance, is_whisper, asr_partial_number, start_ts=None):
        if self.last_partial_number and asr_partial_number <= self.last_partial_number:
            return

        self.last_partial_number = asr_partial_number

        self.vins_request_parts = parts

        if end_of_utterance:
            self.processor.store_event_age("vins_request_eou_evage")
        request_fallback_parts = {}
        if self.classify_result:
            request_fallback_parts[VinsRequest.RequestParts.CLASSIFY] = self.classify_result
        if self.scoring_result:
            request_fallback_parts[VinsRequest.RequestParts.YABIO] = self.scoring_result

        request = VinsRequest(
            self.system,
            self.message_id,
            self.params,
            self.__on_vins_result__,
            self.__on_vins_error__,
            self.__on_vins_cancel__,
            self.rt_log,
            fake=self.fake,
            vins_timings=self.vins_timings,
            request_fallback_parts=request_fallback_parts,
            start_ts=start_ts,
            get_user_ticket=self.processor._get_user_ticket,
        )

        self.all_vins_requests.add_vins_request(request_text, request)
        request.start_request(parts)
        self.request_scores_if_needed(parts, request)

        if self.vins_session_loaded:
            request.set_vins_session_result(self.vins_session)

        if self.notification_state_loaded:
            request.set_notification_state_result(self.notification_state)

        if self.contacts_state_loaded:
            request.set_contacts_state_result(self.contacts_state)
        if self.contacts_proto_state_loaded:
            request.set_contacts_proto_state_result(self.contacts_proto_state)

        if self.laas_data_loaded:
            request.set_laas_result(self.laas_data)

        if self.memento_state_loaded:
            request.set_memento_state_result(self.memento_state)

        if self.smart_home_loaded:
            request.set_smart_home_result(self.smart_home)

        if asr_result is not None:
            request.set_asr_result(asr_result, asr_partial_number, end_of_utterance, is_whisper, request_text, self.last_asr_core_debug)
        return request

    def __on_vins_error__(self, message):
        self.ERR("Bad vins response:", message)
        self.__on_vins_response__(error="Bad vins response: %s" % (message))
        self.close()

    def __on_vins_result__(self,
                           vins_request,
                           vins_response,
                           request_text,
                           asr_result=None,
                           eou=False,
                           force_eou=False):
        assert vins_request

        if vins_request.type == VinsRequest.RequestType.Run:
            self.vins_timings.on_request(
                vins_request.timings.begin_request_ts,
                vins_request.timings.end_request_ts
            )
            personal_data_end_ts = self.processor.get_event_age(events.EventVinsPersonalDataEnd.NAME)
            vins_request.timings.set_personal_data_ts(personal_data_end_ts)

        if force_eou and self.support_forced_eou:
            self.vins_partial_results[request_text] = vins_response
            self.vins_timings.on_select_useful_request(vins_request)
            self.__on_vins_response__(raw_response=vins_response, force_eou=True)
            GlobalCounter.VINS_PARTIAL_205_SUMM.increment()
        elif self.vins_partial and not eou:
            if vins_request.asr_result_same_as_eou and not vins_request.is_trash_partial:
                if self.e2e_partials or self.yaldi_partials:
                    GlobalCounter.VINS_PARTIAL_200_SUMM.increment()
                self.__process_vins_response__(vins_request=vins_request,
                                               vins_response=vins_response,
                                               asr_result=asr_result,
                                               request_text=request_text)
            else:
                self.vins_partial_results[request_text] = vins_response
                GlobalCounter.VINS_PARTIAL_202_SUMM.increment()
                self.__process_vins_partial__(vins_response)
        elif request_text == self.vins_request_text or eou or self.allow_deferred_apply_check:
            if self.e2e_partials or self.yaldi_partials:
                GlobalCounter.VINS_PARTIAL_200_SUMM.increment()
            self.__process_vins_response__(vins_request=vins_request,
                                           vins_response=vins_response,
                                           asr_result=asr_result,
                                           request_text=request_text)

    def __get_tts_markup__(self, vins_response):
        voice_response = vins_response.get("voice_response", {})
        markup = (voice_response.get('output_speech', {}) or {}).get('text', '')

        if self._emotion_hack:
            emotion = voice_response.get('output_emotion')
            if emotion is None:
                emotion = voice_response.get('output_speech', {}).get('emotion')

            if markup and emotion:
                markup = '<speaker emotion="%s">%s' % (emotion, markup)

        return markup

    def __process_vins_partial__(self, vins_response):
        self.DLOG("Got vins partial: ", vins_response)
        markup = self.__get_tts_markup__(vins_response)
        self.__on_vins_partial__(what_to_say=markup)

    def __process_vins_response__(self,
                                  vins_request,
                                  vins_response,
                                  asr_result,
                                  request_text):
        self.DLOG("Got vins response: ", vins_response)

        what_to_say = self.__get_tts_markup__(vins_response)

        assert vins_request

        if vins_request.type == VinsRequest.RequestType.Run:
            self.vins_timings.on_select_useful_request(vins_request)
            self.vins_timings.on_event(
                event=events.EventResultVinsRunResponseIsReady,
                timestamp_sec=vins_request.timings.end_request_ts
            )
        elif vins_request.type == VinsRequest.RequestType.Apply:
            self.vins_timings.on_apply_request(
                vins_request.timings.begin_request_ts,
                vins_request.timings.end_request_ts
            )

        directives = list(filter(
            lambda x: x.get("type") == "uniproxy_action",
            vins_response.get("voice_response", {}).pop("directives", [])
        ))

        self.system.logger.log_directive(
            {
                "ForEvent": self.message_id,
                "type": "VoiceResponseDirectives",
                "Body": {"directives": directives}
            },
            rt_log=self.rt_log,
        )

        deferred_apply, session = False, None
        for directive in vins_response.get("response", {}).get("directives", []):
            if directive.get("type", None) == "uniproxy_action":
                name = directive.get("name", None)
                if name == "defer-apply":
                    deferred_apply = True
                    session = directive.get("payload", None)
                    break
                if name == "defer_apply":
                    self.DLOG("Got DEFERRED_APPLY")
                    deferred_apply = True
                    session = directive.get("payload", {}).get("session", None)
                    break

        if deferred_apply:
            if vins_request.type == VinsRequest.RequestType.Apply:
                # we don't know if it's valid or not but it may cause hanging request
                self.ERR("got 'deferred apply' directive on 'apply'")

            if not self.all_vins_requests.has_vins_apply_backend():
                # fork coroutine here
                self.__start_vins_apply_request(vins_request, session, request_text, asr_result)
            return

        # This flag is being used for double-check, because of due to
        # async uniproxy we may have two async
        # __process_vins_response__() calls in the same time.
        if self.vins_response_processed:
            return
        self.vins_response_processed = True

        self.vins_timings.on_event_with_value(
            event=events.EventLastVinsRunRequestIntentName,
            value=VinsAdapter.__get_intent_from_vins_response(vins_response)
        )

        self.vins_timings.on_select_useful_request(vins_request)
        self.__on_vins_response__(raw_response=vins_response, vins_directives=directives, what_to_say=what_to_say)
        self.close()

    @staticmethod
    def __get_intent_from_vins_response(vins_response):
        response = vins_response.get("response", {})

        for info in response.get("meta", []):
            if info.get("type", "") == "analytics_info":
                return info.get("intent", "")

        return response.get("features", {}).get("form_info", {}).get("intent", "")

    def log_partial(self, status, text):
        self.system.logger.log_directive(
            {
                "type": "VinsPartial",
                "status": status,
                "refMessageId": self.message_id
            },
            rt_log=self.rt_log,
        )
        self.rt_log('vins.end_of_utterance', status=status, text_result=text)

    def set_partial_as_good(self, req):
        self.vins_timings.useful_partial = True
        self.vins_timings.useful_vins_response_before_eou = True
        req.asr_result_same_as_eou = True

    def on_eou_when_waiting_for_good_partial(self, req, text, asr, partialno):
        GlobalCounter.VINS_PARTIAL_GOOD_WAIT_SUMM.increment()
        self.set_partial_as_good(req)

    def on_eou_when_partial_is_good(self, req, resp, text, asr, partialno):
        GlobalCounter.VINS_PARTIAL_GOOD_SUMM.increment()

        self.set_partial_as_good(req)

        self.log_partial('good_partial', text)

        self.__process_vins_response__(
            vins_request=req,
            vins_response=resp,
            asr_result=req.asr_result or asr,  # trying to use same asr-partial for apply as for run
            request_text=text
        )

    def on_eou_when_new_request_required(self, req, text, asr, is_whisper, partialno):
        self.vins_request_text = text
        self.vins_request_parts = self.get_request_parts()

        self.__start_vins_request__(
            self.vins_request_parts,
            asr_result=asr,
            end_of_utterance=True,
            is_whisper=is_whisper,
            request_text=text,
            asr_partial_number=partialno,
        )

    def on_eou_when_partial_is_trash(self, req, text, asr, is_whisper, partialno):
        GlobalCounter.VINS_PARTIAL_TRASH_VINS_SUMM.increment()
        self.vins_partial = False
        self.log_partial('trash_partial', text)
        self.on_eou_when_new_request_required(req, text, asr, is_whisper, partialno)

    def on_eou_when_partial_is_bad(self, req, text, asr, is_whisper, partialno):
        GlobalCounter.VINS_PARTIAL_BAD_SUMM.increment()
        self.vins_partial = False
        self.log_partial('bad_partial', text)
        self.on_eou_when_new_request_required(req, text, asr, is_whisper, partialno)

    def on_eou_when_asr_is_trash_or_empty(self, result):
        GlobalCounter.VINS_PARTIAL_TRASH_ASR_SUMM.increment()
        if result.get('is_trash'):
            self.__on_vins_cancel__(reason='trash result')
        else:
            self.__on_vins_cancel__(reason='empty result')
        self.__on_vins_response__()
        self.close()

    def on_eou_when_partials_are_disabled(self, text, asr, is_whisper, partialno):
        self.rt_log('vins.end_of_utterance', status='no_partial', text_result=text)
        self.on_eou_when_new_request_required(None, text, asr, is_whisper, partialno)

    def __on_end_of_utt__(self, result):
        assert(self.end_of_utt is False)
        self.end_of_utt = True
        self.vins_timings.on_event(event=events.EventEndOfUtterance)
        text_result, asr_result = self.__update_asr_result_to_vins_format__(result)
        asr_partial_number = result['asr_partial_number']
        is_whisper = result.get('whisperInfo', {}).get('isWhisper')
        if is_whisper:
            self.whisper = True

        if not text_result:
            self.on_eou_when_asr_is_trash_or_empty(result)
            return

        if not self.vins_partial:
            self.on_eou_when_partials_are_disabled(text_result, asr_result, is_whisper, asr_partial_number)
            return

        req = self.all_vins_requests.get(text_result)
        if not req:
            self.on_eou_when_partial_is_bad(req, text_result, asr_result, is_whisper, asr_partial_number)
            return

        if req.is_trash_partial:
            self.on_eou_when_partial_is_trash(req, text_result, asr_result, is_whisper, asr_partial_number)
            return

        resp = self.vins_partial_results.get(text_result)
        if resp is None:
            self.on_eou_when_waiting_for_good_partial(req, text_result, asr_result, asr_partial_number)
            return

        self.on_eou_when_partial_is_good(req, resp, text_result, asr_result, asr_partial_number)

    def request_score(self, yabiostream, score_type, vins_request):
        self.DLOG('request', score_type)
        # self.vins_requests required set (now or later) yabio score result
        if yabiostream.last_result_is_actual():
            self.DLOG('last ', score_type, 'result actual')
            if score_type == 'yabio':
                vins_request.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                self.all_vins_requests.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                if not self.scoring_result:
                    # EOU on empty audio ?
                    self.WARN(score_type, 'empty actual scoring result ??')
            else:
                vins_request.set_classify_result(self.classify_result)
                self.all_vins_requests.set_classify_result(self.classify_result)
                if not self.classify_result:
                    # EOU on empty audio ?
                    self.WARN(score_type, 'empty actual classify result ??')
        else:
            # need more fresh scoring
            try:
                # we MUST guarantee receiving score (else we lock on future), so check add_chunk result
                # weak method, but :( ?!
                if score_type == 'yabio':
                    need_score_processed_chunks = yabiostream.add_chunk(need_result=True)
                    if not need_score_processed_chunks:
                        self.DLOG(score_type, 'fail add_chunk with need_result')
                        vins_request.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                        self.all_vins_requests.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                    else:
                        vins_request.need_score_processed_chunks = need_score_processed_chunks
                        self.DLOG('wait next', score_type, 'result')
                        # call set_yabio_result from callback
                else:
                    need_classify_processed_chunks = yabiostream.add_chunk(need_result=True)
                    if not need_classify_processed_chunks:
                        self.DLOG(score_type, 'fail add_chunk with need_result')
                        vins_request.set_classify_result(self.classify_result)
                        self.all_vins_requests.set_classify_result(self.classify_result)
                    else:
                        vins_request.need_classify_processed_chunks = need_classify_processed_chunks
                        self.DLOG('wait next', score_type, 'result')
                        # call set_yabio_result from callback
            except Exception as exc:
                self.WARN(score_type, 'fail request fresh yabio score: {}'.format(str(exc)))
                if score_type == 'yabio':
                    vins_request.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                    self.all_vins_requests.set_yabio_result(self.scoring_result, end_of_utterance=self.end_of_utt)
                else:
                    vins_request.set_classify_result(self.classify_result)
                    self.all_vins_requests.set_classify_result(self.classify_result)

    def __update_asr_result_to_vins_format__(self, result):
        remove_align_info = not self._conducting_experiment("vin_request_keep_align_info")
        asr_result = result.get("e2e_recognition", result.get("recognition", []))

        for x in asr_result:
            if self.spotter_glue_phrase:
                x["utterance"] = self.spotter_glue_phrase + ' ' + x["normalized"].lower()
            else:
                x["utterance"] = x["normalized"].lower()

            if remove_align_info and "alignInfo" in x:
                del x["alignInfo"]
            for y in x.get("words", []):
                y["value"] = y["value"].lower()
                if remove_align_info and "alignInfo" in y:
                    del y["alignInfo"]
        if result.get('context_hints') and len(asr_result) > 0:
            asr_result[0]['context_hints'] = result['context_hints']
        if asr_result and result.get('cacheKey', None) is not None:
            return (result['cacheKey'], asr_result)
        elif asr_result and asr_result[0]["normalized"]:
            return (" ".join([x["value"] for x in asr_result[0]["words"]]), asr_result)
        else:
            return ("", asr_result)

    def DLOG(self, *args):
        try:
            self._log.debug(self.system.session_id, *args, rt_log=self.rt_log)
        except ReferenceError:
            pass

    def INFO(self, *args):
        try:
            self._log.info(self.system.session_id, *args, rt_log=self.rt_log)
        except ReferenceError:
            pass

    def ERR(self, *args):
        try:
            self._log.error(self.system.session_id, *args, rt_log=self.rt_log)
        except ReferenceError:
            pass

    def WARN(self, *args):
        try:
            self._log.warning(self.system.session_id, *args, rt_log=self.rt_log)
        except ReferenceError:
            pass
