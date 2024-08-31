import weakref
import json

import tornado.gen

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import SPOTTER_MAPS
from alice.uniproxy.library.utils import deepupdate
from alice.uniproxy.library.utils.experiments import conducting_experiment, mm_experiment_value
from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.utils.timestamp import PerformanceCounter

from alice.uniproxy.library.activation_storage import make_activation_storage, SpotterFeatures

from . import YaldiStream


class SpotterStream(YaldiStream):
    spotter_conf = {
        "advanced_options": {
            "allow_multi_utt": False,
            "punctuation": False,
            "partial_results": False,
            "request_front": 1000,
            "spotter_back": 2000,
        }
    }

    def make_spotter_features(self):
        rms = self.params.get("request", {}).get("additional_options", {}).get("spotter_rms", [])

        coeff = 1.0
        yandex_station_device_id_length = 20

        if self._device_id:
            if self._device_id[0] == 'L':
                coeff = float(
                    mm_experiment_value('mul_rms_for_activation_on_yandexmicro',
                                        self.params, self._system.uaas_flags) or 1.0
                )
            # hack to select yandex_station devices for rms multiplier experiment
            elif len(self._device_id) == yandex_station_device_id_length:
                coeff = float(
                    mm_experiment_value('mul_rms_for_activation_on_station',
                                        self.params, self._system.uaas_flags) or 1.0
                )

        return SpotterFeatures().with_rms(rms, coefficient=coeff)

    def get_speakers_count(self):
        try:
            rval = self.params.get("request", {}).get("additional_options", {}).get("speakers_count", 0)
            return int(rval)
        except:
            pass
        return 0

    def conducting_experiment(self, exp_name):
        return conducting_experiment(exp_name, self.params, self._system.uaas_flags)

    def _make_activation_storage(self, smart_activation):
        settings_from_manager = self.params.get('settings_from_manager', {})

        self._activation_running = False

        smart_activation_demanded = all([
            (smart_activation is not False),
            (self.get_speakers_count() != 1),
            (not self.conducting_experiment('skip_multi_activation_check')),
            (self.conducting_experiment('supress_multi_activation')),
        ])

        self._two_steps_activation = smart_activation_demanded
        self._allow_activation_by_unvalidated_spotter = smart_activation_demanded

        cachalot_kwargs = dict(
            retry_delay_milliseconds=settings_from_manager.get('cachalot_activation_retry_delay_milliseconds', 0),
            freshness_delta_milliseconds=int(
                mm_experiment_value('cachalot_activation_freshness_delta_milliseconds',
                                    self.params, self._system.uaas_flags) or 0
            ) or None,
            allow_activation_by_unvalidated_spotter=self._allow_activation_by_unvalidated_spotter,
        )

        self._activation_storage = make_activation_storage(
            self._two_steps_activation, smart_activation_demanded, self._uid, self._device_id,
            self.make_spotter_features(), cachalot_kwargs=cachalot_kwargs,
        )

        self._activation_storage.start()

    def __init__(self, callback, error_callback, params, session_id, message_id,
                 close_callback=None, system=None, rt_log=None, rt_log_label=None,
                 smart_activation=None, apphosted_asr=False):
        self._log = Logger.get('.spotterstream')
        self._message_id = message_id
        self.session_id = session_id
        self.rt_log = rt_log
        self._system = system
        self.params = deepupdate(params, self.spotter_conf)
        deepupdate(self.params["advanced_options"], self.params.get("spotter_options", {}), copy=False)
        self.after_stream_control = False
        self.params['advanced_options']['spotter_validation'] = True
        spotter_phrase = self.params.get("spotter_phrase")
        if spotter_phrase:
            self.params['advanced_options']['spotter_phrase'] = spotter_phrase
        embedded_spotter_info = self.params.get('embedded_spotter_info')
        # IF YOU SEE THIS AND THE DATE IS 2021 MAY 22 OR LATER, REMOVE THE CODE BELOW
        if not embedded_spotter_info:
            embedded_spotter_info = self.params.get('spotter_metainfo')
        # END OF CODE TO BE REMOVED
        if embedded_spotter_info:
            self.params['advanced_options']['embedded_spotter_info'] = embedded_spotter_info
        self.activation_phrase = self.params.get("spotter_phrase", "").strip().replace(" ", "").lower()
        self.params["topic"] = SPOTTER_MAPS.get(
            lang=self.params.get("lang", "ru-RU"),
            topic=self.params["topic"],
            phrase=self.activation_phrase
        )
        self.DLOG("Request topic {} mapped to {}".format(params["topic"], self.params["topic"]))
        self.result_callback = weakref.WeakMethod(callback)

        self._device_id = deepupdate(
            self.params.get('vins', {}).get('application', {}),
            self.params.get('application', {})
        ).get('device_id')
        if not self._device_id:
            self._device_id = self.params.get("request", {}).get("device_state", {}).get("device_id", None)

        self._uid = self._system and self._system.uid

        self._make_activation_storage(smart_activation)
        super().__init__(callback, error_callback, self.params, session_id, message_id, close_callback,
                         unistat_counter='spotter', rt_log=rt_log, rt_log_label=rt_log_label, system=system,
                         apphosted_asr=apphosted_asr, is_spotter=True)
        # Rewrite after super()
        self._log = Logger.get('.spotterstream')
        self.is_spotter = True

    @tornado.gen.coroutine
    def multi_activation_check_first(self, spottered):
        GlobalCounter.SPOTTER_ACTIVATION_REQUESTS_SUMM.increment()

        if not self._uid or not self._device_id:
            skip_reason = 'ACTIVATION no uid({}) or device_id({}) for supressing multi activation'.format(
                self._uid, self._device_id)
            self.INFO(skip_reason)
            GlobalCounter.SPOTTER_ACTIVATION_INVALID_DATA_SUMM.increment()
            return True, skip_reason

        self.DLOG('ACTIVATION using storage for checking multi activation')
        with UnistatTiming('spotter_multi_activation'):
            activation, log_info = yield self._activation_storage.activate(
                self._uid, self._device_id, spotter_validated=spottered,
            )
            self._system.logger.log_directive(
                {
                    "ForEvent": self._message_id,
                    "type": "MultiActivation",
                    "Body": log_info
                },
                rt_log=self.rt_log,
            )

        if activation:
            GlobalCounter.SPOTTER_ACTIVATION_ALLOWED_SUMM.increment()
        else:
            GlobalCounter.SPOTTER_ACTIVATION_CANCELLED_SUMM.increment()

        return activation, json.dumps(log_info)

    def increment_activate_counter(self, activation, final=False):
        if not self._two_steps_activation:
            if activation:
                GlobalCounter.SPOTTER_MULTI_ACTIVATION_ACTIVATE_SUMM.increment()
            else:
                GlobalCounter.SPOTTER_MULTI_ACTIVATION_CANCEL_SUMM.increment()
        else:
            if activation:
                GlobalCounter.SPOTTER_MULTI_ACTIVATION_ACTIVATE_TWO_STEPS_SUMM.increment()
            else:
                if final:
                    GlobalCounter.SPOTTER_MULTI_ACTIVATION_CANCEL_SECOND_STEP_SUMM.increment()
                else:
                    GlobalCounter.SPOTTER_MULTI_ACTIVATION_CANCEL_FIRST_STEP_SUMM.increment()

    @tornado.gen.coroutine
    def multi_activation_check_final(self, need_activate, reason, subtype="normal"):
        if not need_activate or not self._two_steps_activation:
            self.increment_activate_counter(need_activate)
            return need_activate, reason

        with UnistatTiming('spotter_multi_activation_final'):
            fail_reason = []
            activation, log_info = yield self._activation_storage.activate(
                self._uid, self._device_id, fail_reason=fail_reason, final=True
            )
            self._system.logger.log_directive(
                {
                    "ForEvent": self._message_id,
                    "type": "MultiActivationFinal",
                    "Body": log_info,
                    "subtype": subtype
                },
                rt_log=self.rt_log,
            )
            if fail_reason:
                log_info = '{}: {}'.format(fail_reason, log_info)
            self.increment_activate_counter(activation, True)
            return activation, json.dumps(log_info)

    def timer_name(self):
        if self._two_steps_activation:
            return "spotter_multi_activation_check_two_steps"
        else:
            return "spotter_multi_activation_check"

    @tornado.gen.coroutine
    def multi_activation_check(self, spottered):
        perf = PerformanceCounter('MultiActivationCheck', self._log)
        perf.start()
        with UnistatTiming(self.timer_name()):
            need_activate, reason = yield self.multi_activation_check_first(spottered)
        try:
            need_activate, reason = yield self.multi_activation_check_final(need_activate, reason)
        except Exception as e:
            self.INFO("Something wrong during final check: {} {}".format(type(e).__name__, e))
        perf.stop()
        return need_activate, reason

    def multi_activation_cancel(self):
        if self._two_steps_activation:
            yield self._activation_storage.cancel()

    # here we come when processor/vins.py timeouted on waiting spotter_validation future
    @tornado.gen.coroutine
    def fast_circuit(self):
        if self._two_steps_activation:
            self.final(0)
            need_activate, reason = yield self.multi_activation_check_final(True, None, "fastcircuit")
            return need_activate
        return True

    @tornado.gen.coroutine
    def on_data(self, *args):
        if not args:
            return
        result = args[0]
        spottered = True

        status = result.get("responseCode", "OK")
        if status != "OK":
            self.log_result(result)
            if self.error_callback and self.error_callback():
                self.error_callback()("Bad spotter validation response: {}".format(status))
            self.close()

        if result.get("endOfUtt", False) and not self._activation_running:
            self.log_result(result)
            self.DLOG(result)
            recognition = result.get("recognition", [])
            if recognition:
                words = [x.get("value", "") for x in recognition[0].get("words", [])]
                text = "".join(words)
                self.DLOG("spotting", self.activation_phrase, "got", text)
                if self.activation_phrase not in text.lower():
                    spottered = False
            else:
                self.DLOG("nothing recognized")

            #
            #   VOICESERV-3235: dirty hack to fix core dumps
            #
            self.force_last_chunk()

            yield self.on_spotter_validation(spottered)

    @tornado.gen.coroutine
    def on_spotter_validation(self, spottered, spotter_text=''):
        if self._apphosted_asr:
            self.increment_unistat_counter(200)
        canceled_cause_of_multiactivation = False
        multiactivation_id = None
        if spottered or self._allow_activation_by_unvalidated_spotter:
            self._activation_running = True
            need_activate, reason = yield self.multi_activation_check(spottered)
            if not need_activate:
                self.INFO('{} ACTIVATION storage suppressed activation: another device activated ({})'.format(
                    self._message_id, reason))
                canceled_cause_of_multiactivation = True
            else:
                self.INFO('{} ACTIVATION OK: {}'.format(self._message_id, reason))
            try:
                log_info = json.loads(reason)
                ts = str(log_info['ActivatedTimestamp'])
                device_id = str(log_info['ActivatedDeviceId'])
                multiactivation_id = ts + device_id
            except Exception:
                multiactivation_id = None
        else:
            self.multi_activation_cancel()

        if self.result_callback and self.result_callback():
            self.result_callback()(
                spottered, spotter_text, canceled_cause_of_multiactivation, multiactivation_id,
                allow_activation_by_unvalidated_spotter=self._allow_activation_by_unvalidated_spotter,
            )
            self.result_callback = None

        self.close()

    def add_chunk(self, data=None):
        self.DLOG("chunk size ", len(data) if data is not None else "-")
        if data is None and not self.after_stream_control:
            # it means end of spotter phrase
            self.DLOG("Got streamcontrol")
            self.after_stream_control = True
            return
        super().add_chunk(data)

    @tornado.gen.coroutine
    def final(self, max_wait_ms):
        if self._two_steps_activation:
            yield self._activation_storage.unlock_final(max_wait_ms)

    def log_result(self, result):
        if self._system:
            self._system.logger.log_directive(
                {
                    "type": "SpotterValidationResult",
                    "ForEvent": self._message_id,
                    "result": result,
                },
                rt_log=self.rt_log,
            )

    def DLOG(self, *args):
        self._log.debug("SPOTTER:", *args)

    def INFO(self, *args):
        self._log.info('SPOTTER:', *args)
