import re
import weakref
import time

from typing import Callable

import tornado.gen

from alice.uniproxy.library.settings import config

from alice.uniproxy.library.backends_common.protostream import ProtoStream
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json

from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import TTS_REQUEST_HGRAM
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_state import GlobalTags

from voicetech.library.proto_api.ttsbackend_pb2 import Generate, GenerateResponse


class TtsStat:
    def __init__(self, speaker, voice):
        self.voice = voice  # originally requested voice

        speaker_mode = speaker.get("mode")
        if speaker_mode in ("gpu", "gpu_valtz", "gpu_oksana"):
            self.mode = speaker_mode
        elif speaker.get("lang", "ru") == "ru":
            self.mode = "ru"
        else:
            self.mode = None

        self._result_count = 0

    def process_result_code(self, code):
        if self.mode is None:
            return

        self._result_count += 1

        mode = self.mode.upper()
        if self._result_count > 1:
            mode += "_FALLBACK"

        if code == 200:
            signame = "TTS_{}_200_SUMM"
        else:
            signame = "TTS_{}_ERR_SUMM"

        counter = getattr(GlobalCounter, signame.format(mode), None)
        if counter is not None:
            counter.increment()

    def process_request_duration(self, duration):
        if self.mode is None:
            return
        GlobalTimings.store("tts_{}_request".format(self.mode), duration)


class TtsStream(ProtoStream):
    default_json = {
        "chunked": True,
        "speed": 1.0,
    }

    def __init__(
            self,
            callback: Callable[[GenerateResponse], None],
            error_callback: Callable[[str], None],
            params,
            host=None,
            rt_log=None,
            unistat_counter='tts',
            system=None,
            message_id=None
    ):
        super(TtsStream, self).__init__(None, None, None, rt_log, rt_log_label='tts')
        self._log = Logger.get('uniproxy.backends.ttstream')
        self.rt_log = rt_log
        self.initial_samplerate = "48000"
        self.params = TtsStream.default_json.copy()
        if system:
            self.params['clientHostname'] = system.hostname
        if message_id:
            self.params['message_id'] = message_id
        self.params.update(params)

        self.not_send_mime = self.params.pop('not_send_mime', False)
        self.check_callback = None
        self._start_time = time.time()
        self.check_timeout = config["ttsserver"].get("check_timeout", 70)
        self.callback = weakref.WeakMethod(callback)
        self.error_callback = weakref.WeakMethod(error_callback)
        self.host = host or config["ttsserver"]["host"]
        self.speaker = None
        self.init_connection()
        if self.speaker:
            self._stat = TtsStat(self.speaker, self.params["voice"])
        self.unistat_counter = unistat_counter
        self._system = system
        self.audio_size = 0
        self.has_finish_code = False
        self.fallback = False
        if self.params['voice'].startswith('shitova'):
            balancing_mode_key = 'balancing_mode_tts_shitova'
        else:
            balancing_mode_key = 'balancing_mode_tts_other'
        balancer_mode = params.get('settings_from_manager', {}).get(balancing_mode_key, 'pre_prod')
        self.balancing_hint_header = GlobalTags.get_balancing_hint_header(balancer_mode)

    def get_native_format(self):
        return "audio/x-pcm;bit=16;rate={}".format(self.initial_samplerate)

    def init_connection(self):
        self.audio_size = 0
        if "voice" not in self.params:
            self.error_callback()("No voice param provided")
            return None

        self.speaker = config["ttsserver"]["voices"].get(self.params["voice"])
        if self.speaker is None:
            self.error_callback()("Unsupported voice %s" % (self.params["voice"],))
            return None

        self.initial_samplerate = self.speaker.get("sampleRate", "48000")
        self.params["voices"] = self.speaker.get("voices", [])
        self.params["genders"] = self.speaker.get("genders", [])
        for to in self.speaker.get("tuning", []):
            if to["name"] == "msd_threshold":
                self.params["msd_threshold"] = to["weight"]
            elif to["name"] == "lf0_postfilter":
                self.params["lf0_postfilter"] = to["weight"]

        if self.params.get("emotion"):
            self.params["emotions"] = [{"name": self.params["emotion"], "weight": 1.0}]
        else:
            self.params["emotions"] = self.speaker.get("emotions", [])

        if "lang" not in self.params:
            self.error_callback()("No lang param provided")
            return None

        language = self.params["lang"]
        self._log.debug("tts_host={} tts_lang={}".format(self.host, language), rt_log=self.rt_log)
        if language is not None:
            for lang_re, lang_res in config["ttsserver"]["langs"]:
                if re.match(lang_re, language):
                    language = lang_res
        if language is None:
            self.error_callback()("Unsupported language %s" % (self.params["lang"],))
            return None
        elif self.speaker.get("lang", language) != language:
            self.params["lang"] = self.speaker["lang"]
        else:
            self.params["lang"] = language

        uri = self._get_uri()

        super(TtsStream, self).__init__(
            self.speaker.get("host", self.host),
            self.speaker.get("port", config["ttsserver"]["port"]),
            uri,
            rt_log=self.rt_log,
            rt_log_label='tts: {}'.format(uri),
        )
        self.connect()

    def _get_uri(self):
        if "tts_balancer_handle" in self.speaker:
            return self.speaker["tts_balancer_handle"]

        uri = "/%s/" % (self.params["lang"],)
        if "mode" in self.speaker:
            uri += self.speaker["mode"] + "/"

        return uri

    def on_connect(self, duration):
        GlobalTimings.store('tts_connect_time', duration)

    def try_fallback(self):
        if self.audio_size == 0 and not self.params.get('disable_tts_fallback', False):
            if self.speaker and "fallback" in self.speaker:
                self.params["voice"] = self.speaker["fallback"]
                self.close()
                self.fallback = True
                self.init_connection()
                return True
        return False

    def on_error(self, message, code=None):
        if code:
            self._log.error("tts_error code={} message_id={} voice={}: {}".format(code, self.params.get('message_id'), self.params.get("voice"), message))
            self.increment_unistat_counter(code)

        if not self.fallback and self.try_fallback():
            self._log.warning("Fallback to {}".format(self.params["voice"]))
            GlobalCounter.TTS_FALLBACK_ERR_SUMM.increment()
            return

        if self.error_callback and self.error_callback():
            self.error_callback()(message)
        else:
            self._log.warning(message, rt_log=self.rt_log)
        if not self.is_closed():
            self.close()

    def on_data(self, *args):
        if not self.is_closed() and self.callback and self.callback():
            self.callback()(*args, fallback=self.fallback)

    def process(self):
        self.send_request()

    @tornado.gen.coroutine
    def send_request(self):
        proto = None
        try:
            proto = proto_from_json(Generate, self.params)
            if self.not_send_mime and proto.HasField("mime"):  # dirty hack for avoid bug in old tts-servers (lost metatinfo)
                proto.ClearField("mime")
        except Exception as exc:
            self._log.error("Can't create Generate request: %s from %s" % (exc, self.params), rt_log=self.rt_log)
            self._log.exception(exc, rt_log=self.rt_log)
            self.error_callback()("Can't create Generate")
            return
        self.send_protobuf_ex(proto).add_done_callback(self.on_sent_generate_request)

    def on_sent_generate_request(self, future):
        if future.exception():
            self.on_sent_generate_request_impl(None)
        else:
            self.on_sent_generate_request_impl(future.result())

    def on_sent_generate_request_impl(self, result):
        if not result:
            self.on_error("Generate request failed", code=600)
        else:
            self.read_protobuf_ex(GenerateResponse).add_done_callback(self.on_generate_response)

    def on_generate_response(self, future):
        if future.exception():
            self.on_generate_response_impl(None)
        else:
            self.on_generate_response_impl(future.result())

    def on_generate_response_impl(self, result):
        if not result:
            self.on_error("stream is closed", code=600)
            return

        if result.audioData is not None:
            self.audio_size += len(result.audioData)
            self._log.debug("Got chunk from tts: {} message: {}".format(len(result.audioData), result.message), rt_log=self.rt_log)
        else:
            self._log.debug("Got chunk from tts: Empty; message: {}".format(result.message), rt_log=self.rt_log)

        if not result.audioData and ((not result.completed and not result.timings) or result.responseCode != 200):
            status = result.responseCode if result.responseCode != 200 else 600
            self.on_error(result.message, code=status)
            return

        self.on_data(result)

        if not result.completed:
            self.read_protobuf_ex(GenerateResponse).add_done_callback(self.on_generate_response)
        else:
            duration = time.time() - self._start_time
            GlobalTimings.store(TTS_REQUEST_HGRAM, duration)
            self._stat.process_request_duration(duration)
            self.increment_unistat_counter(200)

    def increment_unistat_counter(self, code):
        self._stat.process_result_code(code)
        if self.has_finish_code:
            return

        self.has_finish_code = True
        GlobalCounter.increment_error_code(self.unistat_counter, code)
        try:
            if self._system:
                self._system.increment_stats(self.unistat_counter, code)
                if self._stat.voice in ("oksana", "good_oksana"):
                    self._system.increment_stats(self.unistat_counter + "_oksana", code)
        except ReferenceError:
            pass
